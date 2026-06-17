# Android notes

The C++ code already has Android-specific branches for OpenGL ES 3.0, touch controls and `.lang` localization assets, but the Gradle project still needs a real SDL2 Android integration before it can produce a complete APK.

Recommended options:

1. Add SDL2 Android sources as a Gradle module, following the official SDL2 Android template.
2. Or use a maintained SDL2 Android AAR that provides:
   - `org.libsdl.app.SDLActivity`
   - native `libSDL2.so`
   - headers/libs visible to CMake/NDK.

Current entry point:

- `android/app/src/main/java/com/example/horror/MainActivity.kt`
- native library loaded: `main`
- expected SDL library: `SDL2`

Once SDL2 is wired, build with:

```bash
cd android
./gradlew assembleDebug
```
