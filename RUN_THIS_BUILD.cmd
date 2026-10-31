@echo off
setlocal
cd /d "%~dp0" || exit /b 2

rem Always activate the selected ESP-IDF 5.5.4, even when IDF_PATH is set
rem but the tools have not yet been exported into this cmd.exe environment.
if defined IDF_PATH (
    if not exist "%IDF_PATH%\export.bat" (
        echo ERROR: ESP-IDF export.bat missing at "%IDF_PATH%\export.bat"
        exit /b 2
    )
    call "%IDF_PATH%\export.bat"
) else (
    if not exist "G:\esp-idf-5.5.4\export.bat" (
        echo ERROR: ESP-IDF 5.5.4 not installed at G:\esp-idf-5.5.4
        exit /b 2
    )
    call "G:\esp-idf-5.5.4\export.bat"
)
if errorlevel 1 exit /b 2

python tools\bootstrap_deps.py
if errorlevel 1 exit /b 3

idf.py set-target esp32p4
if errorlevel 1 exit /b 4

idf.py build
if errorlevel 1 exit /b 5

echo PASS: Firmware built. No device was flashed.
exit /b 0
