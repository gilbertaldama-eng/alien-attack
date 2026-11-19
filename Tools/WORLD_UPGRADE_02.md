# EarthContact Upgrade 2 — Shaped Desert and Impact Crater

Branch: feature/dual-weapons-nordic-boss

## Implemented
- The procedural terrain moves from a 111 x 111 grid sampled at 240 cm
  to a 161 x 161 grid sampled at 170 cm. The geographic span remains
  approximately the same and collision is generated as before.
- Solo Hunt uses layered low-frequency ridges and a dry arroyo.
- The direct house-to-crash route stays relatively gentle, and the
  house footprint is flattened to protect the gameplay introduction.
- A real ground depression (145 cm maximum) and raised ejecta rim
  (105 cm maximum) surround the UFO crash at (3600,-2600).
- Mesh vertex colors shade sandy flats and steeper rock surfaces.
  NOTE: the material must use Vertex Color for these colors to display.
- Missing optional rock assets no longer cause a null-dereference crash.
- The existing /Game/Field/M_ArroyoSand material remains preferred; a
  visible engine material is used if it is missing.
- Relay mode keeps its original station terrain behavior.

## Not claimed
- This is procedural terrain geometry, NOT a saved Unreal Landscape
  editor asset, Nanite landscape, scanned photogrammetry, or a finished
  photo-realistic environment.
- The code has been committed but NOT compiled or visually tested in
  the user's Unreal Editor. Crater appearance, seams, collision,
  player navigation and actor alignment require an actual playtest.
- Imported 3D models are untouched; the playable home and character
  models are separate visual upgrades.

## Build / acceptance checks for later batched QA
- Compile UE 5.8 project, launch Solo Hunt.
- Enter and exit home without collision traps.
- Walk from home to crater: terrain should be passable.
- Verify spacecraft visually sits within or against rim, not floating.
- Greys spawn on traversable ground, can follow around crater.
- Verify Relay Battle remains playable with station cover.
- Check Output Log for WORLD terrain grid and any missing material warnings.
