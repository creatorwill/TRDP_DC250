; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=
LastTemplate=CAsyncSocket
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "FTPServer.h"

ClassCount=4
Class1=CFTPServerApp
Class2=CClientSocket
Class3=CAboutDlg

ResourceCount=3
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Class4=CListenSocket
Resource3=IDD_TRDPSERVER_DIALOG

[CLS:CFTPServerApp]
Type=0
HeaderFile=FTPServer.h
ImplementationFile=FTPServer.cpp
Filter=N

[CLS:CAboutDlg]
Type=0
HeaderFile=FTPServerDlg.h
ImplementationFile=FTPServerDlg.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[CLS:CListenSocket]
Type=0
HeaderFile=ListenSocket.h
ImplementationFile=ListenSocket.cpp
BaseClass=CAsyncSocket
Filter=N
VirtualFilter=q

[CLS:CClientSocket]
Type=0
HeaderFile=ClientSocket.h
ImplementationFile=ClientSocket.cpp
BaseClass=CAsyncSocket
Filter=N
VirtualFilter=q

[DLG:IDD_TRDPSERVER_DIALOG]
Type=1
Class=?
ControlCount=6
Control1=IDC_EDIT2,edit,1350633604
Control2=IDC_LIST1,SysListView32,1350631425
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_EDIT3,edit,1352728580
Control6=IDC_BUTTON3,button,1342242816

