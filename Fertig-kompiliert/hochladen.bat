@echo off
setlocal
cd /d "%~dp0"
echo ============================================================
echo   E09 Roboter - fertigen Code hochladen (ohne Kompilieren)
echo ============================================================
echo.
echo Welche Version?
echo   1 = Version 2  Baukasten + Hotspot + Sortierrad  (empfohlen)
echo   2 = Version 1  Werkbank
set /p V="Auswahl [1]: "
if "%V%"=="" set V=1
if "%V%"=="1" (set DIR=v2-baukasten& set NAME=ip-motorshield-blocks) else (set DIR=v1-werkbank& set NAME=ip-motorshield)
echo.
echo Wie hochladen?
echo   1 = Per WLAN  (nur wenn UNSER Code schon drauf ist; Laptop im Roboter-WLAN)
echo   2 = Per USB-Kabel  (geht immer, auch nach fremdem Code)
set /p M="Auswahl [1]: "
if "%M%"=="" set M=1
echo.
if "%M%"=="2" goto KABEL

set /p PW="WLAN-Passwort, das GERADE auf dem Roboter ist [WeLove--]: "
if "%PW%"=="" set PW=WeLove--
tools\espota.exe -i 192.168.4.1 -p 3232 -a "%PW%" -r -f "%DIR%\%NAME%.ino.bin"
goto ENDE

:KABEL
echo Verfuegbare COM-Ports:
powershell -NoProfile -Command "[System.IO.Ports.SerialPort]::GetPortNames()"
set /p PORT="COM-Port des Roboters (z.B. COM4): "
tools\esptool.exe --chip esp32s3 --port %PORT% --baud 921600 write_flash 0x0 "%DIR%\%NAME%.ino.merged.bin"

:ENDE
echo.
if errorlevel 1 (echo FEHLER beim Hochladen - siehe oben.) else (echo FERTIG. Roboter startet neu. WLAN-Passwort ist jetzt: WeLove--)
pause
