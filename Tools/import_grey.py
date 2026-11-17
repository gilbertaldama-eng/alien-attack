"""
EarthContact - import selected CC0 rigged Grey alien FBX in Unreal Editor.

Model: https://grab3d.com/models/characters/alien-grey/
License declared CC0 by source; inspect model's LICENSE.txt and manifest.json.

1. Download "Rigged FBX" to your computer.
2. Enable "Python Editor Script Plugin" in Unreal 5.8; restart editor.
3. Set the EARTHCONTACT_GREY_FBX environment variable to its full local path,
   or edit FBX_PATH here; restart editor after setting variable.
4. Tools > Execute Python Script -> this file.
5. Inspect /Game/Characters/Greys/SK_GreyAlien and skeletal animations.
"""
import os
import unreal

DEST = "/Game/Characters/Greys"
FBX_PATH = os.environ.get("EARTHCONTACT_GREY_FBX", "").strip()
# Or enter your actual local full path here:
# FBX_PATH = r"C:\Users\Gilbert\Downloads\alien-grey-rigged.fbx"

if not FBX_PATH:
    raise RuntimeError("Set EARTHCONTACT_GREY_FBX or edit FBX_PATH inside Tools/import_grey.py")
if not os.path.isfile(FBX_PATH):
    raise FileNotFoundError(FBX_PATH)
if unreal.EditorAssetLibrary.does_asset_exist(DEST + "/SK_GreyAlien"):
    raise RuntimeError("SK_GreyAlien already exists. Inspect it before replacing.")

options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_as_skeletal", True)
options.set_editor_property("import_animations", True)
options.set_editor_property("import_materials", True)
options.set_editor_property("import_textures", True)
options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)

task = unreal.AssetImportTask()
task.set_editor_property("filename", FBX_PATH)
task.set_editor_property("destination_path", DEST)
task.set_editor_property("destination_name", "SK_GreyAlien")
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", False)
task.set_editor_property("save", True)
task.set_editor_property("options", options)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
paths = list(task.get_editor_property("imported_object_paths"))
unreal.log("GREY IMPORT: imported paths = " + str(paths))
mesh = unreal.EditorAssetLibrary.load_asset(DEST + "/SK_GreyAlien")
if not mesh or not isinstance(mesh, unreal.SkeletalMesh):
    raise RuntimeError("Expected skeletal mesh missing at " + DEST + "/SK_GreyAlien")
unreal.EditorAssetLibrary.save_directory(DEST)
unreal.log("GREY IMPORT: skeletal mesh ready. Check axes, skin, height and imported animation clips.")
unreal.log_warning(
    "Animation names vary by FBX exporter. Create/rename valid matching Grey "
    "animation sequences to AS_GreyIdle and AS_GreyWalk in this content folder. "
    "Do not use Manny/Quinn clips on the Grey skeleton."
)
