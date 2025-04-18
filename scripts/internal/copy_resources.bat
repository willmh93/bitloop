set BUILD_DIR=..\build\web
set DEPLOY_DIR=..\deployed\web
set STATIC_DIR=..\static

rmdir /s /q %DEPLOY_DIR%
mkdir %BUILD_DIR%
mkdir %DEPLOY_DIR%

:: Move output files to deploy folder
echo Moving build output to %DEPLOY_DIR%...
::copy %BUILD_DIR%\bitloop.html %DEPLOY_DIR%\bitloop.html
copy %BUILD_DIR%\*.js %DEPLOY_DIR%\
copy %BUILD_DIR%\*.wasm %DEPLOY_DIR%\
copy %BUILD_DIR%\*.svg %DEPLOY_DIR%\
copy %BUILD_DIR%\*.png %DEPLOY_DIR%\

:: Override with custom files

copy %STATIC_DIR%\bitloop.html %DEPLOY_DIR%\bitloop.html
copy %STATIC_DIR%\*.png %DEPLOY_DIR%\
