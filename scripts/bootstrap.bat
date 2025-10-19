@echo off
setlocal

rem ─── Locate this script’s directory ──────────────────────────
rem %~dp0 is something like "C:\dev\bitloop\scripts\"
set "REPO_ROOT=%~dp0"

rem ─── Compute the parent (repo root) ─────────────────────────
rem Appending .. and normalizing via a FOR
for %%I in ("%REPO_ROOT%..") do set "REPO_ROOT=%%~fI"

rem ─── Determine repo root ───────────────────────────────
echo Repo Root:
echo %REPO_ROOT%

rem ─── 1) Ensure Git ─────────────────────────────────────
where git >nul 2>&1
if ERRORLEVEL 1 (
  echo Installing Git via winget
  winget install --id Git.Git --silent --accept-package-agreements --accept-source-agreements
) else (
  echo Git already installed.
)

rem ─── Ensure Windows SDK via BuildTools bootstrapper ─────────────────────

rem ─── Ensure MSVC Build Tools + C++ workload + Windows SDK ─────────────────
where cl >nul 2>&1
if ERRORLEVEL 1 (
  echo Installing MSVC Build Tools, C++ workload, and Windows SDK via winget…
  winget install --id Microsoft.VisualStudio.2022.BuildTools --exact ^
  --silent --accept-package-agreements --accept-source-agreements ^
  --override "--add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.Windows10SDK.19041.Desktop --includeRecommended --passive --norestart --wait"
) else (
  echo MSVC Build Tools already installed.
)

rem ─── 4) Bootstrap vcpkg ────────────────────────────────
echo Bootstrapping vcpkg...
call "%REPO_ROOT%\vcpkg\bootstrap-vcpkg.bat" || (
  echo ERROR: vcpkg bootstrap failed.
  exit /b 1
)



rem ─── 3) Install Ninja via vcpkg ──────────────────────────
echo Installing Ninja via vcpkg...
"%REPO_ROOT%\vcpkg\vcpkg" install vcpkg-tool-ninja:x64-windows --classic

rem ─── Locate the Ninja tool binary ─────────────────────────────
set "VCPKG_NINJA=%REPO_ROOT%\vcpkg\installed\x64-windows\tools\ninja"


if not exist "%VCPKG_NINJA%\ninja.exe" (
  echo ERROR: ninja.exe not found under %VCPKG_NINJA%
  exit /b 1
)

rem ─── Prepend it to the **current** PATH so CMake sees it immediately ───
set PATH=%VCPKG_NINJA%;%PATH%

rem ─── (Optionally) Persist for future sessions ───────────────────────
rem You can use setx if you like, but remember new shells will pick it up:
rem setx PATH "%PATH%"

echo Ninja binary located and on PATH: %VCPKG_NINJA%\ninja.exe

rem ─── 4) Persist MSVC compiler on PATH ─────────────────────────────
echo Locating MSVC installation via vswhere…

rem Run vswhere.exe and capture its output into %%I
for /f "tokens=*" %%I in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath') do (
    set "VSINSTALL=%%I"
)

if not defined VSINSTALL (
  echo ERROR: vswhere failed to locate Visual Studio.
  exit /b 1
)

rem Find the one—and only—MSVC version folder under VC\Tools\MSVC
for /f "tokens=*" %%V in ('dir /b "%VSINSTALL%\VC\Tools\MSVC"') do (
    set "MSVCVER=%%V"
)

set "MSVC_BIN=%VSINSTALL%\VC\Tools\MSVC\%MSVCVER%\bin\Hostx64\x64"

echo Adding MSVC compiler dir to user PATH: %MSVC_BIN%

rem Only append if it’s not already there
echo %PATH% | findstr /i /c:"%MSVC_BIN%" >nul
if errorlevel 1 (
  setx PATH "%PATH%;%MSVC_BIN%" >nul
  echo Done—new shells will have cl.exe on PATH.
) else (
  echo MSVC compiler already on PATH.
)



rem ─── Persist BITLOOP_ROOT ───────────────────────────────────
echo Setting BITLOOP_ROOT to "%REPO_ROOT%"...
setx BITLOOP_ROOT "%REPO_ROOT%" >nul

rem ─── Append to user PATH if missing ─────────────────────────
rem Query the current user-level PATH from the registry
for /f "tokens=2,*" %%A in ('
  reg query "HKCU\Environment" /v PATH 2^>nul
') do set "USER_PATH=%%B"

rem ─── Append to user PATH if missing ─────────────────────────
echo %PATH% | findstr /i /c:"%REPO_ROOT%" >nul
if errorlevel 1 (
  echo Adding "%REPO_ROOT%" to your user PATH...
  rem setx PATH appends to the user PATH (merging with system PATH automatically)
  setx PATH "%PATH%;%REPO_ROOT%" >nul
) else (
  echo "%REPO_ROOT%" is already in your PATH.
)

:: Also update the **current** session
set BITLOOP_ROOT=%REPO_ROOT%
set PATH=%PATH%;%REPO_ROOT%



echo.
echo ─── Done. Close and reopen your Command Prompt to pick up the changes.