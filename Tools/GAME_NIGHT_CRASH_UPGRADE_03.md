# EarthContact Upgrade #3 — The game-night crash opening

Branch: feature/dual-weapons-nordic-boss

## Sequence (Solo Hunt only)
1. **0–7 seconds:** Full-screen ORIGINAL arcade military-shooter parody
   "DUTY CALLS / DUST WARFARE". A faux FPS HUD and targets animate on
   the screen. This is **not** Call of Duty gameplay or copyrighted assets.
   Player walking/shooting are suppressed in this opening vignette.
2. **7–11 seconds:** Dog barks repeatedly, screen shows barking warning.
   Barking audio is synthesized in C++ at runtime and sounds stylized;
   no sound pack download is required.
3. **11–16 seconds:** An intact UFO mesh (if imported/resolved) visibly
   descends along a curved/damped interpolation and rotates through the
   sky toward the crater. A built-in cylinder is a visual fallback.
4. **16 seconds:** Collision impact swaps to the crashed UFO mesh when
   available, plays original generated low-frequency boom, spawns a
   bright transient impact light and persistent blue wreck glow.
   The spacecraft remains at the crater for investigation.
5. **Afterward:** Dog runs toward the crash, Greys appear, the existing
   pursuit and monster-boss fight continue.

## Starter weapon
- Solo Hunt now begins with the 17-round semi-auto pistol equipped.
- Prefer an existing project mesh at
  /Game/Weapons/Pistol/Meshes/SM_Pistol.SM_Pistol.
- If absent, a distinctive block-built handgun-shaped fallback replaces
  the former single cube, with slide, angled grip, trigger guard, sights.
- Rifle remains accessible via key 1, pistol via key 2; no new purchase.

## Preserved
- Imported assets and Unreal project content not modified or deleted.
- Relay Battle remains a separate mode.
- Enemy AI, ammo and reload logic, dog pursuit and boss mission still
  use previous gameplay systems.

## Important limitations (not falsely claimed as complete)
- The shooter opening is an on-screen 2D faux-game sequence, **not** an
  animated cinematic of a seated person holding a controller.
- The pistol is Glock-*style* geometry unless an actual licensed model
  is present; no Glock asset or licensed logo was installed.
- Synthesized audio is temporary, not studio-quality dog recordings.
- No Niagara smoke, hand animation, furniture cinematic camera, or
  polished weapon reload animation is implemented in this pass.
- The source has been committed, but UE 5.8 compilation, runtime
  animation, collision, lights and audio have NOT been verified.

## Batch QA checks (when owner chooses to review)
- Launch Solo Hunt fresh and watch in one run: 7s game -> 4s dog barks
  -> 5s descending saucer -> one impact -> dog runs -> Grey sighting.
- Verify no pre-spawned crashed craft or glow is visible before impact.
- Verify gun starts on pistol with 17 rounds and can fire/reload after
  the vignette. Press 1 for rifle / 2 for pistol.
- Verify actual UFO models appear when already imported and mapped.
- Verify under missing model paths, fallback renders without crashing.
- Verify no immediate Grey attacks during the introduction.
- Verify Relay Battle spawns all original combatants and cover.
- Inspect Unreal Output Log for INTRO stage, INTRO falling UFO,
  INTRO UFO impacted, INTRO starter pistol equipped.
