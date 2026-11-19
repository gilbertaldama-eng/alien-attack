# EarthContact imported model hookup — Unreal Engine 5.8

Code committed to the existing feature branch. Imported Unreal assets remain local on your PC unless committed/pushed.

## Unreal Content Browser (NOT Windows Explorer)

Put **exactly one** model of the indicated type in each dedicated folder:

| Purpose | Unreal asset type | Content Browser folder |
| --- | --- | --- |
| 3-foot Grey | Skeletal Mesh | /Game/Characters/Greys |
| Monster leader | Skeletal Mesh | /Game/Characters/MonsterLeader |
| Crashed saucer | Static Mesh | /Game/Vehicles/UFOs/Crashed |
| Intact flying saucer | Static Mesh | /Game/Vehicles/UFOs/Intact |

The model name can vary. Textures, materials and skeleton assets can share the folder.
If there are multiple Skeletal Mesh or Static Mesh candidates in a given folder,
the resolver refuses to choose arbitrarily and logs the candidates.
The Grey supports the original path /Game/Characters/Greys/SK_GreyAlien too.
An alternative for monster is a single Skeletal Mesh in /Game/Characters/Monsters.

## Play it

1. In Unreal, choose File > Save All, then close the editor.
2. In GitHub Desktop on feature/dual-weapons-nordic-boss, click Fetch origin and Pull origin. Preserve all your locally imported assets; don't discard uncommitted .uasset files.
3. Double-click Play Earth Contact.cmd to rebuild and launch.
4. Leave the house; see the crash, dog pursuit, Greys, and boss. Inspect the new meshes.
5. Logs should show CONTACT ASSET selected ... and CONTACT UFOs intact=1 crashed=1.
6. Test Play Relay Battle.cmd to ensure the original mode remains functional.

## Important limits

- This C++ change is not yet compiled or gameplay-tested on the user's Unreal installation. Red compiler errors need correction before treating this as working.
- The character replacements require actual Skeletal Meshes. A static-only monster won't be used as the boss.
- This is model hookup, not automatic animation retargeting. Native Grey Idle/Walk clips must reference the imported skeleton.
- Position/scale of imported UFOs is approximate and needs visual inspection.
- AssetRegistry discovery in an Editor standalone game does not guarantee cooking/packaging. Before packaging, add explicit cook references or primary asset rules.
- Local .uasset files aren't uploaded merely by pulling source from GitHub. They must be saved and committed separately.
- Don't rerun the unverified MetaHuman nightmare script; it previously caused a crash.
