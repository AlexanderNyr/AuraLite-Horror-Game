# Implemented Improvement Pass

This document records the major project-wide upgrade pass.

## Stability fixes

### 1. Android ACTION event

Before: `Game::update()` cleared `actionPressed` immediately after entering gameplay, so touch ACTION could be lost before objectives read it.

Now: update captures `interactPressed` once per frame and consumes it after objective handling.

### 2. Billboard mesh corruption / memory leak

Before: Day 3/4/5 collectibles reused `billboardQuad` and called `generateBox()` + `upload()` every frame. `generateBox()` appends vertices, so the mesh grew forever and later billboards rendered with corrupted geometry.

Now: collectibles have dedicated immutable meshes:

- `logMesh`
- `pageMesh`
- `crossVerticalMesh`
- `crossHorizontalMesh`

`billboardQuad` is only used for billboard sprites.

### 3. Cross-platform `M_PI`

`game.cpp` used `M_PI` without defining it. This can fail on MSVC. The file now has a local fallback definition.

### 4. Shader cleanup

Shader compilation/linking error paths now delete created GL resources before returning.

### 5. Safer transparent billboards

Billboards are rendered with depth writes disabled while blending is active. They still respect opaque scene depth, but no longer incorrectly occlude later transparent sprites.

### 6. Precomputed forest instances

Before: tree positions were regenerated in `render()` every frame using the deterministic LCG seed.

Now: the forest layout is generated once during initialization and stored in `treePositions` / `treeScales`.

## Gameplay upgrades

### Stamina

Sprint is no longer free forever. The player has a normalized stamina meter:

- sprint drains stamina;
- walking/standing regenerates it;
- low stamina disables sprint until recovered.

On Android, strong joystick deflection counts as sprint intent.

### Panic / Fear

Seeing entities, staying in darkness and being close to the chaser now increases panic. Panic affects stamina, movement speed, heartbeat and screen overlays.

### Flashlight battery

Days 4-5 now use a finite flashlight battery. Players can toggle it with `F` or Android `LIGHT`; low battery reduces intensity and increases flicker.

### Smarter threats

- Red eyes fade/react under direct flashlight.
- Silhouettes increase panic when watched.
- The Day 5 cross repels silhouettes after pickup.
- The Day 6 chaser has a grace period, adaptive speed and fog repositioning.

### Better HUD

HUD now shows:

- day;
- current objective;
- player coordinates;
- target name and distance;
- stamina bar.

### Scaled mobile controls

Virtual joystick positions/radii are recalculated from the real screen size via `Game::resize()` and `updateTouchLayout()`.

## Build-system upgrades

`CMakeLists.txt` was modernized:

- CMake 3.16 minimum;
- `configure_common_target()` helper;
- warnings for MSVC/GCC/Clang;
- SDL2 config package path;
- SDL2 module fallback;
- FetchContent fallback;
- `OpenGL::GL` imported target.

## Localization

The game now supports external `.lang` files through `src/localization.*`.

- Languages shipped: English, Русский, Espanol.
- Runtime switch: `F2`.
- Startup override: `AURALITE_LANG=ru`.
- Missing keys fall back to English/hardcoded text.
- CMake copies `lang/` next to the desktop executable after build.

See `docs/LOCALIZATION.md`.

## Remaining high-impact future work

1. Fully integrate SDL2 Android AAR/source so Gradle builds an APK out of the box.
2. Add real audio: heartbeat, footsteps, ambient forest, jumpscare sting.
3. Add collision volumes for cabin, trees, well and car.
4. Move day definitions into data structures instead of hardcoded `if/else` blocks.
5. Add save/load or checkpoint system.
6. Add automated CI builds for Windows/Linux.
