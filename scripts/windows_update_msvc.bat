
@echo off
REM Set variables
set BUILD_DIR=../build/windows
set GENERATOR="Visual Studio 17 2022"
set CONFIG=Debug

REM Remove old build directory (optional)
if exist %BUILD_DIR% (
    echo Removing old build directory...
    rmdir /s /q %BUILD_DIR%
)

REM Create build directory
echo Creating build directory...
mkdir %BUILD_DIR%

REM Run CMake to generate the solution
echo Running CMake...
cmake -S .. -B %BUILD_DIR% -G %GENERATOR% -DCMAKE_BUILD_TYPE=%CONFIG%

REM Check if CMake was successful
if %errorlevel% neq 0 (
    echo CMake failed!
    exit /b %errorlevel%
)

echo Done!
