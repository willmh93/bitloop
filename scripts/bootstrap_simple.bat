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

rem ─── 4) Bootstrap vcpkg ────────────────────────────────
echo Bootstrapping vcpkg...
call "%REPO_ROOT%\vcpkg\bootstrap-vcpkg.bat" || (
  echo ERROR: vcpkg bootstrap failed.
  exit /b 1
)

rem ─── Ensure Windows SDK via BuildTools bootstrapper ─────────────────────

rem ─── Ensure MSVC Build Tools + C++ workload + Windows SDK ─────────────────
where cl >nul 2>&1
if ERRORLEVEL 1 (
  echo Installing MSVC Build Tools, C++ workload, and Windows SDK via winget…
  winget install --id Microsoft.VisualStudio.2022.BuildTools --exact ^
  --silent --accept-package-agreements --accept-source-agreements ^
  --override "--add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.Windows10SDK.19041.Desktop --add Microsoft.VisualStudio.Component.VC.CMake.Tools --includeRecommended --passive --norestart --wait"
) else (
  echo MSVC Build Tools already installed.
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