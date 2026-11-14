# Estado comprobado del proyecto

Fecha de corte: 14 de noviembre de 2026, America/Mexico_City. Este archivo distingue lo observado de lo que falta probar. Actualizarlo después de cambios reales.

## Verificado

- `RV_PIA.uproject` está asociado a Unreal Engine 5.6. El registro `Saved/Logs/RV_PIA.log` identifica la sesión como 5.6.1-44394996.
- No existe carpeta `Source` ni módulos C++ declarados en el descriptor inicial; base predominantemente Blueprint.
- El descriptor habilita explícitamente OpenXR, OpenXREyeTracker y OpenXRHandTracking. Esto no demuestra conexión de visor ni necesidad de seguimiento ocular/manos.
- `Config/DefaultEngine.ini` fija `VRTemplateMap` como mapa inicial y de juego, y `VRGameMode` como modo global. `DefaultGame.ini` contiene `bStartInVR=True`.
- El único `.umap` encontrado en Content es `Content/VRTemplate/Maps/VRTemplateMap.umap`.
- En la inspección visual del editor abierto se observó ese mapa con la sala de pruebas de la plantilla y un contador de 48 actores. No se hicieron cambios ni se ejecutó una sesión nueva.
- `Content/Ciudad/Downtown_YongeDundas` contiene 1,417 archivos, unos 586.93 MiB: 1,194 en StaticMeshes, 99 en Materials y 124 en Textures. Estos son conteos de archivos, no de objetos funcionales certificados.
- Content completo contiene 1,615 archivos, unos 654.8 MiB. También hay recursos de plantilla, personajes, prototipado, espectador y armas; su presencia no los incluye en el producto.
- Hay piezas de ruedas `SM_*Wheel_*` dentro de la ciudad. No bastan para afirmar que existe un vehículo con rig, física o controles.
- Se verificó una fuente GLTF externa con una escena y 1,832 nodos. Puede servir para recuperar la distribución; todavía no se ha importado como escena. El ZIP externo no contiene mapas Unreal. Detalles y procedencia pendiente en `ASSETS.md`.
- `vswhere` detecta Visual Studio Community 18 con componente de herramientas C++ x86/x64. Esto no demuestra compatibilidad con UE 5.6 ni compilación del proyecto; deben verificarse versiones de compilador y SDK antes de crear el módulo.
- Git y Git LFS están disponibles. Se inicializó el repositorio local en `main`, sin remoto. Los assets Unreal se conservan mediante LFS; consultar `git log` para el commit de esta base.

## Indicios pendientes de resolver

- El registro previo muestra que OpenXR no pudo enumerar extensiones y sugiere ausencia de runtime. Es evidencia de esa sesión, no un diagnóstico permanente del equipo ni del futuro visor.
- La configuración de render contiene Forward Shading, opciones de RV, Lumen/ray tracing y claves repetidas de exposición/RHI. Se requiere revisar su combinación y medir; no se cambió nada en esta tarea.
- Existen opciones Android/Meta Quest heredadas. No prueban que el visor prestado sea Quest ni que haya un APK funcional.
- El registro contiene advertencias de importación que omiten reemplazar recursos existentes y recursos renombrados. No demuestran corrupción; revisar dependencias cuando se monte la escena.
- Se vieron errores de `LogAutomationTest: Condition failed` en el registro anterior. Falta identificar su contexto; no atribuirlos automáticamente a lógica de conducción.

## No demostrado

Ciudad ensamblada y guardada como nivel; colisiones y escala transitables; vehículo poseído y conducible; interacción con G920; force feedback ligado a la simulación; comunicación Arduino; visor real; sonido ambiente del recorrido; rendimiento de RV; empaquetado ejecutable; web/red social/documental y documentación final.

## Herramientas de revisión

- La extracción de texto de los dos DOCX fue correcta. El render de la rúbrica no pudo ejecutarse por falta de LibreOffice en las dependencias disponibles; no se auditó paginación.
- `Scripts/Get-ProjectInventory.ps1` permite repetir el inventario estático. La evidencia local está bajo `Saved/AgentReports/` y se excluye de Git.
- `Scripts/Audit-Unreal.ps1` y `Scripts/Editor/audit_project.py` preparan revisión interna de clases, actores y colisiones. El intento se detuvo en la protección de editor abierto; no se ejecutó Python dentro de Unreal. No presentar sus resultados como obtenidos.

## Validación de la base de gobernanza

- Inventario estático ejecutado: 1,615 archivos Content, 686,604,161 bytes y un mapa guardado.
- Enlaces locales de Markdown comprobados y sintaxis de scripts PowerShell/Python revisada.
- Se compararon hashes SHA-256 de los 1,622 archivos originales de Content, Config y descriptor: sin cambios. No se modificó la sala de plantilla ni se integró todavía la ciudad.
- Cachés y reportes locales quedan excluidos; se verificaron 1,641 archivos preparados y 1,615 paquetes bajo LFS. El mapa está representado por un puntero LFS válido en el índice.
- La revisión de espacios del snapshot completo señala líneas vacías finales preexistentes en tres archivos `Config` (DefaultEngine, DefaultGame y DefaultInput); se preservaron para no alterar la línea base. Los archivos nuevos se comprueban por separado.
- La integridad Git/LFS se comprueba al cerrar el commit; la prueba funcional del simulador permanece pendiente.

## Próximo resultado útil

Crear una escena de conducción pequeña, guardada y transitable con vehículo y teclado. Esa escena permitirá integrar G920 y visor sin esperar a que estén disponibles para empezar el desarrollo.
