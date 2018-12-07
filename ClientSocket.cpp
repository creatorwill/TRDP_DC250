// ClientSocket.cpp : implementation file
//

#include "stdafx.h"
#include "TRDPServer.h"
#include "ClientSocket.h"
#include  "TRDPServerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClientSocket

CClientSocket::CClientSocket()
{
	m_wndParent = NULL;


}

CClientSocket::~CClientSocket()
{
}


// Do not edit the following lines, which are needed by ClassWizard.
// BEGIN_MESSAGE_MAP(CClientSocket, CAsyncSocket)
	//{{AFX_MSG_MAP(CClientSocket)
	//}}AFX_MSG_MAP

// END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClientSocket member functions

void CClientSocket::OnReceive(int nErrorCode) 
{
	// TODO: Add your specialized code here and/or call the base class
	if(m_wndParent)
	{
	    // m_wndParent->OnReceive(this);
	}
	CAsyncSocket::OnReceive(nErrorCode);
}



void CClientSocket::SetParent(CTRDPServerDlg *pParent)
{
m_wndParent=pParent;
}

#if 0
void FTPTransfer::Upload(LPCTSTR pstrLocalFile, LPCTSTR pstrRemoteFile)
{
	FTPTransfer transfer;
	transfer.m_csLocalDir = url + strImgNo.c_str();
	transfer.m_astrAllDirName.Add(csImgNo);
	try { 
		if (!transfer.UploadAll()) {
			CString strlog(_T("[threadOperationTransfer]: "));
			ServiceLog.write_log("[threadOperationTransfer]: ERROR! Upload error.");
			CString csError = transfer.GetLastError();
			ServiceLog.write_log((LPCWSTR)(strlog+ csError));
			return -1; // FTP Error;
		}
	} catch (...) {
		CString strlog(_T("[threadOperationTransfer]: "));
		ServiceLog.write_log("[threadOperationTransfer]: ERROR! upload except:");
		CString csError = transfer.GetLastError();
		ServiceLog.write_log((LPCWSTR)(strlog+ csError));
		return -1;
	}
}


UINT CFtpThread::CFtpThread()
{
	m_bConnectionAttempted = false;

	m_pInetSession = new CInternetsession(AfxGetAppName(), 1, PRE_CONFIG_INTERNET_ACCESS);

	try {
		m_pFtpConnection = m_pInetsession->GetFtpConnection("FTP.MICROSOFT.COM");
	} catch(CInternetException *pEx) {
		TCHAR szError[1024];

		if (pEx->GetErrorMessage(szError, 1024)) {
			AfxMessageBox(szError);
		} else {
			AfxMessageBox("There was an exception");
		}

		pEx->Delete();
		m_pFtpConnection = NULL;
	}
}

CFtpThread::~CFtpThread()
{
	if (m_pFtpConnection != NULL) {
		m_pFtpConnection->Close();
		delete m_pFtpConnection;
	}

	delete m_pInetsession;
}

//该函数实现从server下载文件
int GetMultipleFile(CStringArray *remoteArray, CStringArray *localArray, int number_file)
{
	// init some var
	BOOL goodfile;
	int x=0;
	int nb_lost_file =0;

	// while loop to transfer every file in the array
	while(x) {
		// try to get file
		goodfile = pFtpConnection->GetFile( remoteArray->GetAt(x), localArray->GetAt(x),
				FALSE,
				FILE_ATTRIBUTE_NORMAL,
				FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE
				);

		missed[x] = goodfile ? 0 : 1;
		// if failed, missed[x] become 1
		// if good, missed become 0
		if(missed[x])
			nb_lost_file++;
		// if the file was missed, increase the number of
		// missing file.
		// increase to the next file
		x++;
	}
	//return the number of missing file, if any.
	return nb_lost_file;
}

UINT CFtpThread::PutFile(LPVOID Status)
{
	int *pnFileStatus;
	CInternetSession *pInetsession;
	CFtpConnection *pFtpConnection=NULL;
	pnFileStatus=(int *)Status;
	*pnFileStatus=0;
	pInetsession=new CInternetsession(AfxGetAppName(), 1, PRE_CONFIG_INTERNET_ACCESS);

	try {
		pFtpConnection=pInetsession->GetFtpConnection("192.34.45.0");
	} catch(CInternetException *pEx) {
		pEx->Delete();
		pFtpConnection=NULL;
		*pnFileStatus=-1;

		goto BallOut;
	}

	*pnFileStatus = 1;

pFtpConnection->Remove("test.txt");

	if (!pFtpConnection->PutFile("test.txt","test.txt")) {
		*pnFileStatus=-2;
	} else {
		*pnFileStatus=2;
	}

BallOut:

	if (pFtpConnection != NULL) {
		pFtpConnection->Close();
		delete pFtpConnection;
	}

	delete pInetsession;
	AfxEndThread(0);
	return false;
}
#endif