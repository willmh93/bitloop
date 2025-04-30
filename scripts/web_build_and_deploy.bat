@echo off
setlocal

echo Removing old build
rmdir /s /q ..\build\web
mkdir ..\build\web

echo Generating build...
call  emcmake cmake -G Ninja  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -S .. -B ..\build\web

echo Building...
cmake --build ..\build\web -j --parallel

echo Patching release html <head>
python.exe internal/patch_web_release_href.py "../deployed/web_release/bitloop.html" "/main/"

echo Copying build files to deployed...
call internal/copy_resources.bat

echo Patching release html...
python.exe "internal/patch_web_release_href.py" "../deployed/web_release/bitloop.html" "/main/"

echo Done!

powershell -NoProfile -ExecutionPolicy Bypass -File "internal/deploy_production.ps1"

endlocal