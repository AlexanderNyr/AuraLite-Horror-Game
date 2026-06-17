# Realistic graphics pass

This pass makes the project look less flat while still keeping the no-assets/procedural approach.

## Main shader

`src/game.cpp` now uses a more cinematic 3D fragment shader:

- gamma-aware albedo handling;
- ACES-like filmic tonemapping;
- hemisphere ambient lighting;
- softer wrapped directional light;
- roughness-based specular highlights;
- warmer flashlight hotspot and softer spill;
- animated procedural fog density;
- height fog near the ground;
- distance desaturation for a cold horror look.

## Textures

`Texture::generateNoise()` now produces richer procedural materials:

- wood has layered grain, plank seams and damp darkening;
- ground has moss/mud variation and small deterministic pebble flecks.

## Menu atmosphere

A real main menu was added before the diary intro:

- localized title/subtitle;
- language selection;
- control hints;
- moving fog-like bands.

## Limitations

The project still has no imported PBR assets, normal maps or shadow maps. The changes are shader/procedural improvements, not a full asset replacement pipeline.
