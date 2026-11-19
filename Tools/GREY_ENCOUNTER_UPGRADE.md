# EarthContact meaningful upgrade — Grey Close Encounter AI

Implemented on feature/dual-weapons-nordic-boss, isolated to Solo Hunt's
small Greys. The boss, weapons, hunt progression and Relay Battle remain.

### Behavior
- Each Grey has a staggered encounter clock and alternates its lateral
  stalking path instead of always rushing straight at the player.
- At 170–1150 cm from player, the creature stops completely for 1.75 s,
  turns toward the player, and only then resumes stalking.
- At under 170 cm, it pauses and telegraphs a reach for 1.15 s.
  A brief 0.75 s movement suppression and 2 HP contact damage apply only
  if the player remains within 160 cm and unobstructed when the pause ends.
- Shooting a Grey interrupts its staring/reach attempt. Touch has a cooldown
  (~5.25 s after the pause). This is meant to be unnerving, not unavoidable.
- Logs: GREY ENCOUNTER ... motionless stare / reach warning / touch completed.

### Missing art
The behavior works through existing CharacterMovement, collision and game
state, whether the visual model is imported or the fallback. A fully
animated hand-reaching contact gesture is **not implemented**. Rigging,
eye tracking, sounds and facial performance remain separate visual work.

### Verification needed in Unreal 5.8
This is a GitHub source change; it was not built or runtime-playtested here.
Check: Greys freeze, face player, circle, threaten touch, can be escaped,
shooting interrupts a reach, normal death still poofs, boss and Relay remain
unchanged. Check Output Log for GREY ENCOUNTER lines.
