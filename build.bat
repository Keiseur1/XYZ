@echo off
setlocal
set "PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.2\mingw_64\bin;C:\Windows\system32;C:\Windows"
set "BUILD_DIR=%~dp0build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%" || exit /b 1

echo [build.bat] Running qmake...
qmake "%~dp0DijkstraMap.pro" -spec win32-g++ "CONFIG+=debug" || exit /b 1

echo [build.bat] Compiling Debug...
mingw32-make -f Makefile.Debug -j4
if errorlevel 1 (
    echo [build.bat] Debug build failed!
    exit /b 1
)

echo [build.bat] Compiling Release...
mingw32-make -f Makefile.Release -j4

echo [build.bat] Done!
endlocal
