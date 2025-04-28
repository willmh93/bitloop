set BUILD_DIR=..\build\web_debug
set DEPLOY_DIR=..\deployed\web_debug
set STATIC_DIR=..\static

rmdir /s /q %DEPLOY_DIR%
mkdir %BUILD_DIR%
mkdir %DEPLOY_DIR%

:: Move output files to deploy folder
echo Moving build output to %DEPLOY_DIR%...
copy %BUILD_DIR%\bitloop.html %DEPLOY_DIR%\bitloop.html
copy %BUILD_DIR%\*.js %DEPLOY_DIR%\
copy %BUILD_DIR%\*.wasm %DEPLOY_DIR%\
copy %BUILD_DIR%\*.data %DEPLOY_DIR%\
