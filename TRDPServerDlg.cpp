// FTPServerDlg.cpp : implementation file
//
#include "StdAfx.h"
#include "TRDPServerDlg.h"
#include "stdio.h"
#include "TRDPServer.h"
#include "ListenSocket.h"	// Added by ClassView
#include "CSpreadSheet.h"
#include "winsock.h"
#include "stdlib.h"

#include <string.h>
#include <windows.h>

#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"ws2_32.lib")
#define DEBUG_PRINTF  // 添加调试控制窗口


#ifdef DEBUG_PRINTF
#include <io.h>
#include <fcntl.h>

void InitConsole()
{
	int nRet= 0;
	FILE* fp;
	AllocConsole();
	nRet= _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
	fp = _fdopen(nRet, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);
}
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



#define RESERVED_MEMORY 160000

static Csharememoryclient m_shareMemory;  // 共享内存

#if 0
typedef struct _ST_DATA_INFO {
	char     strType[20]; 
	unsigned data[20000];   // 数据
} st_data_info;
#endif 

#define LENGTH_1701 1946
#define LENGTH_1801 4666

enum TRDPPortEnum{
	PUBLISH = 0,
	SUBSCRIBE
} trdpdir;


unsigned char cData1701[2300] = {0}; // 1701 1702公共端口，记录内容，记录周期100ms，数据刷新应小于100ms，每小时整点生成一个文件，文件后缀为edf ，数据类型为0x42
unsigned char cData1801[5000] = {0}; // 1801 1802 1301 1302 记录内容，记录周期1s，数据刷新周期500ms，每小时整点生成一个文件，文件后缀为eds，数据类型为0x43。
unsigned char DeviceID = 0;
/* 存放 TRDP数据。 */
stTrdpDataInfo data_put_test[10];
stTrdpDataInfo data_get_record[80]; 

HANDLE  hWriteDRU1;  // 互斥量
HANDLE  hWriteDRU2;  // 互斥量

CString strFtpIP;           // ftp 服务器ip地址
bool    isFileOpen = FALSE; // 文件是否被打开
int     g_iCycleCnt = 0;    // trdp刷新时间计数，以10ms定时器为基准，适应其他周期的数据

unsigned short g_iComTotalNum = 0; // 要记录变量的端口数量
unsigned int  g_iTRDP_common_size; // 公共以太网 端口数量
unsigned int  g_iTRDP_record_size; // 记录以太网 端口数量

stTrdpPortPara trdpport_comm[10];    // 公共端口 配置
stTrdpPortPara trdpport_record[120]; // 记录端口 配置

mapFileInfo  g_mapFileLocal;   // 存储本地文件信息：文件名, 文件大小
mapFileInfo  g_mapFileServer;  // 存储ftp服务端文件信息：文件名，文件大小
listFileInfo g_uploadFileList; // 待同步 文件名
listFileInfo g_removeFileList; // 待删除 文件名
stConfigInfo stCfgInfo[200];   // 端口属性 设置

/* ==================================================================================
 * Local/Global variables definitions
 * ================================================================================*/
static TRDP_APP_SESSION_T session_handle;
static TRDP_MD_INFO_T rx_callback_msg;

static char   recv_buffer[1000];
static UINT32 recv_length;
static UINT32 is_msg_received;

TRDP_PD_CONFIG_T      pd_Config;
TRDP_MEM_CONFIG_T     trdpMem;
TRDP_PROCESS_CONFIG_T process_Config;

DG_U32          ret_val;
TRDP_PD_INFO_T  pd_info[150];
TRDP_PUB_T      pub_handle[5];      // 发布 句柄
TRDP_SUB_T      sub_handle[180];    // 订阅 句柄

#define TRDPSENDTIMER    10

/* ==================================================================================
 * Local function declarations
 * ================================================================================*/
static void print_log(void *pRefCon, VOS_LOG_T category, const CHAR8 *pTime, const CHAR8 *pFile, UINT16 line, const CHAR8 *pMsgStr);
static void wait_for_msg();
static DG_S32 trdp_pd_demo(DG_S32 *trdp_err_no);
static void md_callback(void *ref, TRDP_APP_SESSION_T apph, const TRDP_MD_INFO_T *msg, UINT8 *data, UINT32 size);

/* ==================================================================================
 * Local function implementations
 * ================================================================================*/

static void wait_for_msg()
{
	while (0 == is_msg_received) {
		tlc_process(session_handle, NULL, NULL);
		Sleep(100);
	}

	is_msg_received = 0;
}

static void print_log (void *pRefCon, VOS_LOG_T category, const CHAR8 *pTime,
		const CHAR8 *pFile, UINT16 line, const CHAR8 *pMsgStr)
{
	static const char *cat[] = { "ERR", "WAR", "INF", "DBG" };
	const char *file = strrchr(pFile, '/');

	if (category < 0) {
		fprintf(stderr, "%s %s:%d %s", cat[category], file ? file + 1 : pFile, line, pMsgStr);
	}
}

static void md_callback (void *ref, TRDP_APP_SESSION_T apph, const TRDP_MD_INFO_T *msg, UINT8 *data, UINT32 size)
{
	memcpy(&rx_callback_msg, msg, sizeof(TRDP_MD_INFO_T));
	recv_length = size;
	memcpy(recv_buffer, data, recv_length);
	is_msg_received = 1;
}

DG_S32 CTRDPServerDlg::trdp_pd_demo(DG_S32 *trdp_err_no)
{
	for (int i = 0; i< g_iTRDP_record_size; i++) {
		/* publish and subscribe telegrams */
		if ( trdpport_record[i].direction == PUBLISH) {  // 发送 {发布} 
			ret_val = tlp_publish(session_handle, &pub_handle[i], trdpport_record[i].comid, 0, 0, 0, vos_dottedIP(trdpport_record[i].trdpip),
					trdpport_record[i].cycle * 1000, 0, TRDP_FLAGS_NONE, NULL, NULL, trdpport_record[i].portsize);
		} else if (trdpport_record[i].direction == SUBSCRIBE) { // 接收 {订阅} 
			ret_val = tlp_subscribe(session_handle, &sub_handle[i], NULL, NULL, trdpport_record[i].comid, 0, 0, 0, 
					vos_dottedIP(trdpport_record[i].trdpip), TRDP_FLAGS_NONE, trdpport_record[i].cycle * 1000, TRDP_TO_SET_TO_ZERO);
		}	
	}

	for (int j = 0; j < g_iTRDP_common_size; j++) {
		/* publish and subscribe telegrams */
		if (trdpport_comm[j].direction == PUBLISH) {  // 发送 {发布} 
			ret_val = tlp_publish(session_handle, &pub_handle[j], trdpport_record[j].comid, 0, 0, 0, vos_dottedIP(trdpport_record[j].trdpip),
					trdpport_record[j].cycle * 1000, 0, TRDP_FLAGS_NONE, NULL, NULL, trdpport_record[j].portsize);
		} else if (trdpport_comm[j].direction == SUBSCRIBE) { // 接收 {订阅} 
			ret_val = tlp_subscribe(session_handle, &sub_handle[j], NULL, NULL,  trdpport_comm[j].comid, 0, 0, 0, 
					vos_dottedIP(trdpport_comm[j].trdpip), TRDP_FLAGS_NONE, trdpport_comm[j].cycle * 1000, TRDP_TO_SET_TO_ZERO);
		}	
	}

	return 0;
}

///////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About
class CAboutDlg : public CDialog
{
	public:
		CAboutDlg();

		// Dialog Data
		//{{AFX_DATA(CAboutDlg)
		enum { IDD = IDD_ABOUTBOX };
		//}}AFX_DATA

		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CAboutDlg)
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

		// Implementation
	protected:
		//{{AFX_MSG(CAboutDlg)
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CTRDPServerDlg dialog

CTRDPServerDlg::CTRDPServerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTRDPServerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTRDPServerDlg)
	m_bStop       = false;   // 暂停按钮  方便调试
	m_iport       = 1000;
	m_TRDPReceive = "TRDP data:";
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTRDPServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTRDPServerDlg)
	DDX_Control(pDX, IDC_EDIT2, m_status);
	DDX_Control(pDX, IDC_LIST1, m_filelist);
	//DDX_Text(pDX, IDC_EDIT1, m_iport);
	DDX_Text(pDX, IDC_EDIT3, m_TRDPReceive);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTRDPServerDlg, CDialog)
	//{{AFX_MSG_MAP(CTRDPServerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
	//ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CTRDPServerDlg message handlers

BOOL CTRDPServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);

	if (pSysMenu != NULL) {
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);

		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	LV_COLUMN lvColumn;
	lvColumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	lvColumn.fmt  = LVCFMT_LEFT;
	lvColumn.cx   = 200;

	if (m_shareMemory.initsharememory()) { // 初始化失败
		m_status.SetWindowText("TRDP服务器未启动！\n");
		TRACE("TRDP服务器未启动！\n");
		return 0;
	}

#ifdef DEBUG_PRINTF
	InitConsole(); // 调试控制台
#endif

	hWriteDRU1 = CreateMutex(NULL,FALSE,NULL); // 互斥量
	hWriteDRU2 = CreateMutex(NULL,FALSE,NULL); // 互斥量
	lvColumn.iSubItem = 0;
	lvColumn.pszText  = "文件名";
	m_filelist.InsertColumn(0,&lvColumn);

	lvColumn.iSubItem = 1;
	lvColumn.pszText  = "文件大小";
	m_filelist.InsertColumn(1,&lvColumn);

	// TODO: Add extra initialization here
	m_IsServerOn = FALSE;
	flag  = 0;
	flag1 = 0;
	flag2 = 0;
	m_ilen_comm = 1;  // 公共端口 的个数
	m_ilen_record = 1;  // 记录端口数据的个数

	getPort();
	OnStartTRDP() ; //启动TRDP
	m_strInfo = "TRDP服务器启动！";

	printf("TRDP服务器启动！");

	m_status.SetWindowText(m_strInfo);

	SetTimer(TRDPSENDTIMER, 10, NULL);  // 设置定时器 10ms

	/* 启动线程 */
	AfxBeginThread(ThreadTrdpDataProcess, NULL);
	AfxBeginThread(ThreadFileSync, NULL);
	AfxBeginThread(ThreadFileRecord, NULL);
	//	AfxBeginThread(ThreadFileRecord, 0, THREAD_PRIORITY_HIGHEST);


	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTRDPServerDlg::OnButton3() 
{
	// TODO: Add your control notification handler code here
	if (m_bStop) {
		m_bStop = false;
	} else {
		m_bStop = true;
	}
	// OnOK();	
}

/**
 * @brief    --  ThreadFileSync: FTP文件传输
 * @param[ ] -- lpParam
 * @return   -- 
 */
UINT ThreadFileSync(LPVOID lpParam)
{
	while(1) {

		printf("start thread 22 ...\n");
		CFTPFunc ftp;
		if (false == ftp.ConnectToServer("127.0.0.1", "liuwming", "12345678", 21)) { // 连接FTP服务器  
			printf("No Connected!!");
			continue;
		}

		ftp.findFileFTPServer(g_mapFileServer, ".");  // 当前文件

		ftp.findFileLocal(g_mapFileLocal, "E://build Test1");  // 获取本地文件列表
		//ftp.findFileLocal(g_mapFileLocal, "D://WTD//DC250//四方标动程序//TRDP");  // 获取本地文件列表

		CString strFileName;

		bool bUpload = ftp.checkFileWaitUpload(g_uploadFileList);  // 查找待上传文件信息
		if (false == bUpload) {
			printf("local no file !");
			Sleep(5000);	// 延时5秒
			continue;
		}

		bool bRemove = ftp.checkFileWaitRemove(g_removeFileList);  // 查找待删除文件
		if (false == bRemove) {
			printf("no file to remove !");
			Sleep(5000);	// 延时5秒
			continue;
		}

		printf("\n\n");     
		POSITION posRemove = g_removeFileList.GetHeadPosition();
		while (posRemove != NULL) {
			strFileName =  g_removeFileList.GetNext(posRemove);
			printf("remove [%s] \n", strFileName);   
			bool bret = ftp.removeFile("./" + strFileName);  
			printf("%d\n\n\n", bret);  
		}

		printf("\n\n");     
		POSITION posUpload = g_uploadFileList.GetHeadPosition();
		while (posUpload != NULL) {
			strFileName = g_uploadFileList.GetNext(posUpload);
			printf("upLoad [%s] \n", strFileName);   
			bool bret = ftp.UploadFile("E:/build Test1/" + strFileName, strFileName);  
			printf("%d\n\n\n", bret);  
		}


		ftp.DisConnect();  // 传输完成后断开连接

		Sleep(5000);	 // 延时5秒
	}

	return 0;
}

/**
 * @brief    -- ThreadFileRecord: 文件记录  记录间隔： 100ms , 500ms
 * @param[ ] -- lpParam
 * @return   -- 
 */

UINT ThreadFileRecord(LPVOID lpParam)
{
	/* 记录文件，整点记录， 记录频率 */
	CFile m_FileRecordMVBTraAllData;

	while(1) {

		printf("start thread 33 ...\n");
		CString newstr = _T("./Trdp.db"); // 文件记录 文件名


		if (!m_FileRecordMVBTraAllData.Open(newstr, CFile::modeCreate|CFile::shareDenyNone | CFile::modeWrite | CFile::typeBinary)) {
			printf("Create MVBTraAll Record  Failed!");
			printf("请检查文件夹!");
			isFileOpen = FALSE;
		} else {
			isFileOpen = TRUE;		
		}	

		WaitForSingleObject(hWriteDRU1, INFINITE);

		if (isFileOpen) {				
			m_FileRecordMVBTraAllData.Write(cData1701, LENGTH_1701);
			m_FileRecordMVBTraAllData.Flush();		
		}

		ReleaseMutex(hWriteDRU1);

		WaitForSingleObject(hWriteDRU2, INFINITE);

		if (isFileOpen) {				
			m_FileRecordMVBTraAllData.Write(cData1801, LENGTH_1801);
			m_FileRecordMVBTraAllData.Flush();		
		}

		ReleaseMutex(hWriteDRU2);



		Sleep(1000);	// 延时1秒
	}

	m_FileRecordMVBTraAllData.Close();
	isFileOpen = FALSE;					

	return 0;
}

#define MCASTADDR "239.194.0.0"
#define MCASTPORT  17224
#define BUFSIZE    112

int m_bIsCurrentETBNValid, m_nGETETBNThreaState;
int m_nCurrentETBNNO;

/**
 * @brief    --  ThreadTrdpDataProcess: TRDP 数据刷新  周期：10ms
 * @param[ ] -- lpParam
 * @return   -- 
 */
UINT ThreadTrdpDataProcess(LPVOID lpParam)
{

	static iCounterLife = 0;
	st_data_info dataRecord;  // mvb 初始化
	st_data_info dataComm;    // 初始化

	memcpy(dataComm.strType, "comm-record", strlen("comm-record"));
	memset(dataComm.data,  0, 20000);

	memcpy(dataRecord.strType, "tcn-bcn", strlen("tcn-bcn"));
	memset(dataRecord.data,  0, 20000); 

	unsigned char HeadPacket[22]  = {0}; // 组帧，初始化帧头。
	unsigned char trainid[4] = {2,3,4,5};
	unsigned char CarID = 0;
	unsigned char SwVer = 0;
	unsigned char CarInfID = 0;

	HeadPacket[0] = 0xAA;
	HeadPacket[1] = 0xAB;
	HeadPacket[2] = 0xAC;
	ULONG devid = DeviceID; 
	HeadPacket[3]  = (devid / 10000000) * 16 + (devid / 1000000) % 10;
	HeadPacket[4]  = ((devid / 100000) % 10) * 16 +(devid / 10000) % 10;
	HeadPacket[5]  = ((devid / 1000) % 10) * 16 + (devid / 100) % 10;
	HeadPacket[6]  = ((devid / 10) % 10) * 16 + (devid) % 10;
	HeadPacket[7]  = 0x00;   // version
	HeadPacket[8] = (trainid[2])/10 * 16 + trainid[2]%10;
	HeadPacket[9] = (trainid[3])/10 * 16 + trainid[3]%10;
	HeadPacket[10] = CarID / 10 * 16 + CarID % 10;
	HeadPacket[11] = 0x42;   // CarType
	HeadPacket[12] = CarInfID;
	HeadPacket[13] = 0;
	HeadPacket[14] = 0;
	HeadPacket[15] = SwVer;
	HeadPacket[16] = 0;
	HeadPacket[17] = 0;
	HeadPacket[18] = 0;
	HeadPacket[19] = 0;
	HeadPacket[20] = 0;
	HeadPacket[21] = 0;


	while (true) {
		if (g_iCycleCnt >= 65535) {
			g_iCycleCnt = 0;
		}
		printf("start thread 11 ...\n");

		/* 公共端口 {1701、1702}:刷新周期100ms， {1801、1802、1301、1302}:刷新周期500ms */
		for (int i=0; i < g_iTRDP_common_size; i++) {
			if ((g_iCycleCnt % (trdpport_comm[i].cycle / 10)) == 0) {  // 计算刷新周期
				if (trdpport_comm[i].direction == PUBLISH) {					

					iCounterLife++;
					if (iCounterLife >= 65536) {
						iCounterLife = 0;
					}

					st_data_info dataPut;
					m_shareMemory.ReadMvbDatatoTrdp(&dataPut);   // 读取共享内存中的mvb数据

					/* 取消前两个字节发送生命信号 */
					dataPut.data[0] = iCounterLife / 256;
					dataPut.data[1] = iCounterLife % 256;

					ret_val = tlp_put(session_handle, pub_handle[i], (DG_U8*)&dataPut.data, trdpport_record[i].portsize);  
					//printf("Put %x,return %d \r\n", trdpport[i].comid, ret_val);
					tlc_process(session_handle, NULL, NULL);

				} else if (trdpport_comm[i].direction == SUBSCRIBE) {

					stTrdpDataInfo  dataGet;
					dataGet.lPort = trdpport_comm[i].comid;
					dataGet.portSize = trdpport_comm[i].portsize;
					ret_val = tlp_get(session_handle, sub_handle[i], &pd_info[i], (DG_U8*)&dataGet.data, &trdpport_comm[i].portsize);
					//printf("Get %x,return %d \r\n", trdpport_comm[i].comid, ret_val);

					if (TRDP_NO_ERR == ret_val) {   // 收发数据无错误  -> 处理数据
						/* 1，获取TRDP数据 */
						/* cData1701 : (head){22} + (data){960 + 960} +  (fcs + end){1 + 3}  = 1946 (LENGTH_1701) */
						if (dataGet.lPort == 1701) {  // 1701 数据长度为 1000
							// unCounter dataLength;
							// memcpy(dataLength.cCounter, dataGet.data +24, 4); // 获取报文 应用内容的实际长度
							memcpy(cData1701 + 22, dataGet.data + 40, 960);  // trdp 包头：40个字节
						}
						if (dataGet.lPort == 1702) {  // 1702 数据长度为 1000  
							memcpy(cData1701+ 960 + 22, dataGet.data + 40, 960);
						}   

						/* cData1801 : (head){22} + (data){960 + 960 + 1360 + 1360} +  (fcs + end){1 + 3}  = 4666 (LENGTH_1801) */
						if (dataGet.lPort == 1801) { // 1801 数据长度为 1000  
							memcpy(cData1801 + 22, dataGet.data + 40, 960);
						}
						if (dataGet.lPort == 1802) { // 1802 数据长度为 1000  
							memcpy(cData1801 + 960 + 22, dataGet.data + 40, 960);
						}
						if (dataGet.lPort == 1301) { // 1301 数据长度为 1400  
							memcpy(cData1801 + 960 + 960 + 22, dataGet.data + 40, 1360);
						}
						if (dataGet.lPort == 1302) {// 1302 数据长度为 1400  
							memcpy(cData1801 + 1360 + 960 + 960 + 22, dataGet.data + 40, 1360);
						}

						/* 2，对 1701/1801 数据进行处理 。头、尾、校验 */
						/* 对数据进行处理，提取数据区，重新组帧 */
						/* 组帧：0xAA 0xAB [4]{carNumber} [2]{dataLength} [1]{version} [4]{frameNumber} [1]{dataType 0x42} [6]{utc时间 ms} [n]{data} [1]{FCS} 0xBA 0xBB */

						if ((dataGet.lPort == 1701) || (dataGet.lPort == 1702)) {  // 1701 
							unCounter frameNumber1701;

							frameNumber1701.iCounter = 0;  // 计数 初始化
							frameNumber1701.iCounter++;
							if (frameNumber1701.iCounter >= 0xffffffff) {
								frameNumber1701.iCounter = 0;
							}

							for (UINT i = 0; i < 13; i++) {
								cData1701[i] = HeadPacket[i];
							}

							cData1701[13] = (LENGTH_1701 - 26) % 256;
							cData1701[14] = (LENGTH_1701 - 26) / 256;
							cData1701[15] = HeadPacket[15];
							cData1701[16] = 0;
							cData1701[17] = frameNumber1701.cCounter[0];
							cData1701[18] = frameNumber1701.cCounter[1];
							cData1701[19] = frameNumber1701.cCounter[2];
							cData1701[20] = frameNumber1701.cCounter[3];
							cData1701[21] = 0x41;

							BYTE  FCS = 0;
							for (unsigned short iall = 3; iall < LENGTH_1701 - 4; iall++) {		
								FCS ^= cData1701[iall];
							}
							cData1701[LENGTH_1701 - 4] = FCS;

							memcpy(dataComm.data, cData1701, LENGTH_1701);
						}

						if ((dataGet.lPort == 1801) || (dataGet.lPort == 1802) || (dataGet.lPort == 1301) || (dataGet.lPort == 1302)) {  // 1801 
							unCounter frameNumber1801;

							frameNumber1801.iCounter = 0;  // 计数 初始化
							frameNumber1801.iCounter++;
							if (frameNumber1801.iCounter >= 0xffffffff) {
								frameNumber1801.iCounter = 0;
							}

							for (UINT i = 0; i < 13; i++) {
								cData1801[i] = HeadPacket[i];
							}

							cData1801[13] = (LENGTH_1801 - 26) % 256;
							cData1801[14] = (LENGTH_1801 - 26) / 256;
							cData1801[15] = HeadPacket[15];
							cData1801[16] = 0;
							cData1801[17] = frameNumber1801.cCounter[0];
							cData1801[18] = frameNumber1801.cCounter[1];
							cData1801[19] = frameNumber1801.cCounter[2];
							cData1801[20] = frameNumber1801.cCounter[3];
							cData1801[21] = 0x41;

							BYTE  FCS = 0;
							for (unsigned short iall = 3; iall < LENGTH_1801 - 4; iall++) {		
								FCS ^= cData1801[iall];
							}
							cData1801[LENGTH_1801 - 4] = FCS;

							memcpy(dataComm.data + 10000 , cData1801, LENGTH_1801);
						}

						/* 3，写入共享内存 */
						m_shareMemory.WriteTrdpDatatoMvb(&dataComm, 0);

					}
				}
			}
		}

		/* dru过程数据 */
		for (int j=0; j <  g_iTRDP_record_size; j++) {
			if ((g_iCycleCnt % (trdpport_record[j].cycle / 10)) == 0) {  // 计算刷新周期

				stTrdpDataInfo  dataGetRecord;
				dataGetRecord.lPort = trdpport_record[j].comid;
				dataGetRecord.portSize =  trdpport_record[j].portsize;

				ret_val = tlp_get(session_handle, sub_handle[j], &pd_info[j], (DG_U8*)&dataGetRecord.data, &trdpport_record[j].portsize);

				printf("Get %x,return %d \r\n",trdpport_record[j].comid, ret_val);

				if (TRDP_NO_ERR == ret_val) {   // 收发数据无错误  -> 处理数据
					for (int ii = 0; ii < g_iComTotalNum; ii++) {
						if (dataGetRecord.lPort == stCfgInfo[ii].comId) {
							for (int iter = 0; iter < stCfgInfo[ii].counter; ++iter) {
								dataRecord.data[stCfgInfo[ii].varInfo[iter].destNum] =
									dataGetRecord.data[stCfgInfo[ii].varInfo[iter].srcNum];  // 按配置文件要求，把相应的TRDP字节存储到mvbBuf
							}
						}
					}

					/*写入共享内存 */
					m_shareMemory.WriteTrdpDatatoMvb(&dataRecord, 1);

					tlc_process(session_handle, NULL, NULL);
				}
			}
		}


#if 0
		char *ip = "189.10.0.1";
		memcpy(data_get_record[0].strIp, ip, strlen(ip));
		data_get_record[0].Port = 1701;

		data_get_record[0].data[0] = 0xff;
		data_get_record[0].data[1] = 0xee;
		data_get_record[0].data[2] = 0xfd;
		data_get_record[0].data[3]++;
		data_get_record[0].data[5] = 0xad;

		//m_shareMemory.WriteTrdpDatatoMvb(&data_get_record[0], 0);
		//m_shareMemory.ReadMvbDatatoTrdp(&data_put_test[0], 0);

		CString strTmp100;
		for (int c=0; c<100; c++) {
			strTmp100.Format("%02X ", *(data_put_test[1].data + c));     // get data
			m_TRDPReceive += strTmp100;
		}

		UpdateData(FALSE);

		tlc_process(session_handle, NULL, NULL);
#endif

		g_iCycleCnt++;   // 频率：10ms
		Sleep(10);	     // 延时10毫秒
	}

	return 0;
}




/**
 * @brief    -- getPort  ：获取以太网端口参数 
 */
void CTRDPServerDlg::getPort() 
{	
	CString strFileName;
	CString path;

	LPTSTR path1 = new char[500]; 
	strcpy(path1, "C:\\config\\TRDP端口配置.xls");

	path = path1;
	path = "C:\\config\\TRDP端口配置.xls";
	path.Replace("\\", "\\\\");

	CSpreadSheet xls_comm(path, "公共以太网记录"); //sheet1  公共端口数据
	CSpreadSheet xls_record(path, "ECN_TCN记录端口协议"); //sheet2 记录端口数据

	CStringArray Rows;  // 定义工作表的行和列
	CStringArray Column; 
	int index = 0;

	if (index > xls_record.GetTotalRows())  {
		index = 1; //行从1开始，列从0开始
	}

	CString strtitle = "";

	unsigned address;
	int t[4];
	CString str;

	sscanf(str, "%x", t);
	address = t[0];
	strtitle = strtitle;

	m_ilen_record = xls_record.GetTotalRows();
	g_iTRDP_record_size = m_ilen_record - 1;   // 记录以太网 端口数量
	for (index = 2; index <= m_ilen_record; index++) {
		xls_record.ReadRow(Rows, index); // 逐行读取

		/* 解析每一行的内容 */
		str = Rows.GetAt(0); 
		if (str != "") {
			trdpport_record[index-2].trdpip = Rows.GetAt(0); //端口地址
		} else {
			//将故障信息写入日志文件
		}

		str = Rows.GetAt(1);
		if (str != "") {
			sscanf(str, "%d", t);
			trdpport_record[index-2].comid  = t[0];
		} else {
			//将故障信息写入日志文件
		}

		str = Rows.GetAt(2);
		if (str != "") {
			sscanf(str, "%d", t);
			trdpport_record[index-2].portsize = t[0];
		} else {
			//将故障信息写入日志文件
		}

		str = Rows.GetAt(3);
		if (str !="") {
			sscanf(str, "%d", t);
			trdpport_record[index-2].cycle = t[0];
		} else {
			//将故障信息写入日志文件
		}

		str = Rows.GetAt(4);
		if (str != "") {
			sscanf(str, "%d", t);
			trdpport_record[index-2].byteofsetstart = t[0];
		} else {
			//将故障信息写入日志文件
		}

		str = Rows.GetAt(5);
		if (str != "") {
			sscanf(str, "%d", t);
			trdpport_record[index-2].byteofsetend = t[0];
		} else {
			//将故障信息写入日志文件
		}

		str = Rows.GetAt(6);
		if (str != "") {
			sscanf(str,"%d",t);
			trdpport_record[index-2].datalen = t[0];
		} else {
			//将故障信息写入日志文件
		}

		str = Rows.GetAt(7);
		if (str != "") {

			CString strTmp = Rows.GetAt(7);   // 发布 与 订阅

			if (strTmp.CompareNoCase("subscribe") == 0) {  // 不区分大小写 比较字符
				trdpport_comm[index-2].direction =  SUBSCRIBE;
			} else if (strTmp.CompareNoCase("publish") == 0) {  // 不区分大小写 比较字符
				trdpport_comm[index-2].direction =  PUBLISH;
			} 
		} else {
			//将故障信息写入日志文件
		}
	}

	m_ilen_comm = xls_comm.GetTotalRows();
	g_iTRDP_common_size = m_ilen_comm - 1;   // 公共以太网 端口数量
	for (index=2; index <= m_ilen_comm; index++) {  
		xls_comm.ReadRow(Rows, index); // 逐行读取

		/* 解析每一行的内容 */
		str = Rows.GetAt(0); 
		if (str != "") {
			trdpport_comm[index-2].trdpip = Rows.GetAt(0); //端口地址
		} else {
			//将故障信息写入日志文件
		}

		str = Rows.GetAt(1);
		if (str != "") {
			sscanf(str, "%d", t);
			trdpport_comm[index-2].comid  = t[0];//Rows.GetAt(2);
		} else {
			//
		}

		str = Rows.GetAt(2);
		if (str !="") {
			sscanf(str, "%d", t);
			trdpport_comm[index-2].portsize = t[0];
		} else {
			//
		}

		str = Rows.GetAt(3);
		if (str !="") {
			sscanf(str, "%d", t);
			trdpport_comm[index-2].cycle = t[0];
		} else {
			//
		}

		str = Rows.GetAt(4);
		if (str != "") {
			sscanf(str,"%d",t);
			trdpport_comm[index-2].byteofsetstart = t[0];
		} else {
			//
		}

		str = Rows.GetAt(5);
		if (str != "") {
			sscanf(str,"%d",t);
			trdpport_comm[index-2].byteofsetend = t[0];
		} else {
			//
		}

		str = Rows.GetAt(6);
		if (str != "") {
			sscanf(str,"%d",t);
			trdpport_comm[index-2].datalen = t[0];
		} else {
			//
		}

		str = Rows.GetAt(7);
		if (str != "") {
			CString strTmp = Rows.GetAt(7);

			if (strTmp.CompareNoCase("subscribe") == 0) {  // 不区分大小写 比较字符
				trdpport_comm[index-2].direction =  SUBSCRIBE;
			} else if (strTmp.CompareNoCase("publish") == 0) {  // 不区分大小写 比较字符
				trdpport_comm[index-2].direction =  PUBLISH;
			} 

		} else {
		}
	}

	return;
}

/**
 * @brief    -- OnStartTRDP : 启动TRDP 
 */
void CTRDPServerDlg::OnStartTRDP() 
{
	// TODO: Add your control notification handler code here
	CString strFileName; 
	CString path;
	CString pathtemp;

	char sPath1[200] = "\0" ;     
	GetModuleFileName(NULL , sPath1 , 200 );  //得到当前全路径,包含文件名 

	//	GetCurrentDirectory(200 , sPath1) ;  
	path  = sPath1;
	int m = path.ReverseFind('\\');
	path  = path.Mid(0, m);

	//	pathtemp="\\设备参数.xls";

	pathtemp = "\\Setting\\trdp_tb_config.xml";

	path = path + pathtemp;	

	DG_S32 trdp_err_no = 0;
	DG_S32 ret_val = 0;

	recv_length = sizeof(recv_buffer);
	is_msg_received = 0;

	/* Read XML file */

	pd_Config.pfCbFunction  = NULL;
	pd_Config.pRefCon       = NULL;
	pd_Config.sendParam.qos = 5;
	pd_Config.sendParam.ttl = 64;
	pd_Config.flags         = TRDP_FLAGS_NONE;
	pd_Config.timeout       = 10000000;
	pd_Config.toBehavior    = TRDP_TO_SET_TO_ZERO;
	pd_Config.port          = 17224;

	trdpMem.p    = NULL;
	trdpMem.size = RESERVED_MEMORY;

	memset(trdpMem.prealloc, 0, sizeof(trdpMem.prealloc));
	ret_val = tlc_init(print_log, &trdpMem);

	if (TRDP_NO_ERR == ret_val)
		ret_val = tlc_openSession(&session_handle,
				vos_dottedIP("10.0.1.210"),       // 本机ip地址
				0,
				NULL,
				&pd_Config,
				NULL,
				&process_Config); /*use default ip*/

	if (TRDP_NO_ERR == ret_val) {
		ret_val = trdp_pd_demo(&trdp_err_no);
	}

	printf("Result: %d\n", trdp_err_no);
}

void CTRDPServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTRDPServerDlg::OnPaint() 
{
	if (IsIconic()) {
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	} else {
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTRDPServerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

/**
 * @brief    -- OnTimer   定时器
 * @param[ ] -- nIDEvent
 */
void CTRDPServerDlg::OnTimer(UINT nIDEvent) 
{ 
	// TODO: Add your message handler code here and/or call default
	switch (nIDEvent) 
	{
		case TRDPSENDTIMER:  // 10ms
			if (0 == getIP(strFtpIP))
				printf("get IP OK!!");

			break;
	}
	CDialog::OnTimer(nIDEvent);

}

/**
 * @brief    -- getIP 
 */
/**
 * @brief    -- getIP :获取对面列车编组号 ，计算出ip地址。 
 * @param[o] -- strIP :IP地址
 */
int CTRDPServerDlg::getIP(CString &strIP)  
{
	m_bIsCurrentETBNValid = 0;
	m_nCurrentETBNNO=0;

	WSADATA wsd;
	struct sockaddr_in local, remote, from;
	SOCKET sock, sockM;
	TCHAR recvbuf[BUFSIZE];
	int len = sizeof(struct sockaddr_in);
	int ret;
	unsigned int ComId = 0;
	CString str_ret;

	/* 初始化WinSock2.2 */
	printf("init winsocket ....\n");

	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		m_nGETETBNThreaState = 0;
		return -1;
	}

	if ((sock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, 
					WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		WSACleanup();
		m_nGETETBNThreaState = 0;
		return -1;
	}

	/* 将sock绑定到本机某端口上。 */
	local.sin_family      = AF_INET;
	local.sin_port        = htons(MCASTPORT);
	local.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		m_nGETETBNThreaState = 0;
		return -1;
	}
	printf("bind ok ....\n");


	/* 加入多播组 */
	remote.sin_family      = AF_INET;
	remote.sin_port        = htons(MCASTPORT);
	remote.sin_addr.s_addr = inet_addr(MCASTADDR);
	if ((sockM = WSAJoinLeaf(sock, (SOCKADDR*)&remote, sizeof(remote), NULL, NULL, NULL, NULL, JL_BOTH)) == INVALID_SOCKET) {
		closesocket(sock);
		WSACleanup();
		m_nGETETBNThreaState = 0;
		return -1;
	}
	printf("add ok ....\n");

	/* 接收多播数据，当接收到的数据为第8，9字节收到100时退出。*/
	m_nGETETBNThreaState = 1;

	while (1) {
		if (m_nGETETBNThreaState == 2) {
			break;
		} else if (m_nGETETBNThreaState == 1) {
			;
		}

		printf("-------\n");
		if ((ret = recvfrom(sock, recvbuf, BUFSIZE, 0, (struct sockaddr*)&from, &len)) == SOCKET_ERROR) {
			closesocket(sockM);
			closesocket(sock);
			WSACleanup();
			m_nGETETBNThreaState=0;
			printf("SOCKET_ERROR!");
			return -1;
		} else {
			CString str0, str1;
			str0 = "";
			str1 = "";

			for (UINT i=0; i < BUFSIZE; i++) {
				str0.Format("%02x-", recvbuf[i]);
				str1 += str0;
			}

			printf("%s\n", str1);
		}

		ComId = (recvbuf[8] << 24) + (recvbuf[9] << 16) + (recvbuf[10] << 8) + recvbuf[11];

		if (ComId == 100) {
			ret = recvbuf[92];
			str_ret.Format("Current_ETBN %d", ret);
			m_bIsCurrentETBNValid = 1;
			m_nCurrentETBNNO      = ret;
			printf("%s\n", str_ret);

			CString strIp = "10.0.0.1";    // 根据编组号，计算出IP地址

		}
		break;

		Sleep(1000);
	}

	closesocket(sockM);
	closesocket(sock);
	WSACleanup();

	if (m_nGETETBNThreaState == 2) {
		printf("检测ETBN工作因重复请求被停止，可重新发起请求！");
	} else {
		printf("已获取本地编组号，请选择要连接设备所在的编组号");
	}
	m_nGETETBNThreaState = 0;

	return 0;
}

CFTPFunc::CFTPFunc()
{
	IsConnectToFtpServer = false;
	m_pInetSession       = NULL;
	m_pFtpConnection     = NULL;
}

CFTPFunc::~CFTPFunc()
{
	if (m_pFtpConnection != NULL) {
		m_pFtpConnection->Close();
		m_pFtpConnection = NULL;
	}
	m_pInetSession = NULL;
}

/**
 * @brief    -- ConnectToServer  : 连接ftp服务器
 * @param[i] -- ServerIP         : IP地址
 * @param[i] -- name             : 用户名
 * @param[i] -- Password         : 密码
 * @param[i] -- port             : 端口
 * @return   -- 
 */
BOOL CFTPFunc::ConnectToServer(LPCTSTR ServerIP, LPCTSTR name, LPCTSTR Password, INTERNET_PORT port)
{
	if (m_pInetSession == NULL) {
		m_pInetSession = new CInternetSession(AfxGetAppName(), 1, PRE_CONFIG_INTERNET_ACCESS);
	}

	try { 
		if (m_pFtpConnection == NULL) {
			m_pFtpConnection = m_pInetSession->GetFtpConnection(ServerIP, name, Password, port);
		} else {
			return true;
		}
	} catch(CInternetException *pEx) {
		TCHAR szError[1024]; //获取错误

		if (pEx->GetErrorMessage(szError, 1024)) {
			printf("%s", szError);
		} else {
			printf("There was an exception!");
		}

		pEx->Delete();
		m_pFtpConnection = NULL;
		return false;
	}

	printf("Connect DRUFTP OK \n");
	return true;
}

/**
 * @brief    -- UploadFile    : 上传文件， 同步
 * @param[i] -- strFileName   : 本地文件名
 * @param[i] -- strRemoteFile : 服务器中的文件名
 * @return   -- 
 */
bool CFTPFunc::UploadFile(CString strFileName, CString strRemoteFile)
{
	if(m_pFtpConnection == NULL) {
		printf("m_pFtpConnection NULL!!");
	}

	try {
		m_pFtpConnection->PutFile(strFileName, strRemoteFile);
		printf("upload %s success !", strFileName);
		return true;
	} catch (CInternetException *pEx) {
		TCHAR szError[1024];	

		if(pEx->GetErrorMessage(szError,1024)) {
			printf("upload ERROR:%s !", szError);
		} else {
			printf("{upload} There was an exception !");
		}

		pEx->Delete();
		m_pFtpConnection = NULL;	

		printf("upload [%s] failed !", strFileName);
		return false;
	}
}

/**
 * @brief    -- removeFile     : 删除服务器中的文件
 * @param[ ] -- strRemoteFile  : 文件名
 * @return   -- 
 */
bool CFTPFunc::removeFile(CString strRemoteFile)
{
	if(m_pFtpConnection == NULL) {
		printf("m_pFtpConnection NULL!!");
	}

	try {
		m_pFtpConnection->Remove(strRemoteFile);
		printf("remove %s success !", strRemoteFile);
		return true;
	} catch (CInternetException *pEx) {
		TCHAR szError[1024];	

		if(pEx->GetErrorMessage(szError,1024)) {
			printf("remove ERROR:%s !", szError);
		} else {
			printf("{remove} There was an exception !");
		}

		pEx->Delete();
		m_pFtpConnection = NULL;	

		printf("upload [%s] failed !", strRemoteFile);
		return false;
	}
}

void CFTPFunc::DisConnect()
{
	if (m_pFtpConnection != NULL) {
		m_pFtpConnection->Close();
		m_pFtpConnection = NULL;
	}
	m_pInetSession = NULL;
}


/**
 * @brief    -- findFileFTPServer : 遍历 ftp服务器 文件夹
 * @param[o] -- mapFileServer     : 存储文件信息：文件名和文件大小。
 * @param[ ] -- strDir            : 目录
 */
void CFTPFunc::findFileFTPServer(mapFileInfo &mapFileServer, CString strDir) 
{
	CFileFind finder; 
	CString filePath;  // 文件路径
	CString fileName;  // 文件名
	CString strVal;

	CFtpFileFind fileFtpFind(m_pFtpConnection);
	printf(" FTP Server start!! \n");

	mapFileServer.RemoveAll();
	BOOL isNotEmpty =  fileFtpFind.FindFile(strDir + _T("//*.*")); // 总文件夹，开始遍历
	while (isNotEmpty) { 
		isNotEmpty =  fileFtpFind.FindNextFile(); // 查找文件
		filePath   =  fileFtpFind.GetFilePath();  // 获取文件的路径，可能是文件夹，可能是文件
		fileName   =  fileFtpFind.GetFileName();  // 获取文件名

		printf(" FTP Server find name:%s!! \n", fileName);
		if (!(fileFtpFind.IsDirectory())) {               // 如果是文件则加入文件列表
			strVal.Format("%d", fileFtpFind.GetLength()); 
			mapFileServer.SetAt(fileName, strVal);    // 把遍历到的文件插入到map，key:文件名， value:文件大小
		} else { // 递归遍历用户文件夹，跳过非用户文件夹
			if(!(fileFtpFind.IsDots() || fileFtpFind.IsHidden() || fileFtpFind.IsSystem() ||
						fileFtpFind.IsTemporary() || fileFtpFind.IsReadOnly())) { 
				findFileFTPServer(mapFileServer, fileName + _T("/")); 
			} 
		} 
	}

	/* 遍历map中的数据 */
	CString strKey = _T(""), str = _T("");
	POSITION pos =  mapFileServer.GetStartPosition();

	while (pos) {
		mapFileServer.GetNextAssoc(pos, strKey, str);
		printf(" FTPFileName:%s, size:%s\n", strKey, str );
	}
}

/**
 * @brief    -- findFileLocal  : 查找本地文件
 * @param[o] -- mapFileLocal   ：存储本地文件信息
 * @param[i] -- strDir         : 文件路径
 */
void  CFTPFunc::findFileLocal(mapFileInfo &mapFileLocal, CString strDir) 
{ 
	CFileFind finder; 
	CString filePath;  // 文件路径
	CString fileName;  // 文件名
	CString strVal;

	mapFileLocal.RemoveAll();
	BOOL isNotEmpty = finder.FindFile(strDir + _T("//*.*")); // 总文件夹，开始遍历
	while (isNotEmpty) { 
		isNotEmpty = finder.FindNextFile(); // 查找文件
		filePath   = finder.GetFilePath();  // 获取文件的路径，可能是文件夹，可能是文件
		fileName   = finder.GetFileName();  // 获取文件名

		if (!(finder.IsDirectory())) {               // 如果是文件则加入文件列表
			strVal.Format("%d", finder.GetLength()); 
			mapFileLocal.SetAt(fileName, strVal);    // 把遍历到的文件插入到map，key:文件名， value:文件大小
		} else { // 递归遍历用户文件夹，跳过非用户文件夹
			if(!(finder.IsDots() || finder.IsHidden() || finder.IsSystem() ||
						finder.IsTemporary() || finder.IsReadOnly())) { 
				findFileLocal(mapFileLocal, fileName + _T("/")); 
			} 
		} 
	} 

	/* 遍历map中的数据 */
	CString strKey = _T(""), str = _T("");
	POSITION pos = mapFileLocal.GetStartPosition();

	while (pos) {
		mapFileLocal.GetNextAssoc(pos, strKey, str);
		printf("localFileName:%s,\n size:%s\n", strKey, str );
	}
}

/**
 * @brief    -- checkFileWaitUpload 
 * @param[ ] -- listRemoveFile
 * @return   -- 
 */
bool CFTPFunc::checkFileWaitRemove(listFileInfo &listRemoveFile)
{
	listRemoveFile.RemoveAll();

	CString strFileSizeServer = _T("");
	CString strFileSizeLocal  = _T("");

	/* 遍历map中的数据 */
	CString strKeyLocal  = _T(""), strValLocal  = _T(""); // 本地   文件名、文件大小
	CString strKeyServer = _T(""), strValServer = _T(""); // 服务端 文件名、文件大小

	printf(" local number:%d,\n server Number:%d\n", g_mapFileLocal.GetCount(), g_mapFileServer.GetCount());
	if (g_mapFileLocal.IsEmpty()) {  // 本地文件目录为空 
		return false;
	} else {

		POSITION posServer = g_mapFileServer.GetStartPosition();
		while (posServer) {   // 遍历 服务器中的文件
			g_mapFileServer.GetNextAssoc(posServer, strKeyServer, strValServer); 

			if (!g_mapFileLocal.Lookup(strKeyServer, strFileSizeLocal)) { // 如果本地文件中 不存在 服务器中的文件
				listRemoveFile.AddTail(strKeyServer);
			}
		}
	}
	return true;
}

/**
 * @brief    -- checkFileWaitUpload 
 * @return   -- listUploadFile : 等待上传的文件
 */
bool CFTPFunc::checkFileWaitUpload(listFileInfo &listUploadFile)
{
	listUploadFile.RemoveAll();

	CString strFileSizeServer = _T("");
	CString strFileSizeLocal  = _T("");

	/* 遍历map中的数据 */
	CString strKeyLocal  = _T(""), strValLocal  = _T(""); // 本地   文件名、文件大小
	CString strKeyServer = _T(""), strValServer = _T(""); // 服务端 文件名、文件大小

	printf(" local number:%d,\n server Number:%d\n", g_mapFileLocal.GetCount(), g_mapFileServer.GetCount());
	if (g_mapFileLocal.IsEmpty()) {  // 本地文件目录为空 
		return false;
	} else {
		POSITION posLocal  = g_mapFileLocal.GetStartPosition();
		while (posLocal) {   // 遍历 本地文件
			g_mapFileLocal.GetNextAssoc(posLocal, strKeyLocal, strValLocal); 
			printf(" key:%s,\n value:%s\n", strKeyLocal, strValLocal);

			if (g_mapFileServer.Lookup(strKeyLocal, strFileSizeServer)) { // 如果服务器中已存在的文件 与本地文件名称一样
				g_mapFileLocal.Lookup(strKeyLocal, strFileSizeLocal);
				if (strFileSizeLocal !=  strFileSizeServer) { // 如果文件大小不一致，重新上传文件
					listUploadFile.AddTail(strKeyLocal);
				}
			} else {   // 服务器中不存在的文件 ， 加入上传列表
				listUploadFile.AddTail(strKeyLocal);
			}
		}
	}
	return true;
}

/**
 * @brief    -- getConfig  ：获取TRDP变量 与MVB变量 的映射关系 配置
 */
void CTRDPServerDlg::getConfig() 
{	

#if 0  // 写入文件 调试 
	CFile file; //定义文件变量
	CString filename = (".//variableTest.log");

	if (file.Open(filename, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyRead)) { 
		file.SeekToBegin(); //到达文件开头 
	}
#endif 

	CString strFileName;
	CString path = "C:\\config\\DRU表格汇总20181121.xls";
	CSpreadSheet xls_comm(path, "Sheet1"); //sheet1  公共端口数据

	CStringArray Rows;  // 定义工作表的行和列
	CStringArray Column; 
	int index = 0;

	if (index > xls_comm.GetTotalRows())  {
		index = 1;  //行从1开始，列从0开始
	}

	CString str;
	int i;
	int t;              // 临时变量
	int ans = 0;        // 端口数量     端口索引
	int iNumIndex = 0;  // 端口中的变量 字索引
	int iBitPos = 0;    // 变量的       位索引
	int comidTmp[300] = {0};  // 端口数量 
	int wordTmp[100] = {0};

	m_ilen_comm = xls_comm.GetTotalRows();
	for (index=3,  i = 0; index <= m_ilen_comm; index++, ++i) {  
		xls_comm.ReadRow(Rows, index); // 逐行读取

		/* 解析每一行的内容 */
		str = Rows.GetAt(24);   // COMID
		if (str != "") {
			sscanf(str, "%d", &t);
			comidTmp[i] = t;

			if (i >= 1) {
				if (comidTmp[i-1] != t) {    // 查找不同 的 Com ID 
					ans++;
					iNumIndex = 0;
				}
				stCfgInfo[ans].comId = t;
			} 

		} else {
			continue;
		}

		str = Rows.GetAt(20);  // WORD   查找不同的字节
		if (str != "") {
			sscanf(str, "%d", &t);

			if (wordTmp[iBitPos - 1] != t) {
				++iNumIndex;
				iBitPos = 0;
			}

			wordTmp[iBitPos] = t;
			stCfgInfo[ans].varInfo[iNumIndex].srcNum = t;

		} 

		str = Rows.GetAt(25);
		if (str != "") {
			sscanf(str, "%d", &t);
			stCfgInfo[ans].varInfo[iNumIndex].destNum = t;
		} 

		str = Rows.GetAt(21);
		if (str != "") {    // BIT
			sscanf(str, "%d", &t);
			stCfgInfo[ans].varInfo[iNumIndex].srcPos[iBitPos].bit = t;
		}

		str = Rows.GetAt(6);
		if (str != "") {
			stCfgInfo[ans].varInfo[iNumIndex].srcPos[iBitPos].varType = str;
		} else {
			stCfgInfo[ans].varInfo[iNumIndex].srcPos[iBitPos].varType = "null";
		}

		str = Rows.GetAt(22);
		if (str != "") {
			sscanf(str, "%d", &t);
			stCfgInfo[ans].portSize = t;
		}

		iBitPos++;
		stCfgInfo[ans].varInfo[iNumIndex].bitNum = iBitPos;   // 每个字节 包含的待处理的 位数 
		stCfgInfo[ans].counter = iNumIndex + 1;
		g_iComTotalNum = ans + 1 ;
	}

#if 0   // 写入文件 调试 
	CString  strEditTmp;

	for (int ii = 0; ii < g_iComTotalNum; ++ii) {
		for (int iter = 0; iter < stCfgInfo[ii].counter; ++iter) {
			for (int it = 0; it < stCfgInfo[ii].varInfo[iter].bitNum; ++it) {
				strEditTmp.Format("COMID[%d] PORTSIZE[%d] COUNTER[%d] VAR[%s] WORD[%d]:%d{%d} -> DEST[%d] \r\n", 
						stCfgInfo[ii].comId,
						stCfgInfo[ii].portSize,
						stCfgInfo[ii].counter,
						stCfgInfo[ii].varInfo[iter].srcPos[it].varType,
						stCfgInfo[ii].varInfo[iter].srcNum,
						stCfgInfo[ii].varInfo[iter].srcPos[it].bit,
						stCfgInfo[ii].varInfo[iter].bitNum,
						stCfgInfo[ii].varInfo[iter].destNum);

				file.Write(strEditTmp, strEditTmp.GetLength());     // 写入实际数据 
			}
		}
	}

	file.Close();
#endif 

	return;
}


