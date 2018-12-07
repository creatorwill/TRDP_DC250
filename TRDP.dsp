# Microsoft Developer Studio Project File - Name="TRDPServer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=TRDPServer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TRDP.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TRDP.mak" CFG="TRDPServer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TRDPServer - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "TRDPServer - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TRDPServer - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /ZI /Od /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 version.lib Win32TRDP_DLL.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Build/TRDPͨѶ.exe"

!ELSEIF  "$(CFG)" == "TRDPServer - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib Win32TRDP_DLL.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "TRDPServer - Win32 Release"
# Name "TRDPServer - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ClientSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\ListenSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\sharememoryclient.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TRDPServer.cpp
# End Source File
# Begin Source File

SOURCE=.\TRDPServer.rc
# End Source File
# Begin Source File

SOURCE=.\TRDPServerDlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "TRDP"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\TRDP\Markup.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\mat.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\matrix.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\mclbase.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\mclcppclass.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\mclmcr.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\mclmcrrt.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\os_def.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\pthread.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\sched.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\tau_marshall.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\tau_xml.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\tmwtypes.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\trdp_if.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\trdp_if_light.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\trdp_mdcom.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\trdp_pdcom.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\trdp_private.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\trdp_proto.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\trdp_stats.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\trdp_types.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\trdp_utils.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\vos_mem.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\vos_sock.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\vos_thread.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\vos_types.h
# End Source File
# Begin Source File

SOURCE=.\TRDP\vos_utils.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ClientSocket.h
# End Source File
# Begin Source File

SOURCE=.\ListenSocket.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\sharememoryclient.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TRDPServer.h
# End Source File
# Begin Source File

SOURCE=.\TRDPServerDlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\FTPServer.ico
# End Source File
# Begin Source File

SOURCE=.\res\TRDPServer.ico
# End Source File
# Begin Source File

SOURCE=.\res\TRDPServer.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
