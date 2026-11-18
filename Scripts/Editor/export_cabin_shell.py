"""Export the training car's two exterior material sections for Blender's cabin cut."""
import json
from pathlib import Path
import unreal

mesh = unreal.load_asset("/Game/Vehicles/SportsCar/SM_SportsCar")
sections = []
for index, slot in enumerate(mesh.static_materials):
    positions, triangles, normals, uvs, _ = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh, 0, index)
    sections.append({"slot": str(slot.material_slot_name), "material": slot.material_interface.get_path_name(),
                     "positions": [v.to_tuple() for v in positions], "normals": [v.to_tuple() for v in normals],
                     "triangles": list(triangles), "uv": [v.to_tuple() for v in uvs]})
out = Path(unreal.Paths.project_saved_dir()) / "CabinSource" / "exterior.json"
out.parent.mkdir(exist_ok=True)
out.write_text(json.dumps(sections, separators=(",", ":")), encoding="utf-8")
RV_RESULT = [(s["slot"], len(s["triangles"])//3) for s in sections]
