@echo off
if not defined DevEnvDir (
	call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)
if not defined DevEnvDir (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)

if "%Platform%" neq "x64" (
    echo ERROR: Platform is not "x64" - previous bat call failed.
    exit /b 1
)

set COMPUTEHADER=ComputeShader.hlsl
set FILES=main.cpp

set RELEASEFLAGS=/O2 /DMAIN_DEBUG=0 /DRUNTIME_DEBUG_COMPILE=0 /DCOMPILED_DEBUG_CSO=0
set DEBUGFLAGS=/Zi /DMAIN_DEBUG=1 /DRUNTIME_DEBUG_COMPILE=0 /DCOMPILED_DEBUG_CSO=0

set LIBS=d3d12.lib dxgi.lib d3dcompiler.lib dxguid.lib kernel32.lib user32.lib gdi32.lib

::TODO does dxc compiler produce better performing shader code?

::Release
fxc /nologo /T cs_5_0 /O3 /WX  /Qstrip_reflect /Qstrip_debug /Qstrip_priv %COMPUTEHADER% /Fh computeShader.h /Vn computeShaderBlob
cl /nologo /W3 /GS- /Gs999999 /arch:AVX2 %RELEASEFLAGS% %FILES% /Fe: FPSCameraBasic.exe %LIBS% /link /incremental:no /opt:icf /opt:ref /subsystem:console

::Debug
fxc /nologo /T cs_5_0 /Zi /WX %COMPUTEHADER% /Fh computeShaderDebug.h /Vn computeShaderBlob
cl /nologo /W3 /GS- /Gs999999 /arch:AVX2 %DEBUGFLAGS% %FILES% /FC /Fe: FPSCameraBasicDebug.exe %LIBS% /link /incremental:no /opt:icf /opt:ref /subsystem:console
