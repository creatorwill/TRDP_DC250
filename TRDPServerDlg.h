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

/******************************** ʹ���߳� *************************************/
// #define WM_USER_THREAD_FINISHED WM_USER + 0x100	// �Զ�����Ϣ

UINT ThreadTrdpDataProcess(LPVOID lpParam); // ���� {TRDP���ݴ���} �̺߳���
UINT ThreadFileSync(LPVOID lpParam);        // ���� {FTP�ļ�ͬ��} �̺߳���
UINT ThreadFileRecord(LPVOID lpParam);      // ���� {�ļ���¼} �̺߳���

/******************************** ʹ���߳� ************************************/
typedef union {
	unsigned int iCounter;
	unsigned char cCounter[4];
} unCounter;


typedef CMap<CString, LPCTSTR, CString, LPCTSTR> mapFileInfo; // �洢�ļ��б� �ļ���, �ļ���С
typedef CList<CString ,CString&> listFileInfo;   // ��ͬ���ļ���Ϣ

typedef struct _ST_TRDP_DATA_INFO {
	long     lPort;           // �˿ں�
	unsigned int portSize;    // �˿������ݴ�С
	unsigned char data[1500]; // ��������
} stTrdpDataInfo;             // TRDP������Ϣ

typedef struct {
	unsigned int destNum;     // Ŀ������  �ֽ�
	int srcNum;               // Դ��������  �ֽ�
	unsigned char bitNum;     // �ֽ��а��� λ��Ϣ �ĸ���  <= 8;

	struct ST_POS_INFO {
		CString  varType;    // ���������� 
		unsigned char bit;   // ��/λƫ��
	} srcPos[15];            // Դ����[�ֽ�] ����λ ,����Ҫ��һ�� ;

} stComInfo;                 // �˿����ֽڵ�����

typedef struct {
	int comId;                // �˿ں�
	int portSize;             // �˿ڴ�С
	int counter;              // �˿��� �������������ݵĸ���
	stComInfo varInfo[5000];  // �˿��� �����ĸ���   ���� < 1500  : 1001 -> 18000 ����1001�˿��������࣬��ֵ��Ϊ5000
} stConfigInfo;               // �˿�������Ϣ

typedef struct {
	CString  trdpip;
	// CHAR8 *  trdpip;
	int  comid;               // �˿ں�
	unsigned int portsize;    // �˿ڴ�С�������С
	int cycle;                // ���ڣ�����Դ�˿ڷ���������Ҫ�ò���
	unsigned int direction;   // ���ݷ��򣬷��ͻ���գ�����߶���д���� ��PUBLISH = 0, SUBSCRIBE
	int byteofsetstart;
	int byteofsetend;
	int datalen;
} stTrdpPortPara;   // �˿�����������Ϣ

/* �ֽ�ƫ�ƣ�λƫ�� */
typedef struct ST_TYPE {
    int iWordOffset;
    int iBitOffset;
} stWordBit;

/* trdp������Ҫ�洢��Ŀ���ַ��trdp���ݵ��֡�λƫ�� */
typedef struct ST_WORD_INFO{
    int iDestNum;
    stWordBit stOffset;
} stVarInfo;

typedef CMap<int, int&, CString, LPCTSTR> mapFaultInfo;             // key:Ŀ������ֽ�ƫ��ֵ�� value: ��������
typedef CMap<CString, LPCTSTR, stWordBit, stWordBit&> mapVarInfo;   // key:��������, value: Ŀ������ֽ�ƫ����λƫ�� 
   
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
		void getConfig();           // ��ȡTRDP���� ��MVB���� ��ӳ���ϵ ����
		unsigned char getETBN();  // ��ȡ�����г������;


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

		afx_msg LONG OnThreadFinished(WPARAM wParam, LPARAM lParam);	//�����Զ�����Ϣ����
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
            mapFaultInfo m_mapFault1Info;  // һ�������� trdp Ŀ��ƫ�ƣ��� ������
			mapFaultInfo m_mapFault3Info;   // �������� trdp Ŀ��ƫ�ƣ��� ������
			mapVarInfo m_mapVarInfo1;   // 1 2 ������
			mapVarInfo m_mapVarInfo3;   // 3 ������

            stVarInfo m_stVarFault1[5000];    // trdp Ŀ��ƫ���Լ� �˿��ֽ�ƫ�ơ�λƫ��  1 2 ������
            stVarInfo m_stVarFault3[5000];    // trdp Ŀ��ƫ���Լ� �˿��ֽ�ƫ�ơ�λƫ�ơ�����������
			int m_iVarFault1Length;
			int m_iVarFault3Length;
};

/**
 * @brief    -- FTP �ļ����
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

