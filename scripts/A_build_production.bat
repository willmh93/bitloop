@echo off
setlocal

echo Removing old build
rmdir /s /q ..\build\web
mkdir ..\build\web

echo Generating build...
call  emcmake cmake -G Ninja  -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_CXX_STANDARD=17 -S .. -B ..\build\web

echo Building...
cmake --build ..\build\web -j --parallel

echo Copying static resources...
call internal/copy_resources.bat

echo Done!
pause

endlocal