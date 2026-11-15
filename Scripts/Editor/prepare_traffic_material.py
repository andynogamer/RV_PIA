"""Create the shared unlit parameterized lens material if it is missing."""
import unreal
lib=unreal.EditorAssetLibrary
path='/Game/RutaVirtual/Materials/M_TrafficLamp'
if not lib.does_asset_exist(path):
    material=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_TrafficLamp','/Game/RutaVirtual/Materials',unreal.Material,unreal.MaterialFactoryNew())
    material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    node=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionVectorParameter)
    node.set_editor_property('parameter_name','LampColor')
    node.set_editor_property('default_value',unreal.LinearColor(.02,.02,.02,1))
    unreal.MaterialEditingLibrary.connect_material_property(node,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    lib.save_loaded_asset(material)
RV_RESULT={'lamp_material':path}
