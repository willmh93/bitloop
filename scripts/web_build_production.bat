@echo off
setlocal

set START_DIR=%CD%

::echo Removing old build
::rmdir /s /q ..\build\web_release
::mkdir ..\build\web_release

echo Generating build...
call  emcmake cmake -G Ninja  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -S .. -B ..\build\web_release

echo Building...
cmake --build ..\build\web_release -j --parallel

echo Copying build files to deployed...
call internal/copy_resources.bat

echo Patching release html...
python.exe "internal/patch_web_release_href.py" "../deployed/web_release/bitloop.html" "/main/"

echo Done!
pause

endlocal