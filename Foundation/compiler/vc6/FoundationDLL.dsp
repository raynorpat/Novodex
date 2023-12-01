# Microsoft Developer Studio Project File - Name="FoundationDLL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=FoundationDLL - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FoundationDLL.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FoundationDLL.mak" CFG="FoundationDLL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FoundationDLL - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "FoundationDLL - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "FoundationDLL"
# PROP Scc_LocalPath "..\.."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FoundationDLL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "FoundationDLL___Win32_Release"
# PROP BASE Intermediate_Dir "FoundationDLL___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../lib/win32/Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FOUNDATIONDLL_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /Zi /O2 /Ob2 /I "../../include" /I "../../src/include" /D "NDEBUG" /D "NX_FOUNDATION_DLL" /D "WIN32" /YX /FD /QIfist /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 /nologo /dll /map /debug /machine:I386 /out:"../../lib/win32/Release/NxFoundation.dll"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=copy to Bin\win32 dir to make accessible to apps
PostBuild_Cmds=copy ..\..\lib\win32\Release\NxFoundation.dll ..\..\..\..\Bin\win32
# End Special Build Tool

!ELSEIF  "$(CFG)" == "FoundationDLL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "FoundationDLL___Win32_Debug"
# PROP BASE Intermediate_Dir "FoundationDLL___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../lib/win32/Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe -lib
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FOUNDATIONDLL_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /Zi /Od /I "../../include" /I "../../src/include" /D "NX_USER_DEBUG_MODE" /D "_DEBUG" /D "NX_FOUNDATION_DLL" /D "WIN32" /Fr /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /dll /incremental:no /debug /machine:I386 /out:"../../lib/win32/Debug/NxFoundationDEBUG.dll" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=copy to Bin\win32 dir to make accessible to apps
PostBuild_Cmds=copy ..\..\lib\win32\Debug\NxFoundationDEBUG.dll ..\..\..\..\Bin\win32
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "FoundationDLL - Win32 Release"
# Name "FoundationDLL - Win32 Debug"
# Begin Group "include"

# PROP Default_Filter ""
# Begin Group "Shapes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\NxBounds3.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxBox.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxCapsule.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxPlane.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxRay.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxSegment.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxSphere.h
# End Source File
# End Group
# Begin Group "Matrices"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\Nx12F32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Nx12F64.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Nx16F32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Nx16F64.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Nx4F32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Nx4F64.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Nx9F32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Nx9F64.h
# End Source File
# End Group
# Begin Group "Allocator"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\NxAllocateable.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxAllocatorDefault.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxUserAllocator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxUserAllocatorAccess.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxUserAllocatorDefault.h
# End Source File
# End Group
# Begin Group "Math"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\NxFPU.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxMat33.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxMat34.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxMath.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxQuat.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxVec3.h
# End Source File
# End Group
# Begin Group "Container"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\NxArray.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxBitfield.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxList.h
# End Source File
# End Group
# Begin Group "Basics"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\Nx.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxAssert.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxBlank.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxException.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Nxf.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxSimpleTypes.h
# End Source File
# End Group
# Begin Group "DebugRender"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\NxDebugRenderable.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxUserDebugRenderer.h
# End Source File
# End Group
# Begin Group "Mesh"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\NxSimpleTriangleMesh.h
# End Source File
# End Group
# Begin Group "Utilities"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\NxProfiler.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxQuickSort.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxUtilities.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxVolumeIntegration.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\include\NxFoundation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxFoundationSDK.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NxUserOutputStream.h
# End Source File
# End Group
# Begin Group "src"

# PROP Default_Filter ""
# Begin Group "Shapes."

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\Box.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Capsule.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Ray.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Segment.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Sphere.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\include\CustomAssert.h
# End Source File
# Begin Source File

SOURCE=..\..\src\DebugRenderable.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\include\DebugRenderable.h
# End Source File
# Begin Source File

SOURCE=..\..\src\DefaultAllocator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\include\DefaultAllocator.h
# End Source File
# Begin Source File

SOURCE=..\..\src\DefaultTriangleMesh.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\include\DefaultTriangleMesh.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Exception.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\include\Exception.h
# End Source File
# Begin Source File

SOURCE=..\..\src\FoundationSDK.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\include\FoundationSDK.h
# End Source File
# Begin Source File

SOURCE=..\..\src\FPU.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Observable.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\include\Observable.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Profiler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\include\STLAllocator.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Time.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\include\Time.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Utilities.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\VolumeIntegration.cpp
# End Source File
# End Group
# Begin Group "vs6"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\..\Viewer\compiler\vc6\icon1.ico
# End Source File
# End Target
# End Project
