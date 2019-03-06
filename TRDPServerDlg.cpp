// FTPServerDlg.cpp : implementation file
#include <string.h>
#include <windows.h>
#include "StdAfx.h"
#include "TRDPServerDlg.h"
#include "stdio.h"
#include "TRDPServer.h"
#include "ListenSocket.h"	// Added by ClassView
#include "CSpreadSheet.h"
#include "winsock.h"
#include "stdlib.h"
#include "shlwapi.h"

#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"ws2_32.lib")

/* 调试窗口 */
#define DEBUG_PRINTF  // 添加调试控制窗口，程序发布时，要屏蔽掉
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

#define GET_BIT(x, y)   ((x) >> (y)&1)  // 获取 字节 x 的 y位的值。


#define RESERVED_MEMORY  660000  // 内存大小
#define LENGTH_1701      3866 
#define LENGTH_1801      9306
#define TRDPSENDTIMER    10
#define TRDP_PORT_NUMBER 90
#define FAULT_MAX_SIZE   3500   // 故障缓存大小
#define DRUSTOREDIR2    "D:\\WTD_INFO\\以太网数据"
// #define REALDATASTOREDIR  "D:\\WTD_INFO\\实时数据"

enum TRDPPortEnum{
	PUBLISH = 0,
	SUBSCRIBE
} trdpdir;

static Csharememoryclient m_shareMemory;  // 共享内存

unsigned char cData1701[8000] = {0}; // 1701 1702公共端口，记录内容，记录周期100ms，数据刷新应小于100ms，每小时整点生成一个文件，文件后缀为edf ，数据类型为0x42
unsigned char cData1801[10000] = {0}; // 1801 1802 1301 1302 记录内容，记录周期1s，数据刷新周期500ms，每小时整点生成一个文件，文件后缀为eds，数据类型为0x43。
unsigned char cDataFault1C1[2000] = {0}; // 一二级故障数据。
unsigned char cDataFault1C2[2000] = {0}; // 一二级故障数据。
unsigned char cDataFault1C3[2000] = {0}; // 一二级故障数据。
unsigned char cDataFault1C4[2000] = {0}; // 一二级故障数据。
unsigned char cDataFault3C1[2000] = {0}; // 三级故障数据。
unsigned char cDataFault3C2[2000] = {0}; // 三级故障数据。
unsigned char cDataFault3C3[2000] = {0}; // 三级故障数据。
unsigned char cDataFault3C4[2000] = {0}; // 三级故障数据。
ULONG DeviceID = 0;

/* 存放 TRDP数据。 */
// stTrdpDataInfo data_put_test[10];
// stTrdpDataInfo data_get_record[80]; 

CString strFtpIP;   // ftp 服务器ip地址
CString strLocalIP; // local ip地址

int g_iCtlType       = 0; // 控车模式   ：1：trdp控车、 0： mvb控车
int g_iCtlModeOffset = 0; // 控车模式 判断值的字节偏移
int g_iCtlModeComID  = 0; // 控车模式 接收数据的端口号
int g_iCtlModeValue  = 0; // 控车模式 判断值
int g_iIsTRDP        = 0; // 判断是否为TRDP控车  兼容长客与四方股份
int g_iETBN          = 0xaa; // 编组号

unsigned char g_TrainId[4] = {0};

bool    isFileOpen1701 = FALSE; // 文件是否被打开
bool    isFileOpen1801 = FALSE; // 文件是否被打开
int     g_iCycleCnt = 0;        // trdp刷新时间计数，以10ms定时器为基准，适应其他周期的数据

unsigned short g_iComTotalNum = 0; // 要记录变量的端口数量
unsigned int  g_iTRDP_common_size; // 公共以太网 端口数量
unsigned int  g_iTRDP_record_size; // 记录以太网 端口数量

mapFileInfo  g_mapFileLocal;       // 存储本地文件信息：文件名, 文件大小
mapFileInfo  g_mapCompressFile;    // 存储本地文件信息：文件名, 文件大小

mapFileInfo  g_mapFileServer;      // 存储ftp服务端文件信息：文件名，文件大小
listFileInfo g_uploadFileList;     // 待同步 文件名
listFileInfo g_removeFileList;     // 待删除 文件名

stVarInfo g_stVarFault1[FAULT_MAX_SIZE];     // trdp 目标偏移以及 端口字节偏移、位偏移  一二级故障 信息
stVarInfo g_stVarFault3[FAULT_MAX_SIZE];     // trdp 目标偏移以及 端口字节偏移、位偏移　三级故障 信息
int g_iVarFault1Length;            // 一二级故障个数
int g_iVarFault3Length;            // 三级故障个数

typedef struct _PUT_DATA {
	DG_U8 data[1500];
} stPutData;
 stPutData putData[10];

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
TRDP_PUB_T      pub_handle_comm[15];    // 发布 句柄
TRDP_PUB_T      pub_handle_record[15];  // 发布 句柄
TRDP_SUB_T      sub_handle_comm[80];    // 订阅 句柄
TRDP_SUB_T      sub_handle_record[80];  // 订阅 句柄

stTrdpPortPara trdpport_comm[15];         // 公共端口 配置
stTrdpPortPara trdpport_record[80];       // 记录端口 配置
stConfigInfo stCfgInfo[TRDP_PORT_NUMBER]; // 端口属性 设置

HANDLE  hWriteDRU1;  // 互斥量
HANDLE  hWriteDRU2;  // 互斥量
int size;

/* ==================================================================================
 * Local function declarations
 * ================================================================================*/
static void print_log(void *pRefCon, VOS_LOG_T category, const CHAR8 *pTime, const CHAR8 *pFile, UINT16 line, const CHAR8 *pMsgStr);
static void wait_for_msg();
static DG_S32 trdp_pd_config();
static void md_callback(void *ref, TRDP_APP_SESSION_T apph, const TRDP_MD_INFO_T *msg, UINT8 *data, UINT32 size);
static BOOL NOPSAVFTPIPSet(CString strsourceIP, BYTE bianzuNo, CString &tarstrIP);
static void compressExport(CString strRecordFile);
BOOL ExecCommand(CString strCmdLine);

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

/**
 * @brief    -- trdp_pd_config : trdp 端口配置， 发布与订阅
 * @return   -- 
 */
DG_S32 CTRDPServerDlg::trdp_pd_config()
{
	for (int i = 0; i < g_iTRDP_record_size; i++) {
		/* publish and subscribe telegrams */
		if ( trdpport_record[i].direction == PUBLISH) {  // 发送 {发布} 
			ret_val = tlp_publish(session_handle,
					&pub_handle_record[i],
					trdpport_record[i].comid,
					0,
					0,
					0,
					vos_dottedIP(trdpport_record[i].trdpip),
					trdpport_record[i].cycle * 1000,
					0,
					TRDP_FLAGS_NONE,
					NULL,
					NULL,
					trdpport_record[i].portsize);

			printf("-PUBLISH---record----------------return: %d  pos:%d \n", ret_val, i);
		} else if (trdpport_record[i].direction == SUBSCRIBE) { // 接收 {订阅} 
			ret_val = tlp_subscribe(session_handle,
					&sub_handle_record[i],
					NULL,
					NULL,
					trdpport_record[i].comid,
					0,
					0,
					0, 
					vos_dottedIP(trdpport_record[i].trdpip),
					TRDP_FLAGS_NONE,
					trdpport_record[i].cycle * 1000,
					TRDP_TO_SET_TO_ZERO);
			printf("-SUBSCRIBE---record--------------return: %d  pos:%d \n", ret_val, i);
		}	
	}

	for (int j = 0; j < g_iTRDP_common_size; j++) {
		/* publish and subscribe telegrams */
		if (trdpport_comm[j].direction == PUBLISH) {  // 发送 {发布} 
			ret_val = tlp_publish(session_handle, 
					&pub_handle_comm[j],
					trdpport_comm[j].comid,
					0,
					0,
					0,
					vos_dottedIP(trdpport_comm[j].trdpip),
					trdpport_comm[j].cycle * 1000,
					0,
					TRDP_FLAGS_NONE,
					NULL,
					NULL,
					trdpport_comm[j].portsize);
			printf("-PUBLISH---comm----------------return: %d  pos:%d \n", ret_val, j);
		} else if (trdpport_comm[j].direction == SUBSCRIBE) { // 接收 {订阅} 
			ret_val = tlp_subscribe(session_handle, 
					&sub_handle_comm[j],
					NULL,
					NULL,
					trdpport_comm[j].comid,
					0,
					0,
					0, 
					vos_dottedIP(trdpport_comm[j].trdpip),
					TRDP_FLAGS_NONE,
					trdpport_comm[j].cycle * 1000,
					TRDP_TO_SET_TO_ZERO);
			printf("-SUBSCRIBE---comm--------------return: %d  pos:%d \n", ret_val, j);
		}	
	}

#if 0
	ret_val = tlp_publish(session_handle,   /*  our application identifier             */
			&pub_handle_comm[0],                /*  our pulication identifier              */
			3110,                              /*  comId to send                          */
			0,                                  /*  etbTopoCnt = 0 for local consist only  */
			0,                                  /*  opTopoCnt = 0 for non-directinal data  */
			0,//vos_dottedIP("172.31.41.202"),  /*  default source IP                      */
			vos_dottedIP("239.255.21.10"),      /*  where to send to                       */
			200000,                          /*  cycleTime in us                        */
			0,                                  /*  not redundant                          */
			TRDP_FLAGS_NONE,                    /*  no flag                                */
			NULL,                               /*  default qos and tll                    */
			NULL,                               /*  sendData                               */
			100);                              /*  data size */ 
#endif

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

	// m_nTestType = 100;
	// m_nTestPlace = 0;
	// m_nTestValue = 0;

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
	//DDX_Control(pDX, IDC_EDIT_TYPE, m_nTestType);
	//DDX_Control(pDX, IDC_EDIT_PLACE, m_nTestPlace);
	//DDX_Control(pDX, IDC_EDIT_VALUE, m_nTestValue);
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
	m_ilen_comm = 1;   // 公共端口 的个数
	m_ilen_record = 1; // 记录端口数据的个数
	g_iVarFault1Length = 0;
	g_iVarFault3Length = 0;
	m_mapFault1Info.RemoveAll();  // 一二级故障 trdp 目标偏移，与 变量名
	m_mapFault3Info.RemoveAll();   // 三级故障 trdp 目标偏移，与 变量名
 
	memset(&g_stVarFault1, 0, sizeof(g_stVarFault1));  // clear
	memset(&g_stVarFault3, 0, sizeof(g_stVarFault3));  // clear

	getFaultConfig(m_mapVarInfo1, "fault_1301.xls");
	getFaultConfig(m_mapFault1Info, "fault12.xls", 1);
	g_iVarFault1Length = getVarInfo(g_stVarFault1, m_mapFault1Info, m_mapVarInfo1);
	printf("g_iVarFault1Length : %d \r\n", g_iVarFault1Length);


	getFaultConfig(m_mapVarInfo3, "fault_1801.xls");
	getFaultConfig(m_mapFault3Info, "fault3.xls", 3);
	g_iVarFault3Length = getVarInfo(g_stVarFault3, m_mapFault3Info, m_mapVarInfo3);
	printf("g_iVarFault3Length : %d \r\n", g_iVarFault3Length);

#if 0
    // 写入文件 调试 
	CFile file; //定义文件变量
	CString filename = (".//variableTest_dest1.log");

	if (file.Open(filename, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyRead)) { 
		file.SeekToBegin(); //到达文件开头 
	}

	CString  strEditTmp;
	for (int ii=0; ii<g_iVarFault1Length; ii++) {
		strEditTmp.Format("dest: %d, word:%d, offset:%d\r\n", g_stVarFault1[ii].iDestNum,
				g_stVarFault1[ii].stOffset.iWordOffset, g_stVarFault1[ii].stOffset.iBitOffset);

		file.Write(strEditTmp, strEditTmp.GetLength());     // 写入实际数据 
	}
	file.Close();

    // 写入文件 调试 
	CString filename3 = (".//variableTest_dest3.log");

	if (file.Open(filename3, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyRead)) { 
		file.SeekToBegin(); //到达文件开头 
	}

	for (int i=0; i<g_iVarFault3Length; i++) {
		strEditTmp.Format("dest: %d, word:%d, offset:%d\r\n", g_stVarFault3[i].iDestNum,
				g_stVarFault3[i].stOffset.iWordOffset, g_stVarFault3[i].stOffset.iBitOffset);

		file.Write(strEditTmp, strEditTmp.GetLength());     // 写入实际数据 
	}
	file.Close();
#endif 

    getConfig();    // 获取mvb与trdp 端口变量映射关系
    getPort();      // 获取trdp配置相关信息
    printf("g_iCtlType: %d, g_iCtlModeOffset: %d, g_iCtlModeComID: %d, g_iCtlModeValue: %d, g_iIsTRDP: %d \r\n", 
            g_iCtlType, g_iCtlModeOffset, g_iCtlModeComID, g_iCtlModeValue, g_iIsTRDP);

	OnStartTRDP() ; // 启动TRDP
	m_strInfo = "TRDP服务器启动！";

	printf("TRDP服务器启动！");

	m_status.SetWindowText(m_strInfo);

	GetDlgItem(IDC_EDIT_TYPE)->SetWindowText("99"); 
	GetDlgItem(IDC_EDIT_PLACE)->SetWindowText("99");
	GetDlgItem(IDC_EDIT_VALUE)->SetWindowText("99");
	Sleep(3000);	// 延时3秒

	SetTimer(TRDPSENDTIMER, 10000, NULL);  // 设置定时器 1000ms

	/* 启动线程 */
	AfxBeginThread(ThreadTrdpDataProcess, NULL);
	AfxBeginThread(ThreadFileSync, NULL);
	// AfxBeginThread(ThreadFileRecord, NULL);
	AfxBeginThread(ThreadFileRecord, 0, THREAD_PRIORITY_HIGHEST);


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
 * @brief    -- ThreadFileSync: FTP文件同步 ， 每10秒同步  
 * @param[ ] -- lpParam
 * @return   -- 
 */
UINT ThreadFileSync(LPVOID lpParam)
{
	printf("start---{{ThreadFileSync}} ...\n");

	CFTPFunc ftp;
	while(1) {
		/* 文件压缩 */
		ftp.findFileLocal(g_mapCompressFile, DRUSTOREDIR2);  // 获取本地文件列表

		/* 遍历map中的数据 */
		CString strKey = _T(""), str = _T(""), strTarName = _T("");
		POSITION pos = g_mapCompressFile.GetStartPosition();
		CString strTimeType = _T("");
		unsigned long int lTimeType = 0;

		CString date = CTime::GetCurrentTime().Format("%m%d%H%M%S");
		unsigned long int lDate = (unsigned long int)_ttoi64(date);

		while (pos) {
			g_mapCompressFile.GetNextAssoc(pos, strKey, str);

			if (strKey.Right(3) == "edf" || strKey.Right(3) == "eds") {
				// g_mapCompressFile.erase(strKey);
				CString strTmp = strKey.Right(14);
				strTimeType = strTmp.Left(10); 
				lTimeType = (unsigned long int)_ttoi64(strTimeType);

				if (lDate - lTimeType > 10000) {   // 获取 1个小时前 未压缩的文件名
					strTarName = strKey;
					break;
				}
			} 
		}

		// printf("get compress name :%s \r\n", strTarName);
		compressExport(strTarName); // 文件压缩
		strTarName = _T("");

		/* 文件同步 */
		if (false == ftp.ConnectToServer("127.0.0.1", "liuwming", "12345678", 21)) { // 连接FTP服务器  
			printf("No Connected!!");
			continue;
		}

		ftp.findFileLocal(g_mapFileLocal, DRUSTOREDIR2);  // 获取本地文件列表
		ftp.findFileFTPServer(g_mapFileServer, ".");      // 获取服务器文件列表

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

		//printf("\n\n");     
		CString strFileName;
		POSITION posRemove = g_removeFileList.GetHeadPosition();
		while (posRemove != NULL) {
			strFileName =  g_removeFileList.GetNext(posRemove);
			//printf("remove [%s] \n", strFileName);   
			bool bret = ftp.removeFile("./" + strFileName);  
			//printf("%d\n\n\n", bret);  
		}

		//printf("\n\n");     
		POSITION posUpload = g_uploadFileList.GetHeadPosition();
		while (posUpload != NULL) {
			strFileName = g_uploadFileList.GetNext(posUpload);
			//printf("upLoad [%s] \n", strFileName);   
			bool bret = ftp.UploadFile("E:/build Test1/" + strFileName, strFileName);  
			//printf("%d\n\n\n", bret);  
		}

		ftp.DisConnect();  // 传输完成后断开连接

		Sleep(100000);	 
	}

	return 0;
}


/**
 * @brief    -- ThreadFileRecord: 文件记录  
 * @param[ ] -- lpParam
 * @return   -- 
 */
UINT ThreadFileRecord(LPVOID lpParam)
{
	printf("start {{ThreadFileRecord}} ...\n");

	/* 记录文件，整点记录， 记录频率 :记录间隔： 100ms , 1s */
	/* 每小时生成一个文件；后缀分别为:edf、eds. */

	/* 文件“000207_01_20160724000000.sbr”
	 * 表示000207列动车组上 01 车安装的WTD设备于北京时间2016年7月24日00时00分00秒开始记录的二进制数据（实时状态数据）*/

	bool bFirstStart = true; // 第一次启动 

	CFile FileRecordMVBTraAllData1701;
	CFile FileRecordMVBTraAllData1801;
	int iCounter = 0;
	CTime time;
	CString newFile1801;
	CString newFile1701;
	unsigned char CarID = 1;

	time = CTime::GetCurrentTime();
	CString date = time.Format("%Y%m%d%H%M%S");

	CString csPath(DRUSTOREDIR2);
	if (!PathIsDirectory(csPath)) { // 不存在则创建
		CreateDirectory(csPath, 0);
	}

#if 0
	csPath = CString(DRUSTOREDIR2) + "\\1701File";
	if (!PathIsDirectory(csPath)) { // 不存在则创建
		CreateDirectory(csPath, 0);
	}
#endif

	newFile1701.Format("%02x%02x_%02x_%s.edf", g_TrainId[1], g_TrainId[2], g_TrainId[3], date);
	newFile1701 = csPath + "\\" +  newFile1701;
	printf("fileName:%s\n", newFile1701);

#if 0
	csPath = CString(DRUSTOREDIR2) + "\\1801File";
	if (!PathIsDirectory(csPath)) { // 不存在则创建
		CreateDirectory(csPath, 0);
	}
#endif

	newFile1801.Format("%02x%02x_%02x_%s.eds", g_TrainId[1], g_TrainId[2], g_TrainId[3], date);
	newFile1801 = csPath + "\\" +  newFile1801;
	printf("fileName:%s\n", newFile1801);


	// 第一次启动   按当前时间 创建文件，并开始记录 
	CFileFind fFind;
	if (!fFind.FindFile(newFile1701)) {
		if (!FileRecordMVBTraAllData1701.Open(newFile1701, 
					CFile::modeCreate |CFile::shareDenyNone | CFile::modeWrite | CFile::typeBinary)) {
			printf("Create 1701File Record  Failed! \r\n");
			isFileOpen1701 = FALSE;
		} else {
			isFileOpen1701 = TRUE;		
		}	
	} else {
		if (!FileRecordMVBTraAllData1701.Open(newFile1701, CFile::shareDenyNone | CFile::modeWrite | CFile::typeBinary)) {
			printf("Create 1701File Record  Failed! \r\n");
			isFileOpen1701 = FALSE;
		} else {
			isFileOpen1701 = TRUE;		
		}	
	}

	if (!fFind.FindFile(newFile1801)) {
		if (!FileRecordMVBTraAllData1801.Open(newFile1801, 
					CFile::modeCreate |CFile::shareDenyNone | CFile::modeWrite | CFile::typeBinary)) {
			printf("Create 1801File Record  Failed! \r\n");
			isFileOpen1801 = FALSE;
		} else {
			isFileOpen1801 = TRUE;		
		}	
	} else {
		if (!FileRecordMVBTraAllData1801.Open(newFile1801, CFile::shareDenyNone | CFile::modeWrite | CFile::typeBinary)) {
			printf("Create 1801File Record  Failed! \r\n");
			isFileOpen1801 = FALSE;
		} else {
			isFileOpen1801 = TRUE;		
		}	
	}

	while (1) {  // 100ms
		time         = CTime::GetCurrentTime();
		CString date = time.Format("%Y%m%d%H%M%S");
		CString tHM  = time.Format("%H%M%S");
		int tHMS     = _ttoi(tHM);
		tHM += _T("\r\n");

		//if ((tHMS % 100) == 0) { // 整点 {{10000}} 1h生成 1个文件 
		if ((tHMS % 10000) == 0) { // 整点 {{10000}} 1h生成 1个文件 

			CFileFind fFind;

			FileRecordMVBTraAllData1701.Close();
			FileRecordMVBTraAllData1801.Close();
			isFileOpen1701 = FALSE;
			isFileOpen1801 = FALSE;

			/* 文件路径 */
			CString csPath(DRUSTOREDIR2);
			if (!PathIsDirectory(csPath)) { // 不存在则创建
				CreateDirectory(csPath, 0);
			}
#if 0
			csPath = CString(DRUSTOREDIR2) + "\\1701File";
			if (!PathIsDirectory(csPath)) { // 不存在则创建
				CreateDirectory(csPath, 0);
			}
#endif

			newFile1701.Format("%02x%02x_%02x_%s.edf", g_TrainId[1], g_TrainId[2], g_TrainId[3], date);
			newFile1701 = csPath + "\\" +  newFile1701;

#if 0
			csPath = CString(DRUSTOREDIR2) + "\\1801File";
			if (!PathIsDirectory(csPath)) { // 不存在则创建
				CreateDirectory(csPath, 0);
			}
#endif

			newFile1801.Format("%02x%02x_%02x_%s.eds", g_TrainId[1], g_TrainId[2], g_TrainId[3], date);
			newFile1801 = csPath + "\\" +  newFile1801;

			if (fFind.FindFile(newFile1701)) {  // 文件 已存在 ，继续写
				if (!FileRecordMVBTraAllData1701.Open(newFile1701, CFile::shareDenyNone | CFile::modeWrite | CFile::typeBinary)) {
					printf("Create 1701File Record  Failed! \r\n");
					isFileOpen1701 = FALSE;
				} else {
					FileRecordMVBTraAllData1701.SeekToEnd(); 
					isFileOpen1701 = TRUE;		
				}	
			} else { // 文件 不存在 ， 新建 
				if (!FileRecordMVBTraAllData1701.Open(newFile1701, CFile::modeCreate |CFile::shareDenyNone | CFile::modeWrite | CFile::typeBinary)) {
					printf("Create 1701File Record  Failed! \r\n");
					isFileOpen1701 = FALSE;
				} else {
					FileRecordMVBTraAllData1701.SeekToBegin();  // 到达文件开头 
					isFileOpen1701 = TRUE;		
				}	
			}

			if (fFind.FindFile(newFile1801)) {  // 文件 已存在 ，继续写
				if (!FileRecordMVBTraAllData1801.Open(newFile1801, CFile::shareDenyNone | CFile::modeWrite | CFile::typeBinary)) {
					printf("Create 1801File Record  Failed! \r\n");
					isFileOpen1801 = FALSE;
				} else {
					FileRecordMVBTraAllData1801.SeekToEnd(); 
					isFileOpen1801 = TRUE;		
				}	
			} else { // 文件 不存在 ， 新建 
				if (!FileRecordMVBTraAllData1801.Open(newFile1801, CFile::modeCreate |CFile::shareDenyNone | CFile::modeWrite | CFile::typeBinary)) {
					printf("Create 1801File Record  Failed! \r\n");
					isFileOpen1801 = FALSE;
				} else {
					FileRecordMVBTraAllData1801.SeekToBegin();  // 到达文件开头 
					isFileOpen1801 = TRUE;		
				}	
			}
		}

		/* 100ms 记录 1701数据 */
		WaitForSingleObject(hWriteDRU1, INFINITE);
		if (isFileOpen1701) {				
			//FileRecordMVBTraAllData1701.Write(tHM, tHM.GetLength());
			FileRecordMVBTraAllData1701.Write(cData1701, tHM.GetLength());
			FileRecordMVBTraAllData1701.Flush();		
		}
		ReleaseMutex(hWriteDRU1);

		/* 1000ms 记录 1801数据 */
		if ((iCounter % 10) == 0) {  // 1000ms
			WaitForSingleObject(hWriteDRU2, INFINITE);
			if (isFileOpen1801) {				
				FileRecordMVBTraAllData1801.Write(cData1801, LENGTH_1801);
				FileRecordMVBTraAllData1801.Flush();		
			}
			ReleaseMutex(hWriteDRU2);
		}

		iCounter++;
		Sleep(100);  // 100ms
	}

	FileRecordMVBTraAllData1701.Close();
	FileRecordMVBTraAllData1801.Close();
	isFileOpen1701 = FALSE;					
	isFileOpen1801 = FALSE;					

	return 0;
}

#define MCASTADDR "239.194.0.0"
#define MCASTPORT  17224
#define BUFSIZE    112

bool bIsCurrentETBNValid; 
int	 iGetETBNThreaState;
int  iCurrentETBNNO;

/**
 * @brief    --  ThreadTrdpDataProcess: TRDP 数据刷新  周期：10ms
 * @param[ ] -- lpParam
 * @return   -- 
 */
UINT ThreadTrdpDataProcess(LPVOID lpParam)
{
	printf("start  {{ThreadTrdpDataProcess}} ...\n");

	static int iCounterLife = 0;
	static stTrdpDataInfo dataGet;

	st_data_info dataRecord;  // mvb 初始化
	st_data_info dataComm;    // comm 初始化
	st_data_info dataFault1;   // fault1 初始化
	st_data_info dataFault3;   // fault3 初始化

	memcpy(dataComm.strType, "comm-record", strlen("comm-record"));
	memset(dataComm.data,  0, 20000);

	memcpy(dataRecord.strType, "tcn-bcn", strlen("tcn-bcn"));
	memset(dataRecord.data,  0, 20000); 

	memcpy(dataFault1.strType, "fault1", strlen("fault1"));
	memset(dataFault1.data,  0, 20000); 

	memcpy(dataFault3.strType, "fault3", strlen("fault1"));
	memset(dataFault3.data,  0, 20000); 

	unsigned char HeadPacket[22]  = {0}; // 组帧，初始化帧头: 需要根据协议填充。
	unsigned char CarID = 0;
	unsigned char SwVer = 0;
    /* 动车组车型区分码 默认：0x0A */
	unsigned char CarInfID = 0x0A;

	HeadPacket[0]  = 0xAA;
	HeadPacket[1]  = 0xAB;
	HeadPacket[2]  = 0xAC;
	ULONG devid    = DeviceID;

    /* 车载设备ID：例如：0x16020004 */
	HeadPacket[3]  = (devid / 10000000) * 16 + (devid / 1000000) % 10;
	HeadPacket[4]  = ((devid / 100000) % 10) * 16 +(devid / 10000) % 10;
	HeadPacket[5]  = ((devid / 1000) % 10) * 16 + (devid / 100) % 10;
	HeadPacket[6]  = ((devid / 10) % 10) * 16 + (devid) % 10;

    /* 设备所在列车号：例如：00523008 */
	HeadPacket[7]  = ( g_TrainId[1])/10 * 16 +  g_TrainId[1]%10;
	HeadPacket[8]  = ( g_TrainId[2])/10 * 16 +  g_TrainId[2]%10;
	HeadPacket[9]  = ( g_TrainId[3])/10 * 16 +  g_TrainId[3]%10;
	HeadPacket[10] = ( g_TrainId[4])/10 * 16 +  g_TrainId[4]%10;

    /* 动车组车型代码 0x36 = CRH3A */
	HeadPacket[11] = 0x36;   // CarType
	HeadPacket[12] = CarInfID;
	HeadPacket[13] = 0;
	HeadPacket[14] = 0;

    /* 协议版本 0x9C = 156 V1.56 */
	HeadPacket[15] = SwVer;
	HeadPacket[16] = 0; // 应答标志

    /* 数据帧序号 */
	HeadPacket[17] = 0;
	HeadPacket[18] = 0;
	HeadPacket[19] = 0;
	HeadPacket[20] = 0;
    /* 数据类型 0x41 = MVB端口数据所有 */
	HeadPacket[21] = 0;

	for (UINT i = 0; i < 13; i++) {
		cData1701[i] = HeadPacket[i];
	}

	cData1701[13] = (LENGTH_1701 - 19) % 256;
	cData1701[14] = (LENGTH_1701 - 19) / 256;
	cData1701[15] = HeadPacket[15];

	while (true) {
		if (g_iCycleCnt >= 65535) {
			g_iCycleCnt = 0;
		}

		/* 公共端口 {1701、1702}:刷新周期100ms， {1801、1802、1301、1302}:刷新周期500ms */
		for (int i=0; i < g_iTRDP_common_size; i++) {
			if ((g_iCycleCnt % (trdpport_comm[i].cycle / 10)) == 0) {  // 计算刷新周期
				// printf("g_iCycleCnt: %d, com:%d, cycle:%d \r\n", g_iCycleCnt, trdpport_comm[i].comid, trdpport_comm[i].cycle);

				if (trdpport_comm[i].direction == PUBLISH) {					

					//printf("direction %d, comid %d portsize %d cycle %d \r\n", trdpport_comm[i].direction, 
					//		 trdpport_comm[i].comid, trdpport_comm[i].portsize, trdpport_comm[i].cycle);

#if 0
					iCounterLife++;
					if (iCounterLife >= 65530) {
						iCounterLife = 0;
					}

					memset(putData[0].data, 0x55, 200);

					/* 取消前两个字节发送生命信号 */
					putData[0].data[0] =  iCounterLife / 256;
					putData[0].data[1] =  iCounterLife % 256;
					putData[0].data[2] =  0x99;
					putData[0].data[3] =  0x33;
					putData[0].data[4] =  0x88;

					putData[0].data[28] =  0x55;
					putData[0].data[29] =  0x88;
					putData[0].data[30] =  0x00;
					putData[0].data[31] =  0x01;
#endif
                    st_data_info dataPut;
					m_shareMemory.ReadMvbDatatoTrdp(&dataPut);   // 读取共享内存中的mvb数据
					memcpy(g_TrainId, dataPut.data + 7, 4);

#if 0
                    printf("ReadMvbDatatoTrdp type : %s, \r\n", dataPut.strType);

                    for (int icounter = 0; icounter < 100; icounter++) {
                        printf("read data[%d]:0x%x \r\n", icounter, (unsigned char)dataPut.data[icounter]);
                    }
#endif
					int retTmp = -99;

					try {
						//printf("putdata value:%x, portsize:%d, pub_handle_addr:%p\n", putData[0].data[1], trdpport_comm[i].portsize, &pub_handle_comm[i]);

						retTmp = tlp_put(session_handle, pub_handle_comm[i], (DG_U8*)&dataPut.data + 60, trdpport_comm[i].portsize);    // + 60 字节偏移为60
					} catch (...) {
						printf("catch---exception --\n");
					}

					// printf("Put comID:%d, return %d  IP_addr: %s\r\n", trdpport_comm[i].comid, retTmp, trdpport_comm[i].trdpip);
					tlc_process(session_handle, NULL, NULL);

				} else if (trdpport_comm[i].direction == SUBSCRIBE) {

					dataGet.lPort = trdpport_comm[i].comid;
					dataGet.portSize = trdpport_comm[i].portsize;
					ret_val = tlp_get(session_handle, sub_handle_comm[i], &pd_info[i], (DG_U8*)&dataGet.data, &trdpport_comm[i].portsize);
					// printf("Get %d,return %d \r\n", trdpport_comm[i].comid, ret_val);

					if (TRDP_NO_ERR == ret_val) {   // 收发数据无错误  -> 处理数据
						/* 1，获取TRDP数据 */
						/* cData1701 : (head){22} + (data){960 + 960 + 960 + 960} +  (fcs + end){1 + 3}  = 3866 (LENGTH_1701) */
						if (dataGet.lPort == 1701) {  // 1701 数据长度为 1000
							// unCounter dataLength;
							// memcpy(dataLength.cCounter, dataGet.data +24, 4); // 获取报文 应用内容的实际长度
							memcpy(cData1701 + 22, dataGet.data + 40, 960);  // trdp 包头：40个字节
						}
						if (dataGet.lPort == 1702) {  // 1702 数据长度为 1000  
							memcpy(cData1701+ 960 + 22, dataGet.data + 40, 960);
						}   

						if (dataGet.lPort == 1703) {  // 1702 数据长度为 1000  
							memcpy(cData1701+ 960 + 960 + 22, dataGet.data + 40, 960);
						}   

						if (dataGet.lPort == 1704) {  // 1702 数据长度为 1000  
							memcpy(cData1701+ 960 + 960 +960 + 22, dataGet.data + 40, 960);
						}   

						/* cData1801 : (head){22} + (data){960 + 960 + 960 + 960 + 1360 + 1360 + 1360 + 1360} + (fcs + end){1 + 3} = 9306 (LENGTH_1801) */
						if (dataGet.lPort == 1801) { // 1801 数据长度为 1000  
							memcpy(cData1801 + 22, dataGet.data + 40, 960);

                            /* 获取 三级 故障数据 */
                            for (int i = 0; i < g_iVarFault3Length; i++) {
                                cDataFault3C1[g_stVarFault3[i].iDestNum] = 
                                    GET_BIT(dataGet.data[g_stVarFault3[i].stOffset.iWordOffset + 40],  g_stVarFault3[i].stOffset.iBitOffset);
                            }

							memcpy(dataFault3.data, cDataFault3C1, g_iVarFault3Length);
						}
						if (dataGet.lPort == 1802) { // 1802 数据长度为 1000  
							memcpy(cData1801 + 960 + 22, dataGet.data + 40, 960);
                            /* 获取 三级 故障数据 */
                            for (int i = 0; i < g_iVarFault3Length; i++) {
                                cDataFault3C2[g_stVarFault3[i].iDestNum] = 
                                    GET_BIT(dataGet.data[g_stVarFault3[i].stOffset.iWordOffset + 40],  g_stVarFault3[i].stOffset.iBitOffset);
                            }

							memcpy(dataFault3.data + 5000, cDataFault3C2, g_iVarFault3Length);
						}
						if (dataGet.lPort == 1803) { // 1802 数据长度为 1000  
							memcpy(cData1801 + 960 + 960 + 22, dataGet.data + 40, 960);
                            /* 获取 三级 故障数据 */
                            for (int i = 0; i < g_iVarFault3Length; i++) {
                                cDataFault3C3[g_stVarFault3[i].iDestNum] = 
                                    GET_BIT(dataGet.data[g_stVarFault3[i].stOffset.iWordOffset + 40],  g_stVarFault3[i].stOffset.iBitOffset);
                            }

							memcpy(dataFault3.data + 10000, cDataFault3C3, g_iVarFault3Length);
						}
						if (dataGet.lPort == 1804) { // 1802 数据长度为 1000  
							memcpy(cData1801 + 960 + 960 + 960 + 22, dataGet.data + 40, 960);
                            /* 获取 三级 故障数据 */
                            for (int i = 0; i < g_iVarFault3Length; i++) {
                                cDataFault3C4[g_stVarFault3[i].iDestNum] = 
                                    GET_BIT(dataGet.data[g_stVarFault3[i].stOffset.iWordOffset + 40],  g_stVarFault3[i].stOffset.iBitOffset);
                            }

							memcpy(dataFault3.data + 15000, cDataFault3C4, g_iVarFault3Length);
						}
						if (dataGet.lPort == 1301) { // 1301 数据长度为 1400  
							memcpy(cData1801 + 3840 + 22, dataGet.data + 40, 1360);  // 960 * 6 = 3840

                            /* 获取 一二级 故障数据 */
                            for (int i = 0; i < g_iVarFault1Length; i++) {
                                cDataFault1C1[g_stVarFault1[i].iDestNum] = 
                                    GET_BIT(dataGet.data[g_stVarFault1[i].stOffset.iWordOffset + 40],  g_stVarFault1[i].stOffset.iBitOffset);
                            }

							memcpy(dataFault1.data, cDataFault1C1, g_iVarFault1Length);
						}
						if (dataGet.lPort == 1302) { // 1302 数据长度为 1400  
							memcpy(cData1801 + 1360 + 3840 + 22, dataGet.data + 40, 1360);

                            /* 获取 一二级 故障数据 */
                            for (int i = 0; i < g_iVarFault1Length; i++) {
                                cDataFault1C2[g_stVarFault1[i].iDestNum] = 
                                    GET_BIT(dataGet.data[g_stVarFault1[i].stOffset.iWordOffset + 40],  g_stVarFault1[i].stOffset.iBitOffset);
                            }

							memcpy(dataFault1.data + 5000, cDataFault1C2, g_iVarFault1Length);
						}
						if (dataGet.lPort == 1303) { // 1302 数据长度为 1400  
							memcpy(cData1801 + 1360 + 1360 + 3840 + 22, dataGet.data + 40, 1360);

                            /* 获取 一二级 故障数据 */
                            for (int i = 0; i < g_iVarFault1Length; i++) {
                                cDataFault1C3[g_stVarFault1[i].iDestNum] = 
                                    GET_BIT(dataGet.data[g_stVarFault1[i].stOffset.iWordOffset + 40],  g_stVarFault1[i].stOffset.iBitOffset);
                            }

							memcpy(dataFault1.data + 5000, cDataFault1C3, g_iVarFault1Length);
						}
						if (dataGet.lPort == 1304) { // 1302 数据长度为 1400  
							memcpy(cData1801 + 1340 + 1360 + 1360 + 3840 + 22, dataGet.data + 40, 1360);

                            /* 获取 一二级 故障数据 */
                            for (int i = 0; i < g_iVarFault1Length; i++) {
                                cDataFault1C4[g_stVarFault1[i].iDestNum] = 
                                    GET_BIT(dataGet.data[g_stVarFault1[i].stOffset.iWordOffset + 40],  g_stVarFault1[i].stOffset.iBitOffset);
                            }

							memcpy(dataFault1.data + 5000, cDataFault1C4, g_iVarFault1Length);
						}

						/* 2，对 1701/1801 数据进行处理 。头、尾、校验 */
						/* 对数据进行处理，提取数据区，重新组帧 */
						/* 组帧：0xAA 0xAB 0xAC [4]{carNumber} [2]{dataLength} [1]{version} [4]{frameNumber} [1]{dataType 0x42} 
                         * [6]{utc时间 ms} [n]{data} [1]{FCS} 0xBA 0xBB */

						if ((dataGet.lPort == 1701) || (dataGet.lPort == 1702) ||
                                (dataGet.lPort == 1703) || (dataGet.lPort == 1704)) {  // 1701 
							unCounter frameNumber1701;

							frameNumber1701.iCounter = 0;  // 计数 初始化
							frameNumber1701.iCounter++;
							if (frameNumber1701.iCounter >= 0xffffffff) {
								frameNumber1701.iCounter = 0;
							}

							cData1701[16] = 0;
							cData1701[17] = frameNumber1701.cCounter[0];
							cData1701[18] = frameNumber1701.cCounter[1];
							cData1701[19] = frameNumber1701.cCounter[2];
							cData1701[20] = frameNumber1701.cCounter[3];
							cData1701[21] = 0x42; // 0x42 =公共端口信息 1701~1704 100ms

							BYTE  FCS = 0;
							for (unsigned short iall = 3; iall < LENGTH_1701 - 4; iall++) {		
								FCS ^= cData1701[iall];
							}

                            /* 尾帧 */
							cData1701[LENGTH_1701 - 4] = FCS;
							cData1701[LENGTH_1701 - 3] = 0xBA; 
							cData1701[LENGTH_1701 - 2] = 0xBB;
							cData1701[LENGTH_1701 - 1] = 0xBC;

							memcpy(dataComm.data, cData1701, LENGTH_1701);
						}

                        if ( (dataGet.lPort == 1801) || (dataGet.lPort == 1802) || (dataGet.lPort == 1803) || (dataGet.lPort == 1804) ||
                                (dataGet.lPort == 1301) || (dataGet.lPort == 1302) || (dataGet.lPort == 1303) || (dataGet.lPort == 1304)) {  // 1801 
                            unCounter frameNumber1801;

							frameNumber1801.iCounter = 0;  // 计数 初始化
							frameNumber1801.iCounter++;
							if (frameNumber1801.iCounter >= 0xffffffff) {
								frameNumber1801.iCounter = 0;
							}

							for (UINT i = 0; i < 13; i++) {
								cData1801[i] = HeadPacket[i];
							}

							cData1801[13] = (LENGTH_1801 - 19) % 256;
							cData1801[14] = (LENGTH_1801 - 19) / 256;
							cData1801[15] = HeadPacket[15];
							cData1801[16] = 0;
							cData1801[17] = frameNumber1801.cCounter[0];
							cData1801[18] = frameNumber1801.cCounter[1];
							cData1801[19] = frameNumber1801.cCounter[2];
							cData1801[20] = frameNumber1801.cCounter[3];
							cData1801[21] = 0x43; // 0x43 = 一二三级故障信息 1301~1304 1801~1804 500ms

							BYTE  FCS = 0;
							for (unsigned short iall = 3; iall < LENGTH_1801 - 4; iall++) {		
								FCS ^= cData1801[iall];
							}

                            /* 尾帧 */
							cData1801[LENGTH_1801 - 4] = FCS;  
							cData1801[LENGTH_1801 - 3] = 0xBA; 
							cData1801[LENGTH_1801 - 2] = 0xBB;
							cData1801[LENGTH_1801 - 1] = 0xBC;

							memcpy(dataComm.data + 10000 , cData1801, LENGTH_1801);
						}

						/* 3，写入共享内存 */
						m_shareMemory.WriteTrdpDatatoMvb(&dataComm, 0);     // 公共 数据

						if (g_iCtlType == 1) { // 1：trdp控车、 0： mvb控车
							m_shareMemory.WriteTrdpDatatoMvb(&dataFault1, 2);   // 一二级故障 数据
						}

						m_shareMemory.WriteTrdpDatatoMvb(&dataFault3, 3);   // san级故障 数据
					}

					tlc_process(session_handle, NULL, NULL);
				}
			}
		}

		/* dru过程数据 */
		for (int j=0; j <  g_iTRDP_record_size; j++) {
			if ((g_iCycleCnt % (trdpport_record[j].cycle / 10)) == 0) {  // 计算刷新周期
				// printf("g_iCycleCnt: %d, com:%d, cycle:%d \r\n", g_iCycleCnt, trdpport_record[j].comid, trdpport_record[j].cycle);

				stTrdpDataInfo  dataGetRecord; 
                /* 控车模式判断 */
                if (trdpport_record[j].comid == g_iCtlModeComID) {
                    ret_val = tlp_get(session_handle, sub_handle_record[j], &pd_info[j], (DG_U8*)&dataGetRecord.data, &trdpport_record[j].portsize);

                    if (TRDP_NO_ERR == ret_val) {   // 收发数据无错误  -> 处理数据
                        if (dataGetRecord.data[g_iCtlModeOffset] == g_iCtlModeValue) {
                            g_iCtlType = 1; // 控车模式   ：1：trdp控车、 0： mvb控车
                        } else {
                            g_iCtlType = 0; // 控车模式   ：1：trdp控车、 0： mvb控车
                        }
                    }
                }

                if (g_iCtlType == 1) { // 1：trdp控车、 0： mvb控车
                    dataGetRecord.lPort = trdpport_record[j].comid;
                    dataGetRecord.portSize =  trdpport_record[j].portsize;
                    ret_val = tlp_get(session_handle, sub_handle_record[j], &pd_info[j], (DG_U8*)&dataGetRecord.data, &trdpport_record[j].portsize);

                    if (TRDP_NO_ERR == ret_val) {   // 收发数据无错误  -> 处理数据
                        // printf("Get %d,return %d \r\n",trdpport_record[j].comid, ret_val);
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

                    }
                }
				tlc_process(session_handle, NULL, NULL);
			}
		}

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

	path = "C:\\config\\TRDP_Config.xls";
	// path = "C:\\Documents and Settings\\Administrator\\桌面\\CRHWTDBZ\\WTD_ETH\\TRDP_Config.xls";
	path.Replace("\\", "\\\\");

	CStringArray Rows;  // 定义工作表的行和列
	CStringArray Column; 
	int index = 0;

	CSpreadSheet xls_record(path, "ECN_TCN"); //sheet2 记录端口数据

	unsigned address;
	int t[4];
	CString str;

	sscanf(str, "%x", t);
	address = t[0];

	m_ilen_record = xls_record.GetTotalRows();
	g_iTRDP_record_size = m_ilen_record - 1;   // 记录以太网 端口数量
	for (index = 2; index <= m_ilen_record; index++) {
		xls_record.ReadRow(Rows, index); // 逐行读取

		/* 解析每一行的内容 */
		str = Rows.GetAt(0); 
		if (str != "") {
			// trdpport_record[index-2].trdpip = str.GetBuffer(str.GetLength()); //端口地址
			trdpport_record[index-2].trdpip = str;
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
				trdpport_record[index-2].direction =  SUBSCRIBE;
			} else if (strTmp.CompareNoCase("publish") == 0) {  // 不区分大小写 比较字符
				trdpport_record[index-2].direction =  PUBLISH;
			} 
		} else {
			//将故障信息写入日志文件
		}
	}

	CSpreadSheet xls_comm(path, "Comm");      //sheet1  公共端口数据
	m_ilen_comm = xls_comm.GetTotalRows();
	g_iTRDP_common_size = m_ilen_comm - 1;   // 公共以太网 端口数量
	for (index=2; index <= m_ilen_comm; index++) {  
		xls_comm.ReadRow(Rows, index); // 逐行读取

		/* 解析每一行的内容 */
		str = Rows.GetAt(0); 
		if (str != "") {
			// trdpport_comm[index-2].trdpip = str.GetBuffer(str.GetLength()); //端口地址
			trdpport_comm[index-2].trdpip = str;
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

    CSpreadSheet xls_local_ip(path, "Local"); 
    xls_local_ip.ReadRow(Rows, 2); 
    /* 解析行的内容 */
    str = Rows.GetAt(0); 
    if (str != "") {
        strLocalIP = str;
        printf("Get local ip: %s \n", strLocalIP);
    } else {
        strLocalIP = "10.0.1.210";
        printf("Get default local ip: %s \n", strLocalIP);
        //将故障信息写入日志文件
    }


    str = Rows.GetAt(1);   // comid
    if (str != "") {
        sscanf(str, "%d", &t);
        g_iCtlModeComID = t[0];
    } 
    str = Rows.GetAt(2);   // word offset
    if (str != "") {
        sscanf(str, "%d", &t);
        g_iCtlModeOffset = t[0];
    } 
    str = Rows.GetAt(3);   // Value
    if (str != "") {
        sscanf(str, "%d", &t);
        g_iCtlModeValue = t[0];
    } 

    str = Rows.GetAt(4);   // 兼容 TRDP 
    if (str != "") {
        sscanf(str, "%d", &t);
        g_iIsTRDP = t[0];
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

	printf("tlc_init Result: %d\n", ret_val);

	if (TRDP_NO_ERR == ret_val)
		ret_val = tlc_openSession(&session_handle,
				vos_dottedIP(strLocalIP),       // 本机ip地址
				0,
				NULL,
				&pd_Config,
				NULL,
				&process_Config); /*use default ip*/
	printf("openSession Result: %d\n", ret_val);

	if (TRDP_NO_ERR == ret_val) {
		ret_val = trdp_pd_config();
		printf("config Result: %d\n", ret_val);
	}

	printf("Result: %d\n", ret_val);
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
	st_data_info dataReserve;  // mvb 初始化
	memcpy(dataReserve.strType, "other", strlen("other"));
	memset(dataReserve.data, 0, 20000); 

	// TODO: Add your message handler code here and/or call default
	switch (nIDEvent) {
		case TRDPSENDTIMER:  // 10ms


#if 1
			// 计算FTP的防火墙设置地址
			// strFtpIP: ftp 服务器ip地址          strLocalIP: local ip地址

            try {
                g_iETBN = getETBN();  // bianzu
				int iTmp = 0;

				if ( g_iETBN >= 1 &&  g_iETBN  <= 2) {
					iTmp =  3 - g_iETBN;
				} else if ( g_iETBN == 3 ||  g_iETBN  == 4) {
					iTmp =  7 - g_iETBN;
				}

                NOPSAVFTPIPSet(strLocalIP, iTmp, strFtpIP);
            } catch (...) {
                printf("get ETBN exception-----\r\n");

            }
			m_TRDPReceive.Empty();
			CString strTmp100;
			for (int c=0; c<100; c++) {
				strTmp100.Format("%02X ", *(cData1701 + c));   // get data
				m_TRDPReceive += strTmp100;
			}
#endif

#if 0
			st_data_info dataPut;
			m_shareMemory.ReadMvbDatatoTrdp(&dataPut, 1);   // 读取共享内存中的mvb数据

			printf("ReadMvbDatatoTrdp type : %s, \r\n", dataPut.strType);
			for (int icounter = 0; icounter < 100; icounter++) {
				printf("read data[%d]:0x%x \r\n", icounter, (unsigned char)dataPut.data[icounter]);
			}

#endif
			UpdateData(TRUE);

			st_data_info dataRecord;  // mvb 初始化
			memcpy(dataRecord.strType, "tcn-bcn", strlen("tcn-bcn"));
			memset(dataRecord.data,  0, 20000); 

			char ch1[10],ch2[10],ch3[10];
			unsigned char ucTestType, ucTestValue, ucTestPlace;

			GetDlgItem(IDC_EDIT_TYPE)->GetWindowText(ch1, 10); 
			GetDlgItem(IDC_EDIT_PLACE)->GetWindowText(ch2, 10);
			GetDlgItem(IDC_EDIT_VALUE)->GetWindowText(ch3, 10);

			ucTestType  = atoi(ch1);
			ucTestPlace = atoi(ch2);
			ucTestValue = atoi(ch3);

			printf("ucTest Type   data:%x \r\n", ucTestType);
			printf("ucTest OffSet data:%x \r\n", ucTestPlace);
			printf("ucTest Value  data:%x \r\n", ucTestValue);

			if (ucTestType >= 0 && ucTestType <= 5) {
				dataRecord.data[ucTestPlace] = ucTestValue;
				/*写入共享内存 */
				m_shareMemory.WriteTrdpDatatoMvb(&dataRecord, ucTestType);
			}

			break;
	}

	/**
	 *  TRDP传给MVB的信息格式（共享内存4）
	 *	 第0字节：编组信息
	 *	 第1字节：控车模式标志位，（如果读不到0xAA，可赋值为0）
	 */

	dataReserve.data[0] = g_iETBN;
	dataReserve.data[1] = g_iCtlType;
	printf("bian zu hao : %02x \r\n ", g_iETBN);
	m_shareMemory.WriteTrdpDatatoMvb(&dataReserve, 4);

	CDialog::OnTimer(nIDEvent);
}

/**
 * @brief    --  getETBN
 */
/**
 * @brief    -- getETBN: 获取对面列车编组号 
 */
unsigned char CTRDPServerDlg::getETBN()  
{
	bIsCurrentETBNValid = 0;
	iCurrentETBNNO=0;

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
		iGetETBNThreaState = 0;
		return -1;
	}

	if ((sock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, 
					WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		WSACleanup();
		iGetETBNThreaState = 0;
		return -1;
	}

	/* 将sock绑定到本机某端口上。 */
	local.sin_family      = AF_INET;
	local.sin_port        = htons(MCASTPORT);
	local.sin_addr.s_addr = INADDR_ANY;   // 绑定此组播IP到自身的套接字上

	if (bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		iGetETBNThreaState = 0;
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
		iGetETBNThreaState = 0;
		return -1;
	}
	printf("add ok ....\n");

	/* 接收多播数据，当接收到的数据为第8，9字节收到100时退出。*/
	iGetETBNThreaState = 1;

	while (1) {
		if (iGetETBNThreaState == 2) {
			break;
		} else if (iGetETBNThreaState == 1) {
			;
		}

		printf("-------\n");
		if ((ret = recvfrom(sock, recvbuf, BUFSIZE, 0, (struct sockaddr*)&from, &len)) == SOCKET_ERROR) {
			closesocket(sockM);
			closesocket(sock);
			WSACleanup();
			iGetETBNThreaState=0;
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
			ret = recvbuf[93];
			str_ret.Format("Current_ETBN %d", ret);
			bIsCurrentETBNValid = 1;
			iCurrentETBNNO      = ret;
			printf("%s\n", str_ret);

			CString strIp = "10.0.0.1";    
		}
		break;

		Sleep(1000);
	}

	closesocket(sockM);
	closesocket(sock);
	WSACleanup();

	if (iGetETBNThreaState == 2) {
		printf("检测ETBN工作因重复请求被停止，可重新发起请求！");
	} else {
		printf("已获取本地编组号，请选择要连接设备所在的编组号");
	}
	iGetETBNThreaState = 0;
	return ret; // 根据编组号
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
			m_pFtpConnection = m_pInetSession->GetFtpConnection(ServerIP, name, Password, port);  // 默认使用主动模式
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

#if 0
	/* 遍历map中的数据 */
	CString strKey = _T(""), str = _T("");
	POSITION pos = mapFileLocal.GetStartPosition();

	while (pos) {
		mapFileLocal.GetNextAssoc(pos, strKey, str);
		printf("localFileName:%s, \t size:%s\n", strKey, str );
	}
#endif
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
	CString path = "C:\\config\\DRU_Config.xls";
//	CString path = "C:\\config\\DRU表格汇总20181121.xls";
	path.Replace("\\", "\\\\");

	CSpreadSheet xls_dru(path, "Sheet1"); // dru_config

	CStringArray Rows;  // 定义工作表的行和列
	CStringArray Column; 
	int index = 0;

	CString str;
	int i;
	int t;                   // 临时变量
	int ans           = 0;   // 端口数量     端口索引
	int iNumIndex     = 0;   // 端口中的变量 字索引
	int iBitPos       = 0;   // 变量的       位索引
	int comidTmp[35000] = {0}; // 端口数量
	int wordTmp[10000]  = {0};

	m_ilen_comm = xls_dru.GetTotalRows();
	//printf("Length : %d\n", m_ilen_comm);
	for (index=3,  i = 0; index <= m_ilen_comm; index++, ++i) {  
		xls_dru.ReadRow(Rows, index); // 逐行读取

		/* 解析每一行的内容 */
		str = Rows.GetAt(24);   // COMID
		if (str != "") {
			sscanf(str, "%d", &t);
			comidTmp[i] = t;

			if (i >= 1) {
				if (comidTmp[i-1] != t) {    // 查找不同 的 Com ID 
					printf("commid: %d \n", t);
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
				// printf("wordTmp -1 %d wordTmp: %d \n", wordTmp[iBitPos - 1], t);
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

#if 0
		printf("COMID[%d] PORTSIZE[%d] COUNTER[%d] VAR[%s] WORD[%d]:%d{%d} -> DEST[%d]  i: %d  ans: %d \r\n", 
				stCfgInfo[ans].comId,
				stCfgInfo[ans].portSize,
				stCfgInfo[ans].counter,
				stCfgInfo[ans].varInfo[iNumIndex].srcPos[iBitPos].varType,
				stCfgInfo[ans].varInfo[iNumIndex].srcNum,
				stCfgInfo[ans].varInfo[iNumIndex].srcPos[iBitPos].bit,
				stCfgInfo[ans].varInfo[iNumIndex].bitNum,
				stCfgInfo[ans].varInfo[iNumIndex].destNum, 
				i,
				ans);
#endif

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

#if 1
//计算FTP的防火墙设置地址
BOOL NOPSAVFTPIPSet(CString strsourceIP, BYTE bianzuNo, CString &tarstrIP)
{
	//修改:m_strNOPASVIP
	BYTE dataS[4];
	memset(dataS, 0, 4);

	int dataCnt  = 0;
	int dataLast = 0;
	int BUFCNT   = 0;

	CString str0 = "";
	CString str1 = "";

	for (UINT i = 0; i < strsourceIP.GetLength(); i++) {
		if (strsourceIP.GetAt(i) == '.') {
			CString str0 = "";
			CString str1 = "";

			for (dataCnt = dataLast; dataCnt < i; dataCnt++) {
				str0.Format("%c", strsourceIP.GetAt(dataCnt));
				str1 += str0;
			}

			dataS[BUFCNT] = (BYTE)atoi(str1);
			BUFCNT++;
			if (BUFCNT >= 3) { //计算公式
				dataS[BUFCNT] = (BYTE)atoi(strsourceIP.Mid((i+1),(strsourceIP.GetLength()-i-1)));
				break;
			} else {
				dataLast = i+1;
			}
		}
	}

	UINT32 devIpAddr = (dataS[0]<<24) | (dataS[1]<<16) | (dataS[2]<<8) | (dataS[3]);
	UINT32 globalIpAddr = 0;
	globalIpAddr        = ((devIpAddr & 0xFF00FFFF) | (bianzuNo << 14) | 0x00800000);

	//计算目标的IP地址
	CString str2 = "";
	CString str3 = "";
	str2.Format("IP From %d.%d.%d.%d", dataS[0], dataS[1], dataS[2], dataS[3]);
	str3.Format("%d.%d.%d.%d", (globalIpAddr>>24)&0xFF, (globalIpAddr>>16)&0xFF, (globalIpAddr>>8)&0xFF, globalIpAddr&0xFF);

	tarstrIP = str3;
	str3 = "--->IP Global: ";
	str3 += tarstrIP;
	printf("%s \r\n", str2 + str3);
	//AfxMessageBox(str2 + str3);

	return TRUE;
}

/**
 * @brief    -- ExecCommand : 执行外部程序 , 压缩文件
 * @param[ ] -- strCmdLine
 * @return   -- 
 */
BOOL ExecCommand(CString strCmdLine)
{
	BOOL  bRet;
	PROCESS_INFORMATION processInformation = {0};
	STARTUPINFO startupInfo = {0};
	startupInfo.cb = sizeof(startupInfo);

	int nStrBuffer = strCmdLine.GetLength() + 50;
	DWORD dwCode   = 0;

	bRet = CreateProcess(NULL, strCmdLine.GetBuffer(nStrBuffer), NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, 
			NULL, NULL, &startupInfo, &processInformation);
	if (!bRet) {
		printf(_T("Createprocess failed!"));
		//AfxMessageBox(_T("Createprocess failed!"));
		return FALSE;
	}

	WaitForSingleObject(processInformation.hProcess, INFINITE);

	bRet = GetExitCodeProcess(processInformation.hProcess, &dwCode);
	if (!bRet) {
		printf(_T("Get exitcode failed!"));
		CloseHandle(processInformation.hProcess);
		CloseHandle(processInformation.hThread);
		return FALSE;
	}
	CloseHandle( processInformation.hProcess );
	CloseHandle( processInformation.hThread );

	if (dwCode != 0) {
		printf(_T("Invalid file execute failed!"));
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief    -- DeleteTempDirectory ： 删除文件夹
 * @param[ ] -- strPath
 * @return   -- 
 */
BOOL DeleteTempDirectory(CString strPath)
{
	CFileFind finder;  
	CString path;  
	BOOL bFind(FALSE);

	if (strPath.IsEmpty()) {
		return FALSE;
	}

	path.Format(_T("%s/*.*"), strPath);  
	bFind = finder.FindFile(path);  
	while (bFind) {  
		bFind = finder.FindNextFile();  
		if (finder.IsDirectory() && !finder.IsDots()) {
			DeleteTempDirectory(finder.GetFilePath());
			RemoveDirectory(finder.GetFilePath());  
		}  else {
			DeleteFile(finder.GetFilePath());  
		}  
	} 

	RemoveDirectory(strPath);
	return TRUE;
}

/**
 * @brief    -- compressExport : 压缩
 */
void compressExport(CString strRecordFile)
{
	if (strRecordFile == _T("")) 
		return;

 	CString strTar = CString(DRUSTOREDIR2) + "\\" +  strRecordFile + ".zip";
	CString csFullName = CString(DRUSTOREDIR2) + "\\" + strRecordFile;

	CString strCmd,  strDll,  strCmdLine;
	CString strDirMnt, strDirNandflash, strDirDatabase;
	TCHAR szPath[MAX_PATH];
	memset(szPath, 0, MAX_PATH);
	::GetCurrentDirectory(MAX_PATH, szPath);

	strCmd.Format(_T("%s\\7z.exe"), szPath);
	strDll.Format(_T("%s\\7z.dll"), szPath);

	CFileFind finder;
	if (!finder.FindFile(strCmd) || !finder.FindFile(strDll)) { //finder.FindFile();找到,返回1。
		printf(_T("7z运行环境不存在，请确认已正确安装!"));
		return;
	}
	if (!finder.FindFile() || !finder.FindFile(csFullName)) {
		printf(_T("文件不存在!"));
		return;
	}

	strCmdLine.Format(_T("\"%s\" a -y -tgzip -sdel \"%s\" \"%s\""), strCmd, strTar, csFullName); // 压缩完成后，删除 
	if (!ExecCommand(strCmdLine))
		return;
	printf(_T("文件压缩成功!"));
}
#endif 

/**
 * @brief    -- getFaultConfig ：从 strFileName配置 文件中 获取TRDP变量与MVB故障信息的映射关系
 * @param[ ] -- mapInfo        : key : number  value: strName 
 * @param[ ] -- strFileName　　："fault12.xls"
 * @param[ ] -- type           ： fault1 : 1   fault3: 3
 */
void CTRDPServerDlg::getFaultConfig(mapFaultInfo &mapInfo, CString strFileName, int iType) 
{	
#if 0  // 写入文件 调试 
	CFile file; //定义文件变量
	CString filename = (".//variableTest_fault.log");

	if (file.Open(filename, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyRead)) { 
		file.SeekToBegin(); //到达文件开头 
	}
#endif 
	CString path = "C:\\config\\" + strFileName;
	path.Replace("\\", "\\\\");

	CSpreadSheet xls_fault1(path, "Sheet1"); // dru_config

	CStringArray Rows;  // 定义工作表的行和列
	CStringArray Column; 
	int index = 0;
	CString str;
	int t;                     // 临时变量
	int iNumIndex     = 0;     // 端口中的变量 字索引

    int iLength = xls_fault1.GetTotalRows();
	printf("Length : %d\n", iLength);

    for (index=2; index <= iLength; index++) {  
        xls_fault1.ReadRow(Rows, index); // 逐行读取

        int iNumTmp = 0;
        /* 解析每一行的内容 */
        str = Rows.GetAt(0);   // COMID
        if (str != "") {
            sscanf(str, "%d", &t);
            iNumTmp = t;
        } else {
            continue;
        }

        str = Rows.GetAt(2);
		CString strName;
		if (str != "" && iNumTmp != 0) {
			if (iType == 1) {
				strName = str + "c1all";
				strName.MakeUpper();
				mapInfo.SetAt(iNumTmp, strName);    // 插入到map，key:编号， value:文件名
			} else if (iType == 3) {
				strName = str + "man";
				strName.MakeUpper();
				mapInfo.SetAt(iNumTmp, strName);    // 插入到map，key:编号， value:文件名
			}
		} else {
			printf("NULL------------------: \r\n");
        }
    }

#if 0
	CString  strEditTmp;

	/* 遍历map中的数据 */
	int iKey;
	CString strValue = _T("");
	POSITION pos = mapInfo.GetStartPosition();

	while (pos) {
		mapInfo.GetNextAssoc(pos, iKey, strValue);
		strEditTmp.Format("Variable number :%d, name :%s \r\n", iKey, strValue);
		file.Write(strEditTmp, strEditTmp.GetLength());     // 写入实际数据 
	}
	file.Close();
#endif

	return;
}

/**
 * @brief    -- getFaultConfig ：从 strFileName配置 文件中 获取TRDP变量与MVB故障信息的映射关系
 * @param[ ] -- mapInfo        ：key : 变量名 value:变量字节偏移,位偏移 
 * @param[ ] -- strFileName    : "fault_1301.xls" "fault_1302.xls"
 */
void CTRDPServerDlg::getFaultConfig(mapVarInfo &mapInfo, CString strFileName) 
{	
#if 0  // 写入文件 调试 
	CFile file; //定义文件变量
	CString filename = (".//variableTest_var1.log");

	if (file.Open(filename, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyRead)) { 
		file.SeekToBegin(); //到达文件开头 
	}
#endif 

	CString path = "C:\\config\\" + strFileName;
	path.Replace("\\", "\\\\");
	printf("fileName : %s \r\n", path);

	CSpreadSheet xls_var(path, "Sheet1"); // dru_config

	CStringArray Rows;  // 定义工作表的行和列
	CStringArray Column; 
	int index = 0;

	CString str;
	int t;                     // 临时变量
	int iNumIndex     = 0;     // 端口中的变量 字索引

    int iLength = xls_var.GetTotalRows();
	printf("Length : %d\n", iLength);
    for (index = 2; index <= iLength; index++) {  
        xls_var.ReadRow(Rows, index); // 逐行读取

        stWordBit stTmp;
        /* 解析每一行的内容 */
        str = Rows.GetAt(11);   // word
        if (str != "") {
            sscanf(str, "%d", &t);
            stTmp.iWordOffset = t;
        } else {
            continue;
        }

        str = Rows.GetAt(12);   // word
        if (str != "") {
            sscanf(str, "%d", &t);
            stTmp.iBitOffset = t;
        } else {
            continue;
        }

        CString strVarName = "";
        strVarName = Rows.GetAt(1);
        if (strVarName != "") {
			strVarName.MakeUpper();
			mapInfo.SetAt(strVarName, stTmp);    // 插入到map，key:编号， value:文件名
		} else {
			printf("NULL------------------: \r\n");
		}
    }

#if 0
	CString  strEditTmp;

	/* 遍历map中的数据 */
	CString strKey = _T("");
	stWordBit stTest;
	POSITION pos = mapInfo.GetStartPosition();

	while (pos) {
		mapInfo.GetNextAssoc(pos, strKey, stTest);
		strEditTmp.Format("Variable Name:%s, word :%d, bit :%d \r\n", strKey, stTest.iWordOffset, stTest.iBitOffset);
		file.Write(strEditTmp, strEditTmp.GetLength());     // 写入实际数据 
	}
	file.Close();
#endif 

	return;
}


/**
 * @brief    -- getVarInfo  :  获取到 目的地址偏移与 trdp数据中的字节偏移、位偏移的映射关系
 * @param[o] -- stInfo      ：
 * @param[i] -- fautInfo
 * @param[i] -- varInfo
 * @return   -- 
 */
int CTRDPServerDlg::getVarInfo(stVarInfo *stInfo, mapFaultInfo &fautInfo, mapVarInfo &varInfo) 
{
    int iKey = 0, i = 0; 
    CString strVal= "";

    stWordBit stVal;

    printf("faultInfo number:%d,\n varInfo Number:%d\n", fautInfo.GetCount(), varInfo.GetCount());
    if (fautInfo.IsEmpty()) {  
        return false;
    } else {
        POSITION posLocal = fautInfo.GetStartPosition();
        while (posLocal) {   // 遍历 
            fautInfo.GetNextAssoc(posLocal, iKey, strVal); 
			// printf("key:%d, value:%s\n", iKey, strVal);

            if (varInfo.Lookup(strVal, stVal)) { 
                stInfo[i].iDestNum = iKey - 1;
                stInfo[i].stOffset.iWordOffset = stVal.iWordOffset;
                stInfo[i].stOffset.iBitOffset = stVal.iBitOffset;
                i++;
			} 
		}
	}
	return i;
}
