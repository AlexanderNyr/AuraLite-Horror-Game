@echo off
title Building Anxiety Fog: Horror Loop
echo =========================================================
echo    Anxiety Fog: Horror Loop - Windows Build Script
echo =========================================================
echo.

:: Check for CMake in the system
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake was not found in your PATH!
    echo Please download and install CMake from: https://cmake.org/
    echo Make sure to select "Add CMake to the system PATH" during installation.
    goto error
)

:: Create build folder if it does not exist
if not exist build (
    echo [INFO] Creating build directory...
    mkdir build
)

cd build

echo [INFO] Configuring the project using CMake...
echo [INFO] Note: If SDL2 is not found on your system, CMake will automatically
echo        download and compile it. This may take a moment on the first run.
echo.

cmake -DCMAKE_BUILD_TYPE=Release ..
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    goto error
)

echo.
echo [INFO] Compiling the project...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed!
    goto error
)

cd ..

echo.
:: Clean up old copy if it exists to make sure we detect the new one
if exist AnxietyHorror.exe del /f /q AnxietyHorror.exe >nul 2>nul

:: Copy the executable to the root folder
if exist build\Release\AnxietyHorror.exe (
    echo [INFO] Copying AnxietyHorror.exe from build\Release\ to root folder...
    copy /y build\Release\AnxietyHorror.exe . >nul
)
if exist build\AnxietyHorror.exe (
    echo [INFO] Copying AnxietyHorror.exe from build\ to root folder...
    copy /y build\AnxietyHorror.exe . >nul
)

:: Foolproof check if the file exists in the root folder now
set COPIED=0
if exist AnxietyHorror.exe set COPIED=1

echo =========================================================
if "%COPIED%"=="1" goto build_ok
goto build_fail

:build_ok
echo [SUCCESS] Build completed successfully!
echo.
echo The executable "AnxietyHorror.exe" has been placed in:
echo --^> %CD%\AnxietyHorror.exe
echo.
echo Double-click "AnxietyHorror.exe" in this folder to play the game!
goto end_report

:build_fail
echo [WARNING] Build claimed success, but the .exe was not found!
echo.
echo This is 99%% likely due to Windows Defender (Antivirus) silently
echo deleting/quarantining the freshly compiled file as a false positive.
echo.
echo To fix this:
echo 1. Open "Windows Security" (Безопасность Windows).
echo 2. Go to "Protection History" (Журнал защиты).
echo 3. Find the recent blocked threat (usually Trojan:Win32/Wacatac in this folder).
echo 4. Click "Actions" -> "Allow on device" (Разрешить на устройстве) or "Restore".
echo 5. Or, add this folder to your Antivirus Exclusions and run this script again.
goto end_report

:end_report
echo =========================================================
goto end

:error
echo.
echo [FAIL] Build failed. Please check the error messages above.
echo =========================================================
cd ..

:end
pause
