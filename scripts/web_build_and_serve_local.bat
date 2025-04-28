@echo off
setlocal

::echo Removing old build
::rmdir /s /q ..\build\web_debug
::mkdir ..\build\web_debug

echo Generating build...
call  emcmake cmake -G Ninja  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=17 -S .. -B ..\build\web_debug

echo Building...
cmake --build ..\build\web_debug -j --parallel

echo Copying static resources...
call internal/copy_resources_local.bat

cd ..
start "" caddy run

endlocal