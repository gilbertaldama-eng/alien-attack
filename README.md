# EARTH: CONTACT

Unreal Engine 5.8 C++ source baseline for a third-person Windows co-op mission. The original test range uses engine primitives. The program builds the basin, mesa silhouettes, three survey stations and landing pad at runtime on the engine Entry map. Distances use Unreal centimeters.

## Current build status

Initial checkout contained only Git metadata. Preflight found Visual Studio 2022 Build Tools with the x64/x86 C++ component and Windows SDK 10.0.26100.0. No Unreal installation was identified via PATH, the checked Epic install locations, launcher manifest or registry. The local engine version and available engine plugins could not be inspected. The project targets 5.8 and Win64 and declares no plugin dependencies.

Editor compilation, cooking, standalone play, two-client play, screenshots, network logs and dedicated-server packaging are pending. No Windows executable has been produced.

## Windows setup and launch

Install Unreal Engine 5.8 with Windows support or use an existing 5.8 checkout. For dedicated packaging, use a built source checkout with Setup.bat, GenerateProjectFiles.bat and the Development Editor Win64 build completed. Use the MSVC and Windows SDK versions accepted by that engine's UnrealBuildTool. Set UE_ROOT to the directory containing Engine. Run these PowerShell commands from the repository root.

```powershell
$project = (Resolve-Path .\EarthContact.uproject).Path
$build = Join-Path $env:UE_ROOT 'Engine\Build\BatchFiles\Build.bat'
$editor = Join-Path $env:UE_ROOT 'Engine\Binaries\Win64\UnrealEditor.exe'
& $build EarthContactEditor Win64 Development "-Project=$project" -WaitMutex
if ($LASTEXITCODE -ne 0) { throw 'Editor target build failed' }
& $editor $project /Engine/Maps/Entry -game -windowed -ResX=960 -ResY=640 -log -Log=Solo.log
```

Walk with WASD, aim the camera with the mouse, hold Left Shift to sprint and press E within 2.6 m of each station. Sprint outruns the alien. It deals 20 damage per hit at close range, at most once every 1.25 seconds. All living players must return to the indicated landing pad after the three scans. A squad wipe ends the mission. Start a fresh session for each test.

## Two-client reproduction

Close the solo instance. In PowerShell with the same variables, start a local server and two independent game windows. The editor binary runs the server during this test. This exercises networking without requiring cooked server content.

```powershell
Start-Process $editor -WindowStyle Hidden -ArgumentList @("`"$project`"",'/Engine/Maps/Entry','-server','-port=7777','-log','-Log=SurveyServer.log')
# Wait for the server log to report that the game net driver is listening.
Start-Process $editor -ArgumentList @("`"$project`"",'127.0.0.1:7777','-game','-windowed','-ResX=800','-ResY=600','-Log=ClientA.log')
Start-Process $editor -ArgumentList @("`"$project`"",'127.0.0.1:7777','-game','-windowed','-ResX=800','-ResY=600','-Log=ClientB.log')
```

Alternatively open the editor, select Play As Client, select 2 players and start Play In New Editor Window.

1. Record each window showing both members. Move A, then B, and observe each from the other window.
2. Press E far from a station. The HUD should stay unchanged. Next press E on both clients at the same station. Expect one completed station and one total evidence award.
3. Let the alien hit A once while B watches. Both team panels should show A at 80 HP. Escape, then split the other scans between A and B.
4. Return A alone while B remains alive outside. Both HUDs should continue to request extraction. Bring B onto the pad and capture both EXTRACTED screens.
5. Restart the server and both clients. Allow both players to fall. Capture both SIGNAL LOST screens. Also run the solo win and wipe paths.
6. Preserve the logs in Saved/Logs. Compare the server's CONTACT scan, hit and end entries with each client's observed HUD. A missing window, crash or mismatch is a failed run.

## Dedicated Windows package

From a matching engine source build, use the same project variable as above.

```powershell
$uat = Join-Path $env:UE_ROOT 'Engine\Build\BatchFiles\RunUAT.bat'
$out = Join-Path $PWD 'BuildOutput'
& $uat BuildCookRun "-project=$project" -noP4 -platform=Win64 -server -noclient -serverconfig=Development -build -cook -map=/Engine/Maps/Entry -stage -pak -archive "-archivedirectory=$out"
if ($LASTEXITCODE -ne 0) { throw 'Server packaging failed' }
& $uat BuildCookRun "-project=$project" -noP4 -platform=Win64 -client -clientconfig=Development -build -cook -map=/Engine/Maps/Entry -stage -pak -archive "-archivedirectory=$out"
```

Run the archived server with `-log -port=7777` and each archived client with `127.0.0.1:7777 -windowed -log`. Record actual executable paths and logs after packaging.

## Source map

- EarthContact.uproject declares the Windows project and runtime module.
- Source/EarthContact.Target.cs, EarthContactEditor.Target.cs, EarthContactClient.Target.cs and EarthContactServer.Target.cs define game, editor, client and dedicated builds respectively.
- Source/EarthContact/EarthContact.Build.cs declares the module dependencies. EarthContact.cpp registers it.
- Source/EarthContact/ContactMission.h declares the responder, alien, field, mission rules, replicated state and HUD. ContactMission.cpp implements them.
- Config/DefaultEngine.ini selects the startup map and game mode. DefaultGame.ini sets the 12-player cap and cook inputs. DefaultInput.ini maps keyboard and mouse controls.
- .gitignore excludes engine build products, test logs and machine-specific project files.

First acceptance gate is a compiled standalone run followed by two clients completing both test paths. The next milestone after acceptance is four-player validation and a second creature archetype.
