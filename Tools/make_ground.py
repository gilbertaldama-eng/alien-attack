"""Generate the station's sand material in UE's editor Python environment."""
import unreal

assets = unreal.AssetToolsHelpers.get_asset_tools()
material = assets.create_asset("M_ArroyoSand", "/Game/Field", unreal.Material, unreal.MaterialFactoryNew())
if material:
    graph = unreal.MaterialEditingLibrary
    def node(kind, x, y):
        return graph.create_material_expression(material, kind, x, y)
    def color(value, x, y):
        expression = node(unreal.MaterialExpressionConstant3Vector, x, y)
        expression.set_editor_property("constant", unreal.LinearColor(*value, 1))
        return expression
    # Object is a single terrain mesh. Local space keeps the sand texture fixed.
    position = node(unreal.MaterialExpressionWorldPosition, -750, 150)
    noise = node(unreal.MaterialExpressionNoise, -480, 150)
    noise.set_editor_property("scale", 0.075)
    noise.set_editor_property("levels", 3)
    noise.set_editor_property("output_min", 0.0)
    noise.set_editor_property("output_max", 1.0)
    graph.connect_material_expressions(position, "", noise, "WorldPosition")
    blend = node(unreal.MaterialExpressionLinearInterpolate, -120, 0)
    graph.connect_material_expressions(color((0.18, 0.062, 0.024), -480, -180), "", blend, "A")
    graph.connect_material_expressions(color((0.48, 0.235, 0.10), -480, -40), "", blend, "B")
    graph.connect_material_expressions(noise, "", blend, "Alpha")
    graph.connect_material_property(blend, "", unreal.MaterialProperty.MP_BASE_COLOR)
    graph.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
