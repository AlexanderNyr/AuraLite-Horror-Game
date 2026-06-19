@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul
title Building AnxietyHorror for Android

echo =========================================================
echo    AnxietyHorror - Android Build Script
echo =========================================================
echo.

:: ============================================================
:: 1. Java / JDK
:: ============================================================
where java >nul 2>nul
if errorlevel 1 goto no_java
echo [OK] Java detected:
java -version 2>&1
echo.
goto java_ok

:no_java
echo [ERROR] Java was not found in your PATH!
echo Please install JDK 17 or newer and make sure that
echo %%JAVA_HOME%%\bin is in PATH.
echo.
goto error

:java_ok

:: ============================================================
:: 2. ANDROID_HOME / ANDROID_SDK_ROOT
:: ============================================================
if not "%ANDROID_HOME%"=="" goto android_sdk_ok
if not "%ANDROID_SDK_ROOT%"=="" goto android_sdk_root
echo [ERROR] ANDROID_HOME or ANDROID_SDK_ROOT is not set!
echo Please install Android SDK, API 33 plus build-tools and
echo platform-tools, then set the ANDROID_HOME environment variable,
echo for example:
echo     set ANDROID_HOME=C:\Android\Sdk
echo.
goto error

:android_sdk_root
set "ANDROID_HOME=%ANDROID_SDK_ROOT%"

:android_sdk_ok
echo [OK] ANDROID_HOME = %ANDROID_HOME%
echo.

:: ============================================================
:: 3. ANDROID_NDK_HOME
:: ============================================================
if not "%ANDROID_NDK_HOME%"=="" goto ndk_ok
echo [WARN] ANDROID_NDK_HOME is not set.
echo Looking for an installed NDK under %%ANDROID_HOME%%\ndkit\ ...
echo.
set "ANDROID_NDK_HOME="
for /f "delims=" %%i in ('dir /b "%ANDROID_HOME%\ndkit" 2^>nul') do (
    if "!ANDROID_NDK_HOME!"=="" set "ANDROID_NDK_HOME=%ANDROID_HOME%\ndkit\%%i"
)
if not "%ANDROID_NDK_HOME%"=="" goto ndk_ok
echo [ERROR] No NDK installation found.
echo Install via Android Studio - SDK Manager - SDK Tools - NDK Side by side.
echo.
goto error

:ndk_ok
echo [OK] ANDROID_NDK_HOME = %ANDROID_NDK_HOME%
echo.

:: ============================================================
:: 4. CMake - version 3.22.1 is pinned in app\build.gradle
:: ============================================================
where cmake >nul 2>nul
if errorlevel 1 goto no_cmake
echo [OK] CMake detected:
cmake --version
echo.
goto cmake_ok

:no_cmake
echo [ERROR] CMake was not found in your PATH!
echo CMake 3.22.1 is required - the version pinned in app\build.gradle.
echo Install from https://cmake.org/ and make sure it is on PATH.
echo.
goto error

:cmake_ok

:: ============================================================
:: 5. curl - used by the Gradle task to fetch SDL2 Java sources
:: ============================================================
where curl >nul 2>nul
if not errorlevel 1 goto curl_ok
echo [WARN] curl was not found in PATH.
echo The first build uses curl to download the SDL2 2.28.5 Java sources.
echo Windows 10 and newer usually has curl built-in - check your %%PATH%%.
echo.
echo If curl is missing, install it, or download SDL2 manually:
echo     https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.28.5.zip
echo and extract android-project\app\src\main\java\org\libsdl\app\
echo into android\app\src\main\java\org\libsdl\app\
echo.

:curl_ok

:: ============================================================
:: 6. Gradle - prefer wrapper, fall back to system gradle
:: ============================================================
set "GRADLE_CMD="
if exist "android\gradlew.bat" goto gradle_wrapper_ok
where gradle >nul 2>nul
if errorlevel 1 goto no_gradle
echo [INFO] No Gradle wrapper present - falling back to system gradle.
gradle --version
echo.
set "GRADLE_CMD=gradle"
goto gradle_ok

:no_gradle
echo [ERROR] Neither android\gradlew.bat nor a system 'gradle' was found.
echo Run 'gradle wrapper --gradle-version 8.0' once to generate the wrapper,
echo then re-run this script.
echo.
goto error

:gradle_wrapper_ok
echo [OK] Found android\gradlew.bat - using the Gradle wrapper.
set "GRADLE_CMD=call android\gradlew.bat"

:gradle_ok

:: ============================================================
:: 7. Run the build
:: ============================================================
echo =========================================================
echo    Starting Android build - assembleDebug
echo =========================================================
echo.
echo This may take a long time on the first run:
echo   1. curl downloads the SDL2 2.28.5 Java sources.
echo   2. CMake and NDK fetch and compile SDL2 native code.
echo   3. CMake and NDK compile the game's libmain.so.
echo   4. Gradle packages everything into an APK.
echo.

cd android
%GRADLE_CMD% assembleDebug --no-daemon
set "BUILD_EXIT=%errorlevel%"
cd ..

if not "%BUILD_EXIT%"=="0" goto build_failed
goto build_succeeded

:build_failed
echo.
echo [ERROR] Gradle build failed with exit code %BUILD_EXIT%.
goto error

:: ============================================================
:: 8. Locate the resulting APK
:: ============================================================
:build_succeeded
set "APK_PATH=android\app\build\outputs\apk\debug\app-debug.apk"
if exist "%APK_PATH%" goto apk_ok

echo [WARN] Build claimed success but %APK_PATH% was not found.
echo Look under android\app\build\outputs\apk\ for the produced APK.
goto end_report

:apk_ok
echo.
echo =========================================================
echo    [SUCCESS] Android build finished!
echo =========================================================
echo.
echo APK location:
echo     %APK_PATH%
echo.
echo Install on a connected device with:
echo     adb install -r %APK_PATH%
echo.
echo Note - the game requests landscape orientation,
echo see android\app\src\main\AndroidManifest.xml.

:end_report
echo =========================================================
goto end

:error
echo.
echo [FAIL] Android build did not complete.
echo Common fixes:
echo   - Install JDK 17 and set JAVA_HOME.
echo   - Install Android SDK, API 33, plus NDK and set
echo     ANDROID_HOME and ANDROID_NDK_HOME.
echo   - Add cmake 3.22.1 and curl to your PATH.
echo   - Run 'gradle wrapper' from the android\ folder once
echo     to generate gradlew.bat.
echo =========================================================

:end
endlocal
pause
