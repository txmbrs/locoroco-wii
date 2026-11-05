@echo off
setlocal
cd /d "%~dp0"
if exist "%~dp0LocoRocoImporter.exe" (
  "%~dp0LocoRocoImporter.exe" %*
) else (
  py -3 "%~dp0convert.py" %*
)
set "converter_exit=%errorlevel%"
echo.
if not "%converter_exit%"=="0" echo Konwersja nie zostala ukonczona. Przeczytaj komunikat powyzej.
pause
exit /b %converter_exit%
