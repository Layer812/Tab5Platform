@echo off
setlocal
cd /d "%~dp0"
if not defined IDF_PATH (
  if exist "G:\esp-idf-5.5.4\export.bat" (
    call "G:\esp-idf-5.5.4\export.bat"
  ) else (
    echo ERROR: ESP-IDF 5.5.4 is not active. Activate its export.bat first.
    exit /b 2
  )
)
python tools\bootstrap_deps.py || exit /b 3
idf.py set-target esp32p4 || exit /b 4
idf.py build || exit /b 5
echo PASS: Firmware built. No device was flashed.
endlocal
