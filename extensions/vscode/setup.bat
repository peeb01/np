@echo off
setlocal

:: Define target extension directory
set "TARGET_DIR=%USERPROFILE%\.vscode\extensions\vscode-np"

echo Installing np language support for VS Code...

:: Create target directory if it doesn't exist
if not exist "%TARGET_DIR%" (
    mkdir "%TARGET_DIR%"
)

:: Copy all files and folders recursively
xcopy "%~dp0" "%TARGET_DIR%\" /E /Y /H /R

echo.
echo Success! np-lang support extension installed globally.
echo Please restart or reload your VS Code window to apply changes.
pause
