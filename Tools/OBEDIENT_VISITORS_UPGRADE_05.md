# EarthContact Upgrade 5 — Obedient Visitors / Overlord command network

## Actual source implementation
- In Solo Hunt, Greys are assigned deterministic directives based on slot:
  advance / flank left / flank right.
- Their movement includes unnatural servo holds, full-stops, and deliberate
  18-degree heading snaps. They act as task-focused automatons instead of
  human soldiers with independent personalities.
- When the 8-foot monster leader appears, every worker updates orders from
  the same five-second command epoch. The three jobs are reassigned in a
  synchronized cycle. An original, generated low robotic tone accompanies
  the leader's order broadcast. No external sample is downloaded.
- Existing close-range reach warning and shooting interruption are retained.
- Before the combat stage, workers do not attack the player.
- Relay Battle does not use the new Solo Hunt command logic.

## Requested battle song
Requested reference video: https://youtu.be/Vph2RfMD6iY

The exact YouTube track was NOT downloaded, copied, embedded, ripped, or
packaged. YouTube availability is not a game soundtrack license, and the
track's exact title/rights could not be verified via the web lookup.

The game now loads an optional Unreal SoundCue:
  /Game/Audio/Music/SC_LicensedBattleCue.SC_LicensedBattleCue
If this asset exists, combat starts it when phase 7 begins, and fades it
out on win/loss. To play a specific track, first obtain a file and
appropriate synchronization/distribution rights (unless the user owns
the track or has permission), import it and create that SoundCue.
Configure the SoundCue to loop if long-running combat is desired.
Without it, combat remains silent from the optional music channel
rather than substituting a ripped or unrelated song.

## Status and tests
GitHub code committed, but Unreal 5.8 compile/run has NOT been performed.
Batch QA: verify mechanical pauses, 18-degree turns, assigned flanks,
boss order broadcast every ~5 seconds, damage and boss progression,
original drone tones, no regression in Relay Battle, and the optional
soundtrack starts/fades when an authorized cue is present.
