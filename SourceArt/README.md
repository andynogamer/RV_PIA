# Arte fuente de RUTA VIRTUAL

`Cockpit/Cabina.blend` y los FBX asociados son geometría original generada mediante `Scripts/Blender/build_cockpit.py` con Blender 3.6. No se descargaron de terceros. Incluyen tablero, marco de parabrisas, paneles interiores y volante; son una cabina de prototipo, pendiente de refinamiento visual.

Para regenerar, ejecutar Blender en modo background con ese script. Los FBX usan metros y se importan en Unreal mediante `Scripts/Editor/import_cockpit.py`. La adaptación de ejes y la posición del volante se establecen en ese importador; el FBX del volante tiene origen en su centro para permitir animarlo.

La geometría fuente está versionada mediante Git LFS. Los modelos de ciudad y del vehículo de Epic tienen procedencia separada en `docs/ASSETS.md`.
