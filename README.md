# EARTH: CONTACT — Roswell Signal Station

Double-click **Play Earth Contact.cmd** on Windows with Unreal Engine 5.8.4 installed. It updates the project module if needed and opens the standalone game. Set `UE_ROOT` if Unreal is installed outside the launcher's default Epic Games folder.

The default **Solo Hunt** pits you alone against five one-foot aliens. Each has 1,000 HP and chases you to attack at close range. Each rifle hit deals 2 damage, so one alien takes 500 hits. Eliminate all five to win. Spare magazines are unlimited.

For the original battle, double-click **Play Relay Battle.cmd**. You deploy behind **12 allied AI soldiers** facing **12 alien raiders**. Sprint through the yard to the relay mast and spend ten seconds within four metres, or eliminate the hostile squad. Relay progress is cumulative. Concrete cover and buildings stop rifle rounds. The relay's east wall shields an operator approaching from the allied side. If the player falls, the mission ends. Press **Enter** after victory or defeat to redeploy.

| Control | Action |
|---|---|
| WASD / mouse | Move / aim the shoulder camera |
| Left mouse | Automatic rifle fire |
| R | Replace the 30-round magazine in 1.6 seconds; spare magazines are unlimited |
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
