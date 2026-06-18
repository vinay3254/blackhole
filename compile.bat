@echo off
echo Compiling Schwarzschild Geodesic Physics Core...
g++ -O3 -std=c++11 GeodesicTable.cpp verify_physics.cpp -o verify_physics.exe
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Compilation successful! Running verify_physics.exe...
    echo.
    verify_physics.exe
) else (
    echo.
    echo Compilation failed!
)
pause
