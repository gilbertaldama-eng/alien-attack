# EarthContact Upgrade #4 — Second UFO abducts the dog

Implemented on feature/dual-weapons-nordic-boss. No local Unreal build was run.

## Complete stage sequence
0  original game-on-TV parody (7 seconds)
1  dog barking alarm (4 seconds)
2  first UFO descends (5 seconds) and crashes
3  dog runs to the impact wreck (up to 20 seconds)
4  second UFO approaches and hovers over the dog (3.2 seconds)
5  blue tractor beam illuminates the dog, who rises visibly (3.4 seconds)
6  abducting UFO escapes into the sky with dog hidden, NOT destroyed (3 seconds)
7  five Greys appear; a second three-Grey reinforcement wave spawns four
   seconds later; surviving the waves unlocks the original monster boss

## Player changes
- Dog ownership/rescue objective appears on the HUD: "RESCUE OBJECTIVE: FIND YOUR DOG".
- Existing rifle equips automatically as the craft takes the dog; pistol
  magazine is preserved for switching back later.
- Player remains in control during abduction. No invincibility.
- The dog actor is hidden, not destroyed. The mission's rescue narrative
  is unresolved; full recovery/rescue mechanic remains future work.
- Attack-stage AI and boss trigger are gated until after abductor escape
  and reinforcement wave so there is no premature boss.
- If second craft cannot be spawned, after 4 seconds of stage 4 the game
  enters the rescue battle rather than hanging indefinitely.

## Art pipeline
- The **second** craft uses the existing imported IntactUFO mesh via the
  asset resolver. If missing, a basic built-in cylinder is used.
- Beam uses an Unreal built-in cylinder mesh and dynamically-tinted
  basic material, with a blue point light. It is not a polished transparent
  particle cone. Dust vortex/audio modulation are not yet implemented.
- No imported source models, materials, or Unreal maps were modified.
- Relay Battle is preserved.

## QA needed when user next batches a playtest
- Compile UE 5.8; check includes, UHT reflected types and world actor spawns.
- Watch second UFO arrive over dog, beam appear, dog rise, hide, craft flee.
- Confirm HUD objective and rifle switch; press 2 returns pistol and ammo.
- Fight 5 initial Greys then 3 reinforcements; verify boss cannot spawn
  until both waves defeated.
- Verify dog actor remains valid and hidden rather than destroyed.
- Repeat with missing UFO asset path to verify failure fallback.
- Open Relay Battle and confirm no regression.

Status: committed source only. Not compile-tested or runtime-verified.
