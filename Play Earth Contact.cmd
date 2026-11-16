@echo off
setlocal
if not defined UE_ROOT set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"
set "EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%EDITOR%" (
    echo Set UE_ROOT to your Unreal Engine 5.8 installation.
    pause
    exit /b 1
)
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" EarthContactEditor Win64 Development "-Project=%~dp0EarthContact.uproject" -WaitMutex
if errorlevel 1 (
    pause
    exit /b 1
)
set "MAP=/Engine/Maps/Entry"
if /I "%~1"=="relay" set "MAP=/Engine/Maps/Entry?Mission=Relay"
start "Earth Contact" "%EDITOR%" "%~dp0EarthContact.uproject" "%MAP%" -game -windowed -ResX=1100 -ResY=700 -Log=Solo.log
