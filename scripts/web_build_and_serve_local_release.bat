@echo off
setlocal

echo Generating build...
call  emcmake cmake -G Ninja  -DCMAKE_BUILD_TYPE=Release -S .. -B ..\build\web_release

echo Building...
cmake --build ..\build\web_release -j --parallel

echo Copying static resources...
call internal/copy_resources.bat

cd ..
start "" caddy run --config local_release.Caddyfile

endlocal