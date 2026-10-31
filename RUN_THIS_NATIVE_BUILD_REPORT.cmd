@echo off
setlocal
cd /d "%~dp0"
python tools\native_build_report.py
exit /b %ERRORLEVEL%
