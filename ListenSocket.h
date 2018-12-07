#if !defined(AFX_LISTENSOCKET_H__0BAD43B8_48F7_4211_87D3_20B1B954BD0F__INCLUDED_)
#define AFX_LISTENSOCKET_H__0BAD43B8_48F7_4211_87D3_20B1B954BD0F__INCLUDED_

//#include "FTPServerDlg.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ListenSocket.h : header file


/////////////////////////////////////////////////////////////////////////////
// CListenSocket command target
class CTRDPServerDlg;
class CListenSocket : public CAsyncSocket
{
// Attributes
public:

// Operations
public:
	CListenSocket();
	virtual ~CListenSocket();

// Overrides
public:
	void SetParent(CTRDPServerDlg *pParent);
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListenSocket)
	public:
	virtual void OnAccept(int nErrorCode);
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CListenSocket)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

// Implementation
protected:
	CTRDPServerDlg *m_wndParent;
private:
	
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTENSOCKET_H__0BAD43B8_48F7_4211_87D3_20B1B954BD0F__INCLUDED_)
