# EARTH: CONTACT — Roswell Signal Station

Double-click **Play Earth Contact.cmd** on Windows with Unreal Engine 5.8.4 installed. It updates the project module if needed and opens the standalone game. Set `UE_ROOT` if Unreal is installed outside the launcher's default Epic Games folder.

The default **Solo Hunt** pits you against five one-foot aliens (90 HP each). Clearing all five triggers a second phase: an eight-foot, pale Nordic-alien boss (550 HP). Defeat the boss to win. This is a **code-only prototype**: the boss currently uses the Quinn mannequin and procedural alien shell with an attempted pale material tint, not a finished bespoke character asset. The pistol is currently a primitive box-mesh placeholder. Art, animations, visual tuning and in-engine build verification remain outstanding.

For the original battle, double-click **Play Relay Battle.cmd**. You deploy behind **12 allied AI soldiers** facing **12 alien raiders**. Sprint through the yard to the relay mast and spend ten seconds within four metres, or eliminate the hostile squad. Relay progress is cumulative. Concrete cover and buildings stop rifle rounds. The relay's east wall shields an operator approaching from the allied side. If the player falls, the mission ends. Press **Enter** after victory or defeat to redeploy.

| Control | Action |
|---|---|
| WASD / mouse | Move / aim the shoulder camera |
| Left mouse | Automatic rifle fire; press per shot for pistol |
| 1 / 2 | Switch between AR-style rifle (30 rounds) and Glock-style pistol prototype (17 rounds) |
| R | Reload current weapon (rifle 1.6s, pistol 1.25s); spare magazines are unlimited |
| Left Shift | Sprint |
| Enter | Restart a completed mission |

Cyan bars identify allies and red bars identify raiders. The squad counters exclude the player. Both teams select visible opponents, use individual firing positions, peek around physical cover, and briefly retreat behind it when hit. The AI uses assigned positions in this station rather than a general navigation system.

## Verification

Build the Development Editor target and launch a real standalone game from PowerShell:

```powershell
$engine = 'C:\Program Files\Epic Games\UE_5.8\Engine'
$project = (Resolve-Path EarthContact.uproject).Path
& "$engine\Build\BatchFiles\Build.bat" EarthContactEditor Win64 Development "-Project=$project" -WaitMutex
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
& "$engine\Binaries\Win64\UnrealEditor.exe" $project "/Engine/Maps/Entry?Mission=Relay" -game -windowed -Log=Station.log
```

For the repeatable relay test, add `-ExecCmds=Contact.TestRelay` on launch. It drives the regular player controls in a fresh mission, records initial counts and the outcome, and requests two screenshots. Search the project log for `RELAY TEST` and `BATTLE`. Enemy hits identify shooter and target slots. Slot -1 is the human player. Playtest evidence is in the Evidence directory.

## Files and assets

- `ContactMission.h/.cpp` contain the player, weapons, AI decisions, cover response, spawning, combat state and mission rules.
- `ContactField.cpp` builds the terrain, light, sky, radio buildings, receiver dish, debris, obstacles and relay.
- `ContactHUD.cpp` draws the reticle, team indicators, health, ammo, objective and match result.
- `RelayPlaytest.cpp` contains the opt-in development playtest.
- `Config/DefaultInput.ini` maps the controls. `DefaultEngine.ini` configures the map, game mode and rendering. `EarthContact.uproject` and `EarthContact.Build.cs` enable Unreal's bundled Procedural Mesh Component.
- `Content/Characters`, `Weapons`, `XRFramework` and `SimSandbox` contain dependencies copied from the local Unreal 5.8 template resources: Manny, Quinn, rifle animations, rifle mesh, fire sound and rock mesh. Epic template content remains subject to Epic's applicable Unreal licensing terms. The raiders use Quinn under an original crested shell and mandibles.
- `Tools/make_ground.py` authored the included `Content/Field/M_ArroyoSand.uasset` in Unreal Editor. Normal gameplay loads the material directly.

This milestone targets standalone single-player in the Development build. Packaged Windows and multiplayer behavior have separate acceptance requirements.

## Dual-weapon / Nordic-boss prototype verification (pending)

The feature is staged on a review branch. It has **not** yet been compiled or playtested in Unreal after these changes. Build `EarthContactEditor` using the commands above, then verify: keys 1/2 swap visible weapons, magazines retain ammo across swaps, pistol requires fresh trigger presses, reloads complete independently, five miniature hunters can be defeated, the tall boss spawns once, defeating it wins, losing and restarting works, and Relay mode still works. Check mannequin proportions, white material tint, camera clipping, spawn collision and sound before calling the boss presentation complete. 

## Experimental teleport presentation

On the feature branch, the Nordic boss arrival hides the character for 0.23 seconds while a temporary pale-blue/white point light flashes at the spawn location; after the flash the boss becomes visible. Defeated aliens flash and vanish rather than leaving corpses. This is a **programmatic lighting prototype**, not a finished cinematic Niagara effect, screen bloom, particle burst, audio cue or custom animation. The renderer configuration currently disables bloom, so review the in-game visibility of the effect. Verify in UE 5.8 before merging; the repository was updated without a local engine build.

## Alien invasion visual / psychic prototype — not yet engine-tested

- The desert scene now stages **three procedural silhouettes**: a hovering black jellyfish UFO with hanging strands, a chrome broom-riding witch, and a partially buried crashed disk. They use existing Unreal basic meshes, not final authored/rigged production models. The original relay-station geometry is retained so the alternate Relay mission still has cover.
- The Nordic boss projects a line-of-sight mind-control attack while nearby: player walk speed is reduced for 1.65 seconds, with a 5.25-second attack interval. Player hits briefly interrupt the boss for 0.33 seconds. Hand-pointing/temple gestures, screen distortion and boss voice are **not** yet animated/implemented.
- Open issues: cinematic arrival, aerial low-gravity battle, Men-in-Black 1.4-second memory-wipe encounter, combo XP/headshot scoring, plasma sidekick and getaway, finished character models and audio.
- **Music:** a YouTube watch URL is not a distributable audio asset or a license. Do not rip or embed third-party music without permission; use cleared music stems to implement adaptive playback up to 1.6x rate.
- **Verification:** The GitHub-only changes have not been compiled or playtested in Unreal Engine 5.8. Test walk-speed recovery, mind-control line of sight and shooting interruption, nighttime/sky readability, Unreal asset loading and both Solo Hunt/Relay modes before merging.
