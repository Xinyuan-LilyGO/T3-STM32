
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

REM 遍历列表并输出文件名
for %%i in (%fileList%) do (
    rmdir /s /q %%i\MDK-ARM\RTE
    rmdir /s /q %%i\.vscode

    del %%i\MDK-ARM\*.lyy
    del %%i\MDK-ARM\*.txt
    del %%i\MDK-ARM\*.ini
    del %%i\MDK-ARM\*.scvd

    del %%i\MDK-ARM\%%i\*.o
    del %%i\MDK-ARM\%%i\*.d
    del %%i\MDK-ARM\%%i\*.htm
    del %%i\MDK-ARM\%%i\*.dep
    del %%i\MDK-ARM\%%i\*.map
    del %%i\MDK-ARM\%%i\*.lnp
    del %%i\MDK-ARM\%%i\*.axf
    del %%i\MDK-ARM\%%i\*.iex
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