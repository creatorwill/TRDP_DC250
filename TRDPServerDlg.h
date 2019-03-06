// FTPServerDlg.h : header file

//#if !defined(AFX_TRDPSERVERDLG_H__219C9D6F_D033_494C_A407_B4C40EA2E553__INCLUDED_)
//#define AFX_TRDPSERVERDLG_H__219C9D6F_D033_494C_A407_B4C40EA2E553__INCLUDED_
#ifndef _TRDPSERVERDLG_H_
#define _TRDPSERVERDLG_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ListenSocket.h"	
//#include "ClientSocket.h"
#include <afxtempl.h>
#include <afx.h> 

//////////////////// trdp ////////////////////////////////
#pragma comment(lib, "Win32TRDP_DLL.lib" ) 
#pragma comment(lib, "version.lib")

#include "TRDP\os_def.h"
#include "TRDP\mclcppclass.h"
#include "TRDP\pugixml.hpp"
#include "TRDP\tau_xml.h"
#include "TRDP\tau_marshall.h"
#include "TRDP\trdp_if.h"
#include "TRDP\trdp_types.h"
#include "TRDP\vos_thread.h"
#include "TRDP\Markup.h"
#include "TRDP\mclcppclass.h"
#include "TRDP\mclmcrrt.h"
#include <afxtempl.h>
#include <afxinet.h>
#include <map>
#include "sharememoryclient.h"
#include "resource.h"

/******************************** 使用线程 *************************************/
// #define WM_USER_THREAD_FINISHED WM_USER + 0x100	// 自定义消息

UINT ThreadTrdpDataProcess(LPVOID lpParam); // 声明 {TRDP数据处理} 线程函数
UINT ThreadFileSync(LPVOID lpParam);        // 声明 {FTP文件同步} 线程函数
UINT ThreadFileRecord(LPVOID lpParam);      // 声明 {文件记录} 线程函数

/******************************** 使用线程 ************************************/
typedef union {
	unsigned int iCounter;
	unsigned char cCounter[4];
} unCounter;


typedef CMap<CString, LPCTSTR, CString, LPCTSTR> mapFileInfo; // 存储文件列表： 文件名, 文件大小
typedef CList<CString ,CString&> listFileInfo;   // 待同步文件信息

typedef struct _ST_TRDP_DATA_INFO {
	long     lPort;           // 端口号
	unsigned int portSize;    // 端口量数据大小
	unsigned char data[1500]; // 接收数据
} stTrdpDataInfo;             // TRDP数据信息

typedef struct {
	unsigned int destNum;     // 目标索引  字节
	int srcNum;               // 源数据索引  字节
	unsigned char bitNum;     // 字节中包含 位信息 的个数  <= 8;

	struct ST_POS_INFO {
		CString  varType;    // 变量的类型 
		unsigned char bit;   // 字/位偏移
	} srcPos[15];            // 源数据[字节] 包含位 ,至少要有一个 ;

} stComInfo;                 // 端口中字节的属性

typedef struct {
	int comId;                // 端口号
	int portSize;             // 端口大小
	int counter;              // 端口中 待处理数据内容的个数
	stComInfo varInfo[5000];  // 端口中 变量的个数   理论 < 1500  : 1001 -> 18000 由于1001端口数量过多，该值设为5000
} stConfigInfo;               // 端口配置信息

typedef struct {
	CString  trdpip;
	// CHAR8 *  trdpip;
	int  comid;               // 端口号
	unsigned int portsize;    // 端口大小，数组大小
	int cycle;                // 周期，尤其源端口发送数据需要该参数
	unsigned int direction;   // 数据方向，发送或接收，如二者都有写两次 ，PUBLISH = 0, SUBSCRIBE
	int byteofsetstart;
	int byteofsetend;
	int datalen;
} stTrdpPortPara;   // 端口配置数据信息

/* 字节偏移：位偏移 */
typedef struct ST_TYPE {
    int iWordOffset;
    int iBitOffset;
} stWordBit;

/* trdp数据需要存储的目标地址：trdp数据的字、位偏移 */
typedef struct ST_WORD_INFO{
    int iDestNum;
    stWordBit stOffset;
} stVarInfo;

typedef CMap<int, int&, CString, LPCTSTR> mapFaultInfo;             // key:目标变量字节偏移值， value: 变量名字
typedef CMap<CString, LPCTSTR, stWordBit, stWordBit&> mapVarInfo;   // key:变量名字, value: 目标变量字节偏移与位偏移 
   
// CTRDPServerDlg dialog

class CTRDPServerDlg : public CDialog
{
	// Construction
	public:
		CTRDPServerDlg(CWnd* pParent = NULL);	// standard constructor

		void ToCharArray(CString s,char a[],int n);
		bool ReflashList();

		void Recurse(LPCTSTR pstr);
		CString recvname;
		DWORD recvlen;

		void OnStartTRDP();
		DG_S32 trdp_pd_config();

		void getPort();
		void getFaultConfig(mapFaultInfo &mapInfo, CString strFileName, int iType);
		void getFaultConfig(mapVarInfo &mapInfo, CString strFileName);
        int getVarInfo(stVarInfo* stInfo, mapFaultInfo &fautInfo, mapVarInfo &varInfo);
		void getConfig();           // 获取TRDP变量 与MVB变量 的映射关系 配置
		unsigned char getETBN();  // 获取对面列车编组号;


		// Dialog Data
		//{{AFX_DATA(CTRDPServerDlg)
		enum { IDD = IDD_FTPSERVER_DIALOG };

		CEdit	  m_status;
		CListCtrl m_filelist;
		UINT      m_iport;
		CString	  m_TRDPReceive;

		//UINT m_nTestType;
		//UINT m_nTestPlace;
		//UINT m_nTestValue;
		//}}AFX_DATA

		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CTRDPServerDlg)
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
		//}}AFX_VIRTUAL

		// Implementation
	protected:
		HICON m_hIcon;

		// Generated message map functions
		//{{AFX_MSG(CTRDPServerDlg)
		virtual BOOL OnInitDialog();
		afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
		afx_msg void OnPaint();
		afx_msg HCURSOR OnQueryDragIcon();
		afx_msg void OnButtonStart();
		afx_msg void OnButton3();
		// afx_msg void OnButton2();
		afx_msg void OnTimer(UINT nIDEvent);
		//}}AFX_MSG

		afx_msg LONG OnThreadFinished(WPARAM wParam, LPARAM lParam);	//声明自定义消息函数
		DECLARE_MESSAGE_MAP()

	private:
			//CListenSocket m_listenSocket;
			//CList <CClientSocket*, CClientSocket*> m_lstSocket;
			CString filename[100];
			DWORD   filelen[100];
			CString filepath[100];
			int  filenum,num;
			CString m_strServerIP;
			UINT m_iServerPort;
			BOOL m_IsServerOn;
			CString m_strInfo;
			CListenSocket m_listenSocket;
			UINT m_MAXID;
			bool m_bStop;
			unsigned int m_ilen_comm;   
			unsigned int m_ilen_record; 
            mapFaultInfo m_mapFault1Info;  // 一二级故障 trdp 目标偏移，与 变量名
			mapFaultInfo m_mapFault3Info;   // 三级故障 trdp 目标偏移，与 变量名
			mapVarInfo m_mapVarInfo1;   // 1 2 级故障
			mapVarInfo m_mapVarInfo3;   // 3 级故障

            stVarInfo m_stVarFault1[5000];    // trdp 目标偏移以及 端口字节偏移、位偏移  1 2 级故障
            stVarInfo m_stVarFault3[5000];    // trdp 目标偏移以及 端口字节偏移、位偏移　３　级故障
			int m_iVarFault1Length;
			int m_iVarFault3Length;
};

/**
 * @brief    -- FTP 文件相关
 */
class CFTPFunc : public CWnd
{
	public:
		CFTPFunc();
		virtual ~CFTPFunc();

		CInternetSession *m_pInetSession;
		CFtpConnection   *m_pFtpConnection;

		void BrowseLocalFile();
		void DisConnect();
		void findFileLocal(mapFileInfo &mapFileLocal, CString strDir); 
		void findFileFTPServer(mapFileInfo &mapFileServer, CString strDir);

		bool checkFileWaitUpload(listFileInfo &listUploadFile);
		bool checkFileWaitRemove(listFileInfo &listRemoveFile);

		BOOL IsConnectToFtpServer;
		bool UploadFile(CString filenamestr, CString strRemoteFile);
		bool removeFile(CString strRemoteFile);
		BOOL ConnectToServer(LPCTSTR ServerIP, LPCTSTR name, LPCTSTR Password, INTERNET_PORT port);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FTPSERVERDLG_H__219C9D6F_D033_494C_A407_B4C40EA2E553__INCLUDED_)

