@echo off
echo ===================================================
echo   Schwarzschild Black Hole Code Compiler
echo ===================================================
echo 1. Compile and Run Geodesic Physics Check (verify_physics)
echo 2. Compile and Run Visual Renderer (render_test - outputs BMP)
echo.
set /p choice="Enter option (1 or 2): "

if "%choice%"=="1" (
    echo.
    echo Compiling Geodesic Physics Check...
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
) else if "%choice%"=="2" (
    echo.
    echo Compiling Visual Renderer (with OpenMP)...
    g++ -O3 -fopenmp -std=c++11 BlackHoleRenderer.cpp render_test.cpp -o render_test.exe
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo Compilation successful! Running render_test.exe...
        echo.
        render_test.exe
    ) else (
        echo.
        echo Compilation failed!
    )
) else (
    echo Invalid choice!
)
pause
