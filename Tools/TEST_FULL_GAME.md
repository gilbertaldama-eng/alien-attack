# EARTH CONTACT: full playable regression test (Unreal Engine 5.8)

This is a manual test plan, not proof that Unreal was compiled or gameplay ran.

## Setup
1. Pull origin on feature/dual-weapons-nordic-boss in GitHub Desktop.
2. Visit https://grab3d.com/models/characters/alien-grey/ and download
   Rigged FBX (listed 19 MB, CC0). Read included license and manifest.
3. Enable Python Editor Script Plugin inside Unreal Engine 5.8.
4. Set EARTHCONTACT_GREY_FBX to the full local downloaded FBX path (restart
   editor) or edit FBX_PATH in Tools/import_grey.py. Execute the script
   in Unreal Editor (Tools > Execute Python Script).
5. Inspect /Game/Characters/Greys/SK_GreyAlien in Content Browser, correct
   facing direction and material assignment as necessary. Source height is
   140cm; in-game hunter collision capsule is scaled to 91.44cm.
6. Compile Development Editor, then open the Solo Hunt launcher.

## Story prologue
- [ ] Player spawns in the little desert home, not the shooting arena.
- [ ] Door is traversable, room props are solid.
- [ ] After about 6 seconds a distant crash flash triggers.
- [ ] About 3.5 seconds later temporary dog silhouette heads toward crash.
- [ ] Five 3-foot Greys appear around the pursuit corridor.
- [ ] Greys use imported FBX and PBR materials instead of primitive shapes;
      check GREY ASSET real skeletal mesh loaded in the log.
- [ ] Imported Grey faces correct direction, feet touch ground and mesh moves
      with character. Idle/walk animation must match imported skeleton.
- [ ] Reaching crash activates combat, not a premature victory.

## Fight and original systems
- [ ] Key 1 = visible AR-style rifle, 30 rounds, fires/reloads.
- [ ] Key 2 = pistol, 17 rounds and semi-auto. Pistol art is still a proxy.
- [ ] Five Greys take damage and poof on death.
- [ ] Nordic spawns once after all five Greys die; mind-control slows player
      temporarily and firing at boss interrupts control.
- [ ] Killing Nordic wins, and Enter allows another attempt.
- [ ] Crashed disk, black jellyfish craft and chrome witch silhouette remain.
- [ ] Play Relay Battle.cmd: original 12 versus 12 still functions.
- [ ] Check for errors and logs: INTRO stage, GREY ASSET, NORDIC BOSS,
      BATTLE end. Screenshot both successful and failed moments.

## Not yet delivered
Finished realistic dog and house art; Grey touch/grab and mimic animation;
cinematic crash audio/VFX; realistic AR/Glock asset; 1.4-second MIB memory
wipe; combo XP/headshot scoring; adaptive cleared soundtrack; plasma sidekick;
flying low-gravity finale; finished Nordic gestures/voice. All require
separate implementation and Unreal playtesting.

The source model is listed as 415k triangles (full), 208k light.
Profile and create LODs before multiplying close-up high-poly enemies.
