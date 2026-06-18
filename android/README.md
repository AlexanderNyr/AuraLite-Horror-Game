# Android build

The project is now set up to build an Android APK with SDL2.

## What is already configured

- `CMakeLists.txt` (project root) builds `libSDL2.so` via CMake/FetchContent for Android and links the native `main` library against it.
- `app/src/main/java/org/libsdl/app/` contains the required SDL2 Java classes (`SDLActivity`, `SDLSurface`, `SDLAudioManager`, `HIDDevice*`, etc.).
- `app/src/main/java/com/example/horror/MainActivity.kt` extends `SDLActivity` and loads the native libraries `SDL2` and `main`.
- `app/build.gradle` contains a `downloadSDL2` task that fetches the SDL2 Java sources automatically if they are missing.
- `.lang` localization files are packaged as assets via `sourceSets`.

## Requirements

- Android Studio or the Android SDK command line tools
- Android SDK (API 33 used by the project)
- Android NDK (CMake 3.22.1)
- `curl` in PATH (used by the Gradle task to download SDL2 Java sources on first build)

## Build

```bash
cd android
./gradlew assembleDebug
```

The first build will:
1. Download SDL2 sources (if the Java files are not already present).
2. Build `libSDL2.so` and `libmain.so` via CMake/NDK.
3. Produce `android/app/build/outputs/apk/debug/app-debug.apk`.

If you want to use a pre-downloaded SDL2 checkout instead of FetchContent, place it at `third_party/SDL2/`; CMake will prefer that local copy.
