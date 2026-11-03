@echo off
echo Compiling Flight Management Backend...
g++ -std=c++17 -O2 -o backend\main.exe backend\main.cpp
if %ERRORLEVEL% == 0 (
    echo Compilation successful! backend\main.exe created.
) else (
    echo Compilation FAILED. Make sure g++ is installed and in PATH.
)
pause
