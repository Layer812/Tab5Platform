@echo off
setlocal
cd /d "%~dp0"
if not exist build\tab5_platform_reference.bin (
  echo ERROR: build\tab5_platform_reference.bin not found. Run RUN_THIS_BUILD.cmd first.
  exit /b 2
)
if not defined IDF_PATH (
  if exist "G:\esp-idf-5.5.4\export.bat" (
    call "G:\esp-idf-5.5.4\export.bat"
  ) else (
    echo ERROR: ESP-IDF 5.5.4 is not active.
    exit /b 3
  )
)
echo EXPERIMENTAL: This is NOT a verified production firmware build.
echo WARNING: Replaces the current factory app at 0x10000 on COM9.
echo This does not flash bootloader, partition table, or humanxdf.
echo Use ONLY if the installed partition layout matches X68KTab5 R6.
idf.py -p COM9 app-flash || exit /b 4
endlocal
