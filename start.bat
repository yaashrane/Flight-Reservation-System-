@echo off
echo ============================================
echo   Flight Management and Reservation System
echo ============================================

echo Compiling backend...
g++ -std=c++17 -O2 -o backend\main.exe backend\main.cpp
if %ERRORLEVEL% NEQ 0 (
    echo Compilation FAILED. Make sure g++ is installed and in PATH.
    pause & exit /b 1
)
echo Compilation successful.

echo Starting server at http://127.0.0.1:8080 ...
start "FlightMS Server" python server\server.py
timeout /t 2 /nobreak >nul
start "" "http://127.0.0.1:8080"
echo Server running. Close this window to stop.
pause
