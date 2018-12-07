#if !defined(AFX_CLIENTSOCKET_H__DB062E2F_CC45_41BF_BFB7_DBA016F928F5__INCLUDED_)
#define AFX_CLIENTSOCKET_H__DB062E2F_CC45_41BF_BFB7_DBA016F928F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// ClientSocket.h : header file
#include <afxinet.h> // for ftp api functions


/////////////////////////////////////////////////////////////////////////////
// CClientSocket command target
class CTRDPServerDlg;

class CClientSocket:public CAsyncSocket
{
	// Construction
	public:
		//CDemoDlg(CWnd* pParent = NULL);	// standard constructor
	protected:
		HICON m_hIcon;
		//afx_msg void OnBtnTest();


	// Attributes
	public:
		// Operations
	public:
		CClientSocket();
		virtual ~CClientSocket();
		// Overrides
	public:
		void SetParent(CTRDPServerDlg *pParent);
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CClientSocket)

	public:
		virtual void OnReceive(int nErrorCode);
		//}}AFX_VIRTUAL
		// Generated message map functions
		//{{AFX_MSG(CClientSocket)
		// NOTE - the ClassWizard will add and remove member functions here.
		//}}AFX_MSG
		// Implementation
	protected:
	
		//BEGIN_MESSAGE_MAP();
	private:
		CTRDPServerDlg *m_wndParent;
};
/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_CLIENTSOCKET_H__DB062E2F_CC45_41BF_BFB7_DBA016F928F5__INCLUDED_)
