@echo off
setlocal

echo Generating build...
call  emcmake cmake -G Ninja  -DCMAKE_BUILD_TYPE=Debug -S .. -B ..\build\web_debug

echo Building...
cmake --build ..\build\web_debug -j --parallel

echo Copying static resources...
call internal/copy_resources_local.bat

cd ..
start "" caddy run --config local_debug.Caddyfile

endlocal