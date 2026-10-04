# Microsoft Developer Studio Project File - Name="Viewer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Viewer - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ViewerRelease.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ViewerRelease.mak" CFG="Viewer - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Viewer - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Viewer - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 1
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Viewer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Viewer___Win32_Release"
# PROP BASE Intermediate_Dir "Viewer___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../../Bin/win32"
# PROP Intermediate_Dir "ReleasePersonal"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /Ob2 /I "../../../Foundation/src/include" /I "." /I "../../src" /I "../../../Graphics/include" /I "..\..\..\SDKs\Foundation\include" /D "NX_FOUNDATION_DLL_USER" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MT /W3 /GX /O2 /Ob2 /I "." /I "../../src" /I "../../../Graphics/include" /I "../../../Graphics/include/win32" /I "..\..\..\SDKs\Foundation\include" /I "..\..\..\SDKs\Physics\includePersonal" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib OpenGL32.lib glu32.lib /nologo /subsystem:console /machine:I386 /out:"../../bin/win32/Release/NovodeXPersonal.exe"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib OpenGL32.lib glu32.lib /nologo /subsystem:console /machine:I386 /out:"../../../Bin/win32/NovodeXPersonal.exe"
# SUBTRACT LINK32 /debug /nodefaultlib

!ELSEIF  "$(CFG)" == "Viewer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Viewer___Win32_Debug"
# PROP BASE Intermediate_Dir "Viewer___Win32_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../../Bin/win32/"
# PROP Intermediate_Dir "DebugPersonal"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /GX /O2 /Ob2 /I "../../../Foundation/src/include" /I "." /I "../../src" /I "../../../Graphics/include" /I "..\..\..\SDKs\Foundation\include" /D "NX_FOUNDATION_DLL_USER" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MTd /W3 /GX /Zi /Od /Ob2 /I "." /I "../../src" /I "../../../Graphics/include" /I "../../../Graphics/include/win32" /I "..\..\..\SDKs\Foundation\include" /I "..\..\..\SDKs\Physics\includePersonal" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib OpenGL32.lib glu32.lib /nologo /subsystem:console /machine:I386 /out:"../../bin/win32/Release/NovodeXPersonal.exe"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib OpenGL32.lib glu32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../../Bin/win32/NovodeXPersonalDEBUG.exe"
# SUBTRACT LINK32 /incremental:yes

!ENDIF 

# Begin Target

# Name "Viewer - Win32 Release"
# Name "Viewer - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter ""
# Begin Group "Act"

# PROP Default_Filter ""
# Begin Group "Controllers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\ActController.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ActController.h
# End Source File
# Begin Source File

SOURCE=..\..\src\CarController.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\CarController.h
# End Source File
# Begin Source File

SOURCE=..\..\src\CloneController.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\CloneController.h
# End Source File
# Begin Source File

SOURCE=..\..\src\FlyController.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FlyController.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\Act.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Act.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ActActor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ActActor.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ActEffector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ActEffector.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ActJoint.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ActJoint.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ActPicking.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ActRecorder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ActRecorder.h
# End Source File
# Begin Source File

SOURCE=..\..\src\CollisionMeshAdapter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\CollisionMeshAdapter.h
# End Source File
# Begin Source File

SOURCE=..\..\src\DebugRenderer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\DebugRenderer.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\CorpusDemoGraphic.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FPSCounter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FPSCounter.h
# End Source File
# Begin Source File

SOURCE=..\..\src\PerfDisplay.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\PerfDisplay.h
# End Source File
# Begin Source File

SOURCE=..\..\src\PhysicsDemo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\PhysicsDemo.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ShapeVis.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ShapeVis.h
# End Source File
# Begin Source File

SOURCE=..\..\src\TextSpooler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\TextSpooler.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ViewerGraphicsContext.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ViewerGraphicsContext.h
# End Source File
# End Group
# Begin Group "vc6.win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\Novodex.rc
# End Source File
# End Group
# Begin Group "Foundation SDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDKs\Foundation\lib\win32\Release\NxFoundation.lib
# End Source File
# End Group
# Begin Group "PhysicsSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDKs\Physics\lib\win32\Release\NxPhysicsPersonal.lib
# End Source File
# End Group
# Begin Group "Graphics Lib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Graphics\lib\win32\glut\glut32.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\Graphics\lib\win32\DevIL\ILUT.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\Graphics\lib\win32\DevIL\il_wrap.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\Graphics\lib\win32\DevIL\ILU.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\Graphics\lib\win32\DevIL\DevIL.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\Graphics\lib\win32\Release\GraphicsLib.lib
# End Source File
# End Group
# Begin Source File

SOURCE=.\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\icon2.ico
# End Source File
# End Target
# End Project
