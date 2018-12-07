// FTPServer.h : main header file for the FTPSERVER application
//

#if !defined(AFX_FTPSERVER_H__BE5C1896_A5A3_4F1F_91F0_66E395D3ED2C__INCLUDED_)
#define AFX_FTPSERVER_H__BE5C1896_A5A3_4F1F_91F0_66E395D3ED2C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTRDPServerApp:
// See FTPServer.cpp for the implementation of this class
//

class CTRDPServerApp : public CWinApp
{
public:
	CTRDPServerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTRDPServerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTRDPServerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FTPSERVER_H__BE5C1896_A5A3_4F1F_91F0_66E395D3ED2C__INCLUDED_)
