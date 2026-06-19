# AuraLite — *AnxietyHorror*

> *Nine quiet days in the mist.*

A first-person psychological horror game written from scratch in **C++17** with **SDL2** and raw **OpenGL** (4.5 Core on desktop, ES 3.2 on Android). The entire world — terrain, buildings, props, weather, audio, UI fonts and shaders — is **procedurally generated at runtime**. No external models, textures, or sound files are shipped: just code, shaders, and a few `.lang` text files for localization.

---

## ✨ Features

- 🌫️ **9-day story loop** in an open foggy valley with daily objectives, sleep-fade transitions, and an ending.
- 🧠 **Sanity, stamina & fatigue systems** — sprint too much, stay out at night, and your fear meter fills until the fog takes you.
- 🌍 **Fully procedural world** — terrain heightmap, cabin, well, car, trees, houses, rocks, fences, collectibles, paths, all generated in `procedural.cpp`.
- 🎨 **Procedural PBR-ish renderer**
  - Cook–Torrance BRDF with hemispheric ambient
  - Procedural detail normals & noise fog
  - Directional sun + dynamic flashlight spotlight
  - 2048×2048 shadow map with `sampler2DShadow` + 3×3 PCF
  - ACES tonemapping, procedural sky shader
- 🌦️ **Dynamic time of day & weather** — clear, rain, snow, fog. Sun and moon move; ambient, fog, and sky colors recompute every frame.
- 👻 **Enemies appear from Day 3** — shadows and ghosts that drift in and spike the fear meter.
- 🔊 **Procedural audio synth** — wind drone, low hum, footsteps, heartbeat, pickup/sleep/repair SFX, all generated in an SDL audio callback at 44.1 kHz.
- 🌐 **Localization** — English, Russian, Spanish (`lang/*.lang`), including a hand-drawn 5×7 Cyrillic bitmap font.
- 💾 **Save & settings systems** — progress and user prefs (language, mouse sensitivity, view distance) persist between sessions.
- 📱 **Cross-platform** — desktop (Windows / Linux) and Android (NDK + Gradle), with on-screen virtual joysticks on mobile.

---

## 🎮 Controls

### Desktop

| Action | Key |
|---|---|
| Move | `W` `A` `S` `D` |
| Look | Mouse |
| Sprint | `Shift` |
| Interact / Pick up | `E` or `Enter` |
| Toggle flashlight | (unlocked on later days) |
| Release mouse / Pause-look | `Esc` |
| Menu navigation | Arrow keys + `Enter` |

### Android

Virtual joysticks (move + look) and on-screen action buttons rendered by the UI layer.

---

## 📁 Project structure

```text
.
├── CMakeLists.txt          # Unified desktop + Android build
├── build_windows.bat       # One-click Windows build helper
├── lang/                   # en.lang, ru.lang, es.lang
├── src/
│   ├── main.cpp            # SDL init, window, GL context, main loop
│   ├── game.{hpp,cpp}      # Game states, world, quests, AI, weather
│   ├── renderer.{hpp,cpp}  # Shaders, meshes, shadow mapping
│   ├── procedural.{hpp,cpp}# Terrain & prop generation
│   ├── camera.{hpp,cpp}    # First-person camera
│   ├── ui.{hpp,cpp}        # 2D UI, bitmap font, virtual joysticks
│   ├── audio.{hpp,cpp}     # Procedural audio synthesis
│   ├── localization.{hpp,cpp}
│   ├── save_system.{hpp,cpp}
│   ├── settings_system.{hpp,cpp}
│   └── math3d.hpp          # Vec2, Vec3, Mat4, AABB, Frustum
├── android/                # Gradle project, SDLActivity, MainActivity.kt
└── third_party/glad/       # OpenGL loader (desktop only)
```

---

## 🛠️ Building

### Dependencies

- **CMake ≥ 3.16**
- A **C++17** compiler (MSVC 2019+, GCC 9+, Clang 10+)
- **SDL2** — auto-fetched via `FetchContent` if not found on the system
- **OpenGL** runtime (4.5 Core on desktop, ES 3.2 on Android)
- *(Android only)* Android SDK (API 33), NDK with CMake 3.22.1, `curl`

### Windows (one-click)

Double-click `build_windows.bat`. It will:

1. Verify CMake is on `PATH`.
2. Configure & build a Release configuration into `build/`.
3. Copy `AnxietyHorror.exe` to the repo root.

> ⚠️ Some antivirus software (notably Windows Defender) sometimes flags freshly compiled unsigned executables as `Trojan:Win32/Wacatac` — a known false positive. If the build reports success but the `.exe` is missing, restore it from *Windows Security → Protection History*.

### Linux / macOS / Windows (manual)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

The executable will be in `build/` (or `build/Release/` on multi-config generators). The `lang/` folder is copied next to the binary automatically as a post-build step.

Run it:

```bash
./build/AnxietyHorror      # Linux/macOS
build\Release\AnxietyHorror.exe   # Windows
```

### Android

```bash
cd android
./gradlew assembleDebug          # if a wrapper is present
# or
gradle assembleDebug             # using a system-installed Gradle
```

The first build downloads SDL2 (Java sources via a Gradle task, native via CMake `FetchContent`), then produces:

```
android/app/build/outputs/apk/debug/app-debug.apk
```

> The Gradle wrapper (`gradlew`, `gradle/wrapper/…`) is **not** committed to the repo. Either open the `android/` folder in Android Studio to let it generate one, run `gradle wrapper` from a system Gradle, or commit the wrapper yourself.

If you prefer to vendor SDL2, drop a checkout at `third_party/SDL2/` and CMake will use it instead of fetching.

---

## 🗓️ The Nine Days

| Day | Objective |
|----:|---|
| 1 | Explore the valley and the village. |
| 2 | Inspect the old well. |
| 3 | Gather 3 logs. *(Shadows begin to appear.)* |
| 4 | Gather 8 flowers. |
| 5 | Find an old tool to the west. |
| 6 | Collect 6 stones. |
| 7 | Search for rare herbs. |
| 8 | Finish your preparations. |
| 9 | Return to the cabin and end the story. |

Stamina drains while sprinting; fatigue accumulates over the day. Your **heartbeat** in the audio mix tells you how close you are to breaking. If `fear` reaches `1.0`, the fog takes you.

---

## 💾 Save data

- `anxietyhorror_save.dat` — current day, well repaired, logs/flowers/stones counts, tool found, herb found.
- `anxietyhorror_settings.dat` — language index (`0=en 1=ru 2=es`), mouse sensitivity, view distance.

On Android both files live under `SDL_AndroidGetInternalStoragePath()` (app-private storage).

Delete either file to reset to defaults.

---

## 🌐 Localization

Format is a trivial `key = value` text file per language, with `\n \t \r \\ \# \=` escapes supported:

```text
menu.title = AnxietyHorror
menu.subtitle = Nine quiet days in the mist
```

To add a new language, copy `lang/en.lang` to `lang/<code>.lang`, translate the values, and extend the language selector in `settings_system` / `localization`.

The UI ships a built-in ASCII bitmap font plus a minimal 5×7 Cyrillic font rendered directly as colored rectangles, so no external font assets are required.

---

## 🧪 Status & known caveats

- The codebase is compact and monolithic — `game.cpp` carries most of the game logic (~1.9k lines). Convenient for a prototype; ripe for splitting into `WorldSystem / QuestSystem / EnemySystem / WeatherSystem` modules.
- No automated tests or CI yet.
- The Android `README.md` references `./gradlew`, but the wrapper isn't committed.
- The CMake status message mentions OpenGL 4.6 while `main.cpp` actually requests 4.5 — cosmetic mismatch.
- The save system records only counters and flags, not player position / time-of-day / battery — picking up mid-day after a quit isn't supported.

See [`PROJECT_REVIEW_RU.md`](./PROJECT_REVIEW_RU.md) for a deeper architectural review (in Russian).

---

## 🤝 Contributing

Issues and PRs are welcome. Good first directions:

- Splitting the `Game` god-class into focused systems.
- Adding a Gradle wrapper to the Android project.
- A small CI matrix building Windows / Linux / Android on push.
- More languages in `lang/`.
- New procedural props, weather variants, or enemy types.

---

## 📜 License

All rights reserved 2026 AlexanderNyr

---

## 🙏 Credits

- **Author:** [@AlexanderNyr](https://github.com/AlexanderNyr)
- **SDL2** — windowing, input, audio ([libsdl.org](https://www.libsdl.org/))
- **GLAD** — OpenGL function loader (desktop)
- Everything else — generated on the fly by the code in `src/`.
