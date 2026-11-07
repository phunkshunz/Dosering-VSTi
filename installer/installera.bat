@echo off
REM KOLDBARAREN - MASKINHALLEN / Phunkshunz AUDIO
REM Hogerklicka och valj "Kor som administrator".

set "DEST=%CommonProgramFiles%\VST3"
if not exist "%DEST%" mkdir "%DEST%"
if exist "%DEST%\KOLDBARAREN.vst3" rmdir /s /q "%DEST%\KOLDBARAREN.vst3"
xcopy /e /i /y "%~dp0KOLDBARAREN.vst3" "%DEST%\KOLDBARAREN.vst3" >nul
if errorlevel 1 (
  echo.
  echo   Misslyckades. Hogerklicka pa installera.bat och valj "Kor som administrator".
) else (
  echo.
  echo   Klart! KOLDBARAREN finns nu i %DEST%
  echo   Starta om din DAW och skanna plugins.
)
echo.
pause
