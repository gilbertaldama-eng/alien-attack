# EarthContact Upgrade 7 — Free Epic/Fab assets, integrated safely

Date: November 2026. Scope: Solo Hunt (Relay Battle unchanged).

## Code committed
The world now searches installed Unreal Static Mesh assets in:
- /Game/Megascans/3D_Assets
- /Game/Megascans
- /Game/EarthContact/Environment
- /Game/Environment
- /Game/CitySampleVehicles

For eligible asset names it chooses deterministic first matching mesh:
- DesertRock: rock / boulder / sandstone
- DesertPlant: cactus / yucca / shrub / bush / dryplant
- AbandonedVehicle: pickup / truck / sedan / carbody

Explicitly mapped models in `Config/ImportedAssets.ini` override discovery.

When available:
- Up to 110 copies of a scanned rock, at varied scale and rotation.
- Up to 165 desert shrubs/plants, varied to break up the flat landscape.
- One parked vehicle by the homestead.
- Ground heights sampled against the existing procedural terrain.
- Corridor and house/crash clearances preserved.
- Instanced mesh rendering to avoid hundreds of individual actors.
- Optional assets absent: gracefully skip. No new paid APIs, no downloads.
- Asset pick and placement counts logged under FREE ASSET / FREE PACK WORLD.

## Current free packs worth considering — verified listings
1. Quixel Megascans through Fab/UE (ground surfaces, rocks, desert details).
   https://dev.epicgames.com/documentation/unreal-engine/free-epic-games-content-for-unreal-engine
2. FPS Weapon Bundle (free, multiple rigged first-person weapons).
   https://www.fab.com/listings/8aeb9c48-b404-4dcd-9e56-1d0ecedba7f5
3. City Sample Vehicles (free vehicles, selective migration recommended).
   https://www.fab.com/listings/2909157b-ddfa-4cef-a925-69dc2467021f
4. Animation Starter Pack (free, older mannequin skeleton; retarget needed).
   https://www.fab.com/listings/98ff449d-79db-4f54-9303-75486c4fb9d9
5. Paragon: Agora and Monolith Environment (free props/FX, aesthetic less
   relevant to grounded desert: defer unless needed).
   https://www.fab.com/listings/6f401fb5-88b5-41b4-bf1b-62321414e1f0

## Do NOT do
- Do not add entire City Sample to EarthContact; huge/unnecessary.
- Do not add huge Paragon libraries without a specific purpose.
- Do not claim an external Fab pack is installed or imported via GitHub.
- Do not auto-install packs requiring an Epic login or accept a license.
- Do not run old MetaHuman scripts.

## Status and limitations
GitHub C++ code is committed. Actual Unreal compilation and playtesting are
pending. Exact mesh scales, terrain grounding, FPS, foliage density and
license suitability must be checked in Unreal before shipping. Some Megascans
content on Fab may have differing license conditions; verify individual
asset license at acquisition. Naming-based discovery can pick inappropriate
meshes; use Config/ImportedAssets.ini to override once inspected.
