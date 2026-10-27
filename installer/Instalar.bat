@echo off
net session >nul 2>&1
if %errorlevel% neq 0 goto noadmin

for /d %%D in ("%~dp0*.vst3") do xcopy "%%D" "%ProgramFiles%\Common Files\VST3\%%~nxD\" /E /I /Y
echo.
echo Instalado en %ProgramFiles%\Common Files\VST3
echo Abre FL Studio: Options - Manage plugins - Find installed plugins.
pause
exit /b

:noadmin
echo Haz clic derecho sobre este archivo y elige "Ejecutar como administrador".
pause
