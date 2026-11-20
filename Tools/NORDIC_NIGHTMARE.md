# Nordic 01 — THE COURTEOUS IMPOSSIBILITY

**Goal:** eight-foot humanlike entity with an almost socially appropriate face
whose gaze, timing, voice and strange gestures make him terrifying.
The suit is futuristic but plausibly functional, not shiny superhero armor.

## Install
- Finish installing **MetaHuman Creator Core Data** in Epic Launcher and restart Unreal.
- Enable **MetaHuman Creator** and **Python Editor Script Plugin**.
- Save MHC_Nordic and **close its MetaHuman editor tab**.
- In Unreal Content Browser, select MHC_Nordic.
- Choose **Tools > Execute Python Script** and run
  \`Tools/nordic_nightmare_preset.py\` from this repository.
- Open \`/Game/Characters/Nordic/MHC_Nordic_NIGHTMARE\` in MetaHuman Creator.
- Inspect appearance; the script applies a pale, desaturated tone, icy eyes,
  darker limbal rings and more pronounced pupils. The exact look depends on
  the character's starting face and Epic's palette. Sculpting and hair are
  left for interactive tuning, not fabricated automation.
- **Original MHC_Nordic is left untouched.** If script fails, inspect the
  duplicate and logs before manually removing or renaming it.
- The script can attach an EXISTING MetaHuman-compatible wardrobe item if
  you set \`OUTFIT_PATH\` in the file. It does not generate a new fitted garment.
- Once you're happy: **Create Full Rig**, then **Assembly > UE Optimized High**
  to create game-ready assets (requires Core Data/cloud sign-in).
- Integrating the assembled MetaHuman in ContactMission.cpp is a separate step;
  this script doesn't silently replace the current procedural boss.

## Art direction: "The Courteous Impossibility"
- Skin: pale and cool grey-beige, healthy subsurface scatter, not blue zombie.
- Eyes: pale steel/silver iris, dark ring, unusually patient stare.
- Face: long, almost symmetrical, cheekbones slightly high, mouth relaxed.
- Expression: minimal; delayed micro-smile only once in a long encounter.
- Hair: smooth silver-white, precisely parted. Avoid glowing hair.
- Scale in *gameplay*: 243.84 cm (8 feet). Maintain sensible capsule and camera.
- Voice: whisper-close but calm, unnaturally polite, humor without punchline.

## Suit design: "EXOSKIN-07"
A **biomimetic breathable jumpsuit** with no seams around the stomach,
a faint micro-porous textile lattice only visible at arm's length, subtle
heat-regulation channels, and ribbed ceramic-soft reinforcement near joints.

Palette: bone-white #CFD2CD base, fog-grey #929A99 microvents,
deep graphite #25292B underarms, silver pearl #B9C2BE piping.
No chrome armor, logos, exposed cables, or bulky spacesuit helmet.

**Reality check:** A clothing spec or an imported material is not a fitted
skinned cloth mesh. Create the garment as a compatible MetaHuman Wardrobe
Item (e.g. from Blender/Marvelous Designer), import and fit it with
MetaHuman clothing tools, then set OUTFIT_PATH in the script. Fabric
breathability is a *visual design characteristic*; real cloth simulation
requires a separate Chaos Cloth setup and testing.

## Horror behavior once connected to the game
- Stands in the distance; never uses obvious monster idle motion.
- Points two fingers at player, inducing short movement suppression.
- One shooting hit breaks control briefly but does not remove him.
- Laugh-like exhalation exactly once when the dog passes.
- Gentle phrase after first gunfire: "It was yours before you found it."
- His appearance, sounds and mind-control effects must remain fair and
  interruptible; do not disable player agency for long stretches.

## Links
Official MetaHuman Python API:
https://dev.epicgames.com/documentation/metahuman/metahuman-creator-python-scripting-in-unreal-engine?lang=en-US
Official MetaHuman wardrobe guidance:
https://dev.epicgames.com/documentation/metahuman/hair-and-clothing-tools?application_version=5.8

**Not yet built/tested in UE 5.8.** This is a source-controlled automation
starter, and the editor may report version-specific API differences.
