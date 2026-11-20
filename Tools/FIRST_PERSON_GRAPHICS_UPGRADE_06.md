# EarthContact Upgrade 6 — First-person presentation + UE 5.8 renderer

## Actual implementation
- Solo Hunt automatically switches to a true eye-level UCameraComponent
  mounted on the character capsule; the previous over-the-shoulder
  camera remains available for Relay Battle.
- Dedicated camera-relative view pistol and rifle meshes are visible only
  to their owner. The mannequin mesh is hidden only from the local owner,
  not removed from the world. Existing gun logic, ammunition and
  third-person Relay Battle remain.
- First-person gun kick/recovery on firing, subtle motion bob while moving,
  and spring interpolation between 92-degree walk FOV and 98-degree
  sprint FOV. No forced camera shake or excessive motion effects.
- In Solo Hunt player shot traces originate at the camera for accurate
  crosshair alignment. AI and Relay keep their existing firing origin.
- Fallback camera-view pistol has grip, guard and sight pieces when a
  real pistol mesh hasn't been imported.
- UE5 rendering: software Lumen GI/reflections, TSR anti-aliasing,
  virtual shadow maps, original bloom preserved, 100% screen percentage.
  No hardware ray-tracing requirement or additional download.

## Honest limitations
- Camera-attached weapons are view models, not full first-person arm rigs.
  Authentic hand, reload and slide animations require a compatible mesh,
  animation assets and retargeting.
- Lumen is a renderer feature, not a replacement for realistic materials
  or high-quality 3D models. Terrain and house art still need upgrades.
- Changes were committed directly to GitHub but NOT compiled or run in
  the user's Unreal 5.8 installation. Correct model orientation,
  first-person clipping, performance, lighting and actor visibility
  require actual runtime playtesting.

## QA batch checklist
- Solo Hunt: correct eye-level view; move/look, inspect first-person pistol,
  fire with centered crosshair; reload; key 1 rifle; key 2 pistol.
- Verify gun models follow pitch, don't occlude camera, don't collide.
- Test during intro and after abduction auto-rifle switch.
- Sprint FOV smoothly changes, motion bob isn't excessive, recoil settles.
- Confirm Rift/Relay Battle uses third-person camera and original logic.
- Verify Lumen GI, reflections, TSR, shadows under actual GPU load.
- Check Unreal compilation and log before considering verified.
