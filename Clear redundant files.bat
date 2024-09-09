
@REM 删除文件中多余的文件
@REM Delete redundant files generated in the project

@echo off
setlocal enabledelayedexpansion

REM 定义列表
set "fileList="
call :appendFileList 1_led
call :appendFileList 2_jlink_rtt_print
call :appendFileList 3_sdcard
call :appendFileList 4_oled
call :appendFileList 5_RF_test
call :appendFileList 6_SubGHz_TXRX
call :appendFileList PingPong
call :appendFileList DeepSleep

REM 遍历列表并输出文件名
for %%i in (%fileList%) do (
    rmdir /s /q examples\%%i\MDK-ARM\RTE
    rmdir /s /q examples\%%i\.vscode

    del examples\%%i\MDK-ARM\*.lyy
    del examples\%%i\MDK-ARM\*.txt
    del examples\%%i\MDK-ARM\*.ini
    del examples\%%i\MDK-ARM\*.scvd

    del examples\%%i\MDK-ARM\%%i\*.o
    del examples\%%i\MDK-ARM\%%i\*.d
    del examples\%%i\MDK-ARM\%%i\*.htm
    del examples\%%i\MDK-ARM\%%i\*.dep
    del examples\%%i\MDK-ARM\%%i\*.map
    del examples\%%i\MDK-ARM\%%i\*.lnp
    del examples\%%i\MDK-ARM\%%i\*.axf
    del examples\%%i\MDK-ARM\%%i\*.iex
    echo %%i
)

endlocal
@REM pause
goto :eof

:appendFileList
if defined fileList (
    set "fileList=!fileList! %1"
) else (
    set "fileList=%1"
)
goto :eof