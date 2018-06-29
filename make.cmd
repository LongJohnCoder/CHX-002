@REM ## @file  
@REM #
@REM # Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
@REM # All rights reserved.This software and associated documentation (if any)
@REM # is furnished under a license and may only be used or copied in 
@REM # accordance with the terms of the license. Except as permitted by such
@REM # license, no part of this software or documentation may be reproduced, 
@REM # stored in a retrieval system, or transmitted in any form or by any 
@REM # means without the express written consent of Byosoft Corporation.
@REM # 
@REM # File Name:
@REM #   make.bat  
@REM # 
@REM # Abstract: 
@REM #   build command batch file to complete the build process.
@REM #
@REM # Revision History:
@REM #

@echo off
SET PLATFORM_PACKAGE=PlatformPkg

set BOARD_ID=HX002EA0
set BOARD_REV=1
set OEM_ID=CRB
set ZXCPU_TYPE=ACpu
set ZXCHIP_TYPE=CHX002
set ZX_IOETYPE= 

call SetBiosVer.cmd

REM ----------------------------------------------------------------------------
REM ******************** make para #1: Debug or realse flag ********************
REM Get "TARGET"
set SKIP_AUTOGEN=
if /I "%1" == "d" (
  SET TARGET=DEBUG
) else if /I "%1" == "dd" (
  SET TARGET=DEBUG
  set SKIP_AUTOGEN=TRUE
) else if /I "%1" == "r" (
  SET TARGET=RELEASE
) else if /I "%1" == "clean" (
  goto DoBuildClean 
) else (
  goto ERROR
)

@if "%TARGET%" == "DEBUG" (
  @SET  BUILD_TYPE=D
) else (
  @SET  BUILD_TYPE=R
)

REM ************************* make para #2: Tool chain *************************
REM Get "TOOL_CHAIN_TAG"
set TOOL_CHAIN_TAG=VS2008x86
if /I "%2" == "2003" (
  set TOOL_CHAIN_TAG=VS2003
) else if /I "%2" == "2005" (
  set TOOL_CHAIN_TAG=VS2005
) else if /I "%2" == "2008" (
  set TOOL_CHAIN_TAG=VS2008
) else if /I "%2" == "2008x" (
  set TOOL_CHAIN_TAG=VS2008x86
) else if /I "%2" == "ddk" (
  set TOOL_CHAIN_TAG=DDK3790
) else (
  echo Default TOOL_CHAIN_TAG = %TOOL_CHAIN_TAG%  
)

REM *************** make para #3: Board and PCIECFG ***************
REM ********** EA0 **********
if /I "%3" == "HX002EA0_03" (
  set BOARD_ID=HX002EA0
  set ZX_MBTYPE=%3
  set ZX_MBPCIECFG=03

REM ********** EB0 **********
)else if /I "%3" == "HX002EB0_00" (
  set BOARD_ID=HX002EB0
  set ZX_MBTYPE=%3
  set ZX_MBPCIECFG=00
)else if /I "%3" == "HX002EB0_11" (
  set BOARD_ID=HX002EB0
  set ZX_MBTYPE=%3
  set ZX_MBPCIECFG=11

REM ********** EC0 **********
)else if /I "%3" == "HX002EC0_01" (
  set BOARD_ID=HX002EC0
  set ZX_MBTYPE=%3
  set ZX_MBPCIECFG=01
)else if /I "%3" == "HX002EC0_10" (
  set BOARD_ID=HX002EC0
  set ZX_MBTYPE=%3
  set ZX_MBPCIECFG=10

REM ********** ED0 **********
)else if /I "%3" == "HX002ED0_02" (
  set BOARD_ID=HX002EC0
  set ZX_MBTYPE=%3
  set ZX_MBPCIECFG=02
)else if /I "%3" == "HX002ED0_10" (
  set BOARD_ID=HX002ED0
  set ZX_MBTYPE=%3
  set ZX_MBPCIECFG=10

REM ********** EE0 **********
)else if /I "%3" == "HX002EE0_04" (
  set BOARD_ID=HX002EE0
  set ZX_MBTYPE=%3
  set ZX_MBPCIECFG=04
)else if /I "%3" == "HX002EE0_05" (
  set BOARD_ID=HX002EE0
  set ZX_MBTYPE=%3
  set ZX_MBPCIECFG=05

REM ********** EH0 **********
)else if /I "%3" == "HX002EH0_01" (
  set BOARD_ID=HX002EH0
  set ZX_MBTYPE=%3
  set ZX_MBPCIECFG=01

)else (
  echo Please Input HX002EA0 or HX002EB0 etc
  goto ERROR
)

REM ************ make para #4: CHX002_A0 ************
if /I "%4" == "CHX002_A0" (
  REM ---
) else (
  echo Please Input CHX002_A0 etccc
  goto ERROR
)

REM ************ make para #5: IOE exist or not ************
if /I "%5" == "IOE_EXIST" (
  set ZX_IOETYPE=%6
  REM ---
)

REM ----------------------------------------------------------------------------
REM edksetup.bat will set some basic variable, so invoke it as soon as possible.
@call edksetup.bat
@echo off

rem del original romsip file
set "ORG_ROMSIP_PATH=.\PlatformPkg\IA32FamilyCpuPkg\SecCore\Ia32"
if exist %ORG_ROMSIP_PATH%\ResetVec.asm16 (
  echo Original ROMSIP file exist! begin to del it
  del %ORG_ROMSIP_PATH%\ResetVec.asm16
)

set "ROMSIP_PATH=.\AsiaPkg\ZxPlatformRomsip"
rem ROMSIP file must exist
if exist %ROMSIP_PATH%\ResetVec.asm16 (
  echo Begin to copy ROMSIP file!
  copy %ROMSIP_PATH%\ResetVec.asm16 %ORG_ROMSIP_PATH%
)


set "USB_FW_PATH=.\AsiaPkg\ZxPlatformBin\Usb"
rem xhci firmware file must exist
if not exist %USB_FW_PATH%\CHX002_xHCI_R*.bin (
  echo xHCI FW file not exist!
  goto ERROR
)
rem extract xhci firmware version
set "XHCI_FW_PREFIX="
set "XHCI_FW_VER="
set "XHCI_FW_TYPE="
set "XHCI_FW_PATH_FILE_NAME="
for /f "tokens=1,2,3,4 delims=_." %%a in ('dir /b %USB_FW_PATH%') do (
  if /i "%%b" == "xhci" (
    if /i "%%a" == "CHX002" (
      set "XHCI_FW_PREFIX=%%a"
      set "XHCI_FW_VER=%%c"
      set "XHCI_FW_TYPE=%%d"
      set "XHCI_FW_PATH_FILE_NAME=%%a_%%b_%%c_%%d.bin"
    )
  )
)

REM set "XHCI_FW_VER=%XHCI_FW_VER:~1%"
REM echo %XHCI_FW_PREFIX%
REM echo %XHCI_FW_VER%
REM echo %XHCI_FW_TYPE%
REM pause

set "IOE_USB_FW_PATH=.\AsiaPkg\ZxPlatformBin"
if /i "%5" == "IOE_EXIST" (
  rem xhci firmware file must exist
  if not exist %IOE_USB_FW_PATH%\CND003_xHCI_R*.bin (
    echo IOE xHCI FW file not exist!
    goto ERROR
  )
  rem extract xhci firmware version
  set "IOE_XHCI_FW_PREFIX="
  set "IOE_XHCI_FW_VER="
  set "IOE_XHCI_FW_TYPE="
  set "IOE_XHCI_FW_PATH_FILE_NAME="
  for /f "tokens=1,2,3,4 delims=_." %%a in ('dir /b %IOE_USB_FW_PATH%') do (
    if /i "%%b" == "xhci" (
      if /i "%%a" == "CND003" (
        set "IOE_XHCI_FW_PREFIX=%%a"
        set "IOE_XHCI_FW_VER=%%c"
        set "IOE_XHCI_FW_TYPE=%%d"
        set "IOE_XHCI_FW_PATH_FILE_NAME=%%a_%%b_%%c_%%d.bin"
      )
    )
  )

  REM set "IOE_XHCI_FW_VER=%IOE_XHCI_FW_VER:~1%"
  REM echo %IOE_XHCI_FW_PREFIX%
  REM echo %IOE_XHCI_FW_VER%
  REM echo %IOE_XHCI_FW_TYPE%
  REM pause
)

if defined PLAT_TOOL_PATH goto PLAT_TOOL_PATH_SET
set PLAT_TOOL_PATH=%WORKSPACE%\%PLATFORM_PACKAGE%\Tools
set PATH=%PLAT_TOOL_PATH%;%PATH%
:PLAT_TOOL_PATH_SET

@set ECP_SOURCE=
@set EFI_SOURCE=
@set EDK_SOURCE=
@set OUTPUT_DIR=%WORKSPACE%\Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%

@if not exist %OUTPUT_DIR%\X64 (
  mkdir %OUTPUT_DIR%\X64
)

if /I "%SKIP_AUTOGEN%" == "TRUE" (
  goto READY_TO_BUILD
)

REM AutoGen Files
set AUTOGEN_TARGET_FILE=%PLATFORM_PACKAGE%\BiosId.bin
echo Generate %AUTOGEN_TARGET_FILE%
%WORKSPACE%\%PLATFORM_PACKAGE%\Tools\Win32Tool.exe -biosid %BOARD_ID% %BOARD_REV% %OEM_ID% %BUILD_TYPE% %BIOS_MAJOR_VER%%BIOS_MINOR_VER% %AUTOGEN_TARGET_FILE% 
if "%ERRORLEVEL%" NEQ "0" goto :ERROR

set AUTOGEN_TARGET_FILE=%PLATFORM_PACKAGE%\AutoBuildTime.h
echo Generate %AUTOGEN_TARGET_FILE%
%WORKSPACE%\%PLATFORM_PACKAGE%\Tools\Win32Tool.exe -bit %PLATFORM_PACKAGE%\BiosId.bin > %AUTOGEN_TARGET_FILE%
if "%ERRORLEVEL%" NEQ "0" goto :ERROR

%WORKSPACE%\%PLATFORM_PACKAGE%\Tools\Win32Tool.exe -bit %PLATFORM_PACKAGE%\BiosId.bin bat > %OUTPUT_DIR%\BiosBuildTime.bat
call %OUTPUT_DIR%\BiosBuildTime.bat

set AUTOGEN_TARGET_FILE=%PLATFORM_PACKAGE%\BiosVersion.h
echo // FILE AUTO GENERATED BY make.cmd, DO NOT EDIT... > %AUTOGEN_TARGET_FILE%
echo #ifndef __BIOS_NAME_H__          >> %AUTOGEN_TARGET_FILE%
echo #define __BIOS_NAME_H__          >> %AUTOGEN_TARGET_FILE%
echo.                                 >> %AUTOGEN_TARGET_FILE%
echo #define TKN_BIOS_MAJOR_VER  %BIOS_MAJOR_VER% >> %AUTOGEN_TARGET_FILE%
echo #define TKN_BIOS_MINOR_VER  %BIOS_MINOR_VER% >> %AUTOGEN_TARGET_FILE%
echo #define TKN_BIOS_VER        %BIOS_MAJOR_VER%%BIOS_MINOR_VER% >> %AUTOGEN_TARGET_FILE%
REM echo #define TKN_BIOSID_NAME     %BOARD_ID%%BOARD_REV%.%OEM_ID%.%BUILD_TYPE%%BIOS_MAJOR_VER%%BIOS_MINOR_VER% >> %AUTOGEN_TARGET_FILE%
echo #define TKN_BIOSID_NAME     %ZX_MBTYPE%_%BUILD_TYPE%%BIOS_MAJOR_VER%%BIOS_MINOR_VER% >> %AUTOGEN_TARGET_FILE%
echo #define BIOS_VEISION_NAME     %ZX_MBTYPE%_R%BIOS_MAJOR_VER%%BIOS_MINOR_VER%_%BUILD_TYPE%_%BIOS_BUILD_DATE_Y2% >> %AUTOGEN_TARGET_FILE%
echo #define TKN_BOARD_ID        %BOARD_ID% >> %AUTOGEN_TARGET_FILE%
echo #define TKN_OEM_ID          %OEM_ID% >> %AUTOGEN_TARGET_FILE%
echo.                                 >> %AUTOGEN_TARGET_FILE%
echo #endif                           >> %AUTOGEN_TARGET_FILE%
echo.                                 >> %AUTOGEN_TARGET_FILE%

set AUTOGEN_TARGET_FILE=%PLATFORM_PACKAGE%\PlatformAutoGen.dsc
echo # autogen file, do not edit ...          > %AUTOGEN_TARGET_FILE%
echo DEFINE ASIA_MBTYPE  = %ZX_MBTYPE%       >> %AUTOGEN_TARGET_FILE%
echo DEFINE ASIA_CPUTYPE = %ZXCPU_TYPE%      >> %AUTOGEN_TARGET_FILE%
echo DEFINE ASIA_NBTYPE  = %ZXCHIP_TYPE%     >> %AUTOGEN_TARGET_FILE%
if "%5" equ "IOE_EXIST" echo DEFINE ASIA_IOETYPE = %ZX_IOETYPE% >> %AUTOGEN_TARGET_FILE%
if "%5" equ "" echo DEFINE ASIA_IOETYPE = NULL >> %AUTOGEN_TARGET_FILE%
REM if "%3" neq "" echo DEFINE %3  = TRUE  >> %AUTOGEN_TARGET_FILE%
if "%3" equ "HX002EA0_03" echo DEFINE %3  = TRUE  >> %AUTOGEN_TARGET_FILE%
if "%3" equ "HX002EB0_00" echo DEFINE %3  = TRUE  >> %AUTOGEN_TARGET_FILE%
if "%3" equ "HX002EB0_11" echo DEFINE %3  = TRUE  >> %AUTOGEN_TARGET_FILE%
if "%3" equ "HX002EC0_01" echo DEFINE %3  = TRUE  >> %AUTOGEN_TARGET_FILE%
if "%3" equ "HX002EC0_10" echo DEFINE %3  = TRUE  >> %AUTOGEN_TARGET_FILE%
if "%3" equ "HX002ED0_02" echo DEFINE %3  = TRUE  >> %AUTOGEN_TARGET_FILE%
if "%3" equ "HX002ED0_10" echo DEFINE %3  = TRUE  >> %AUTOGEN_TARGET_FILE%
if "%3" equ "HX002EE0_04" echo DEFINE %3  = TRUE  >> %AUTOGEN_TARGET_FILE%
if "%3" equ "HX002EE0_05" echo DEFINE %3  = TRUE  >> %AUTOGEN_TARGET_FILE%
if "%3" equ "HX002EH0_01" echo DEFINE %3  = TRUE  >> %AUTOGEN_TARGET_FILE%

if "%4" neq "" echo DEFINE %4  = TRUE  >> %AUTOGEN_TARGET_FILE%

REM ********** If IOE_EXIST exist, then define it. **********
if "%5" neq "" echo DEFINE %5  = TRUE  >> %AUTOGEN_TARGET_FILE%

if "%XHCI_FW_PREFIX%" neq ""      echo DEFINE XHCI_FW_PREFIX  = %XHCI_FW_PREFIX%  >> %AUTOGEN_TARGET_FILE%
if "%XHCI_FW_VER%" neq ""         echo DEFINE XHCI_FW_VER     = %XHCI_FW_VER%     >> %AUTOGEN_TARGET_FILE%
if "%XHCI_FW_TYPE%" neq ""        echo DEFINE XHCI_FW_TYPE    = %XHCI_FW_TYPE%    >> %AUTOGEN_TARGET_FILE%
if "%XHCI_FW_PATH_FILE_NAME%" neq ""   echo DEFINE XHCI_FW_PATH_FILE  = %USB_FW_PATH:\=/%/%XHCI_FW_PATH_FILE_NAME%  >> %AUTOGEN_TARGET_FILE%

if "%IOE_XHCI_FW_PREFIX%" neq ""      echo DEFINE IOE_XHCI_FW_PREFIX  = %IOE_XHCI_FW_PREFIX%  >> %AUTOGEN_TARGET_FILE%
if "%IOE_XHCI_FW_VER%" neq ""         echo DEFINE IOE_XHCI_FW_VER     = %IOE_XHCI_FW_VER%     >> %AUTOGEN_TARGET_FILE%
if "%IOE_XHCI_FW_TYPE%" neq ""        echo DEFINE IOE_XHCI_FW_TYPE    = %IOE_XHCI_FW_TYPE%    >> %AUTOGEN_TARGET_FILE%
if "%IOE_XHCI_FW_PATH_FILE_NAME%" neq ""   echo DEFINE IOE_XHCI_FW_PATH_FILE  = %IOE_USB_FW_PATH:\=/%/%IOE_XHCI_FW_PATH_FILE_NAME%  >> %AUTOGEN_TARGET_FILE%

echo "Update Conf\target.txt"
@findstr /V "ACTIVE_PLATFORM TARGET TARGET_ARCH TOOL_CHAIN_TAG BUILD_RULE_CONF MAX_CONCURRENT_THREAD_NUMBER" Conf\target.txt > %OUTPUT_DIR%\target.txt
@echo ACTIVE_PLATFORM = %PLATFORM_PACKAGE%/PlatformX64Pkg.dsc     >> %OUTPUT_DIR%\target.txt
@echo TARGET          = %TARGET%                                  >> %OUTPUT_DIR%\target.txt
@echo TARGET_ARCH     = IA32 X64                                  >> %OUTPUT_DIR%\target.txt
@echo TOOL_CHAIN_TAG  = %TOOL_CHAIN_TAG%                          >> %OUTPUT_DIR%\target.txt
@echo BUILD_RULE_CONF = Conf/build_rule.txt                       >> %OUTPUT_DIR%\target.txt
@echo MAX_CONCURRENT_THREAD_NUMBER = %NUMBER_OF_PROCESSORS%       >> %OUTPUT_DIR%\target.txt
@move /Y %OUTPUT_DIR%\target.txt Conf > NUL



REM ------------------------------- BUILD --------------------------------------
:READY_TO_BUILD
echo Start building ...
@build -j %OUTPUT_DIR%\Build.log
@if "%ERRORLEVEL%" NEQ "0" goto :ERROR



REM --------------------------------- FCE --------------------------------------
echo [FCE]
pushd %OUTPUT_DIR%\FV
if exist *.bin del *.bin
set OUTPUT_FD_FILE=ZX8711A.fd
if "%3" equ "HX002EH0_01" set ZX_MBTYPE=%ZX_IOETYPE%
set BIOS_TARGET_NAME=%ZX_MBTYPE%_R%BIOS_MAJOR_VER%%BIOS_MINOR_VER%_%BUILD_TYPE%_%BIOS_BUILD_DATE_Y2%.bin

FCE.exe read -i %OUTPUT_FD_FILE% 0006 005C 0078 0030 0030 0030 0031 > FdVar.txt
@if "%ERRORLEVEL%" NEQ "0" (
  echo create FdVar.txt failed!
  goto FCE_Exit
)
FCE.exe update -i %OUTPUT_FD_FILE% -s FdVar.txt -g 004AE66F-F074-4398-B47F-F73BA682C7BE -o %BIOS_TARGET_NAME%
@if "%ERRORLEVEL%" NEQ "0" (
  echo FCE update failed!
  goto FCE_Exit
)

if exist %OUTPUT_FD_FILE%.bak del %OUTPUT_FD_FILE%.bak
ren %OUTPUT_FD_FILE% %OUTPUT_FD_FILE%.bak

echo -----------------------------------------------------
Win32Tool.exe -cs %BIOS_TARGET_NAME% | tee.exe ChkSum.bin
echo -----------------------------------------------------

popd


REM ------------------------------- SUCCESS ------------------------------------
echo The EDK2 BIOS build has been completed.
goto :EOF


REM ------------------------------- CLEAN --------------------------------------
:DoBuildClean
@echo off
@if exist Build rmdir /Q /S Build
@if exist Conf  rmdir /Q /S Conf

@echo.
@echo clean OK.
@echo.

goto :EOF

:FCE_Exit
:ERROR
echo Build Error occurred!
