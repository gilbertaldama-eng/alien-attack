"""
EARTHCONTACT / NORDIC_01 — Unreal 5.8 MetaHuman spooky-preset installer

Run in the Unreal Editor with Python Editor Script Plugin enabled.
Select your existing MHC_Nordic MetaHuman Character in the Content Browser
or let the script find MHC_Nordic by name.

Safety:
- Never changes source MHC_Nordic; duplicates it to MHC_Nordic_NIGHTMARE.
- Never overwrites an existing duplicate. Stop if output already exists.
- Does not create a skin-tight jumpsuit from thin air: outfit must be a real
  compatible MetaHuman wardrobe item already imported (OUTFIT_PATH below).
- Logs every applied edit. Some MetaHuman 5.8 APIs require Core Data;
  do not proceed until the missing Core Data warning is resolved.
- Close MHC_Nordic's asset editor before executing this script.

Unreal menu: Tools > Execute Python Script > this file.
"""
import unreal

SOURCE_NAME = "MHC_Nordic"
OUTPUT_NAME = "MHC_Nordic_NIGHTMARE"
OUTFIT_PATH = ""  # e.g. "/Game/Characters/Nordic/Outfits/WI_BreathableSuit.WI_BreathableSuit"
DEST_FOLDER = "/Game/Characters/Nordic"

def note(message):
    unreal.log("[NORDIC_01] " + message)

def get_source():
    selected = list(unreal.EditorUtilityLibrary.get_selected_assets())
    for item in selected:
        if item.get_name() == SOURCE_NAME and item.get_class().get_name() == "MetaHumanCharacter":
            return item
    matches = [p for p in unreal.EditorAssetLibrary.list_assets("/Game", recursive=True, include_folder=False)
               if p.split("/")[-1].split(".")[0] == SOURCE_NAME]
    if len(matches) != 1:
        raise RuntimeError("Select MHC_Nordic in Content Browser; found {} matching assets.".format(len(matches)))
    return unreal.load_asset(matches[0])

def set_nested_eyes(eyes):
    for side in ("eye_left", "eye_right"):
        eye = eyes.get_editor_property(side)
        iris = eye.get_editor_property("iris")
        iris.set_editor_property("global_saturation", 0.18)
        iris.set_editor_property("global_tint", unreal.LinearColor(0.65, 0.73, 0.79, 1.0))
        iris.set_editor_property("limbal_ring_color", unreal.LinearColor(0.035, 0.045, 0.058, 1.0))
        eye.set_editor_property("iris", iris)
        pupil = eye.get_editor_property("pupil")
        pupil.set_editor_property("dilation", 0.78)
        eye.set_editor_property("pupil", pupil)
        eyes.set_editor_property(side, eye)
    return eyes

def apply_appearance(character, subsystem):
    skin_settings = character.get_editor_property("skin_settings")
    skin = skin_settings.get_editor_property("skin")
    # These are palette coordinates, not literal RGB. Preview and tune in editor.
    skin.set_editor_property("u", 0.18)
    skin.set_editor_property("v", 0.14)
    skin.set_editor_property("roughness", 0.95)
    skin_settings.set_editor_property("skin", skin)
    subsystem.commit_skin_settings(character, skin_settings)
    note("Applied cool, desaturated Nordic skin palette and natural skin sheen")

    eyes = character.get_editor_property("eyes_settings")
    subsystem.commit_eyes_settings(character, set_nested_eyes(eyes))
    note("Applied icy near-monochrome eyes, dark limbal ring, large pupils")

def apply_outfit_if_present(character):
    if not OUTFIT_PATH:
        note("No jumpsuit Wardrobe Item specified; refusing to pretend it exists.")
        return
    outfit = unreal.load_asset(OUTFIT_PATH)
    if not outfit:
        note("Wardrobe item not found: " + OUTFIT_PATH)
        return
    # Official MetaHuman slot-based wardrobe API; outfit must be compatible.
    collection = character.get_editor_property("internal_collection")
    key = collection.try_add_item_from_wardrobe_item("Outfits", outfit)
    if not key:
        note("Could not register outfit; check it is a MetaHuman Wardrobe Item.")
        return
    selection = unreal.MetaHumanPipelineSlotSelection()
    selection.set_editor_property("slot_name", "Outfits")
    selection.set_editor_property("item_key", key)
    collection.get_editor_property("default_instance").try_add_slot_selection(selection)
    note("Attached user-supplied MetaHuman-compatible outfit")

source = get_source()
if not source:
    raise RuntimeError("MHC_Nordic not found.")
original_path = source.get_path_name().split(".")[0]
destination = DEST_FOLDER + "/" + OUTPUT_NAME
if unreal.EditorAssetLibrary.does_asset_exist(destination):
    raise RuntimeError(
        "Output already exists: {}. Rename or remove only that duplicate yourself before rerunning.".format(destination)
    )
if not unreal.EditorAssetLibrary.does_directory_exist(DEST_FOLDER):
    unreal.EditorAssetLibrary.make_directory(DEST_FOLDER)
created = unreal.EditorAssetLibrary.duplicate_asset(original_path, destination)
if not created:
    raise RuntimeError("Could not duplicate MHC_Nordic. Source was not modified.")
character = unreal.load_asset(destination)
if not character:
    raise RuntimeError("Duplicated character did not load; inspect " + destination)
if not hasattr(unreal, "MetaHumanCharacterEditorSubsystem"):
    raise RuntimeError("MetaHuman Character Editor subsystem unavailable. Enable MetaHuman Creator plugin.")
subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
already = subsystem.is_object_added_for_editing(character)
if not already and not subsystem.try_add_object_to_edit(character):
    raise RuntimeError("Cannot enter edit session on duplicate. Close the asset editor and retry.")
try:
    apply_appearance(character, subsystem)
    apply_outfit_if_present(character)
    unreal.EditorAssetLibrary.save_loaded_asset(character)
    note("SAVED: " + destination)
    note("Open the duplicated MetaHuman in Content Browser. Finish facial sculpt, hair, and rig/assembly.")
    note("Nordic game height 243.84cm is applied on the in-game actor, not as a universal preset height.")
except Exception as error:
    unreal.log_error("[NORDIC_01] Edit failed on duplicate: " + str(error))
    raise
finally:
    if not already:
        subsystem.remove_object_to_edit(character)
