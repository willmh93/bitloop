@echo off
setlocal

echo Removing old build
::rmdir /s /q ..\build\web_release
::mkdir ..\build\web_release

echo Generating build...
call  emcmake cmake -G Ninja  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -S .. -B ..\build\web_release

echo Building...
cmake --build ..\build\web_release -j --parallel

echo Copying static resources...
call internal/copy_resources.bat

echo Done!
pause

endlocal