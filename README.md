# RUTA VIRTUAL

Simulador académico inmersivo de familiarización con la conducción, desarrollado en Unreal Engine 5.6. Proyecto técnico: `RV_PIA`. El objetivo es practicar controles y maniobras sencillas en un entorno controlado. No es un tratamiento clínico ni sustituye a un instructor.

Entrega indicada por el usuario: jueves 19 de noviembre de 2026, zona America/Mexico_City. Falta confirmar si corresponde al segundo avance o a la entrega final; se planifica conservadoramente para la final.

## Estado inicial

Existe la plantilla de RV y una ciudad importada. El único mapa guardado encontrado es `Content/VRTemplate/Maps/VRTemplateMap.umap`; al inspeccionarlo muestra la sala de plantilla. La ciudad todavía requiere montaje y validación como escenario conducible. No se ha demostrado un vehículo controlable, force feedback, Arduino ni RV con visor real.

El G920 está pendiente de llegar y el visor será prestado; su modelo no se conoce. El detalle comprobado y sus límites están en [estado del proyecto](docs/ESTADO.md).

## Punto de entrada

- Agentes: [AGENTS.md](AGENTS.md), aplicable a todo el proyecto.
- Equipo: [alcances y límites de los agentes](docs/ALCANCE_AGENTES.md).
- Producto y rúbrica: [proyecto](docs/PROYECTO.md).
- Trabajo siguiente: [tareas ordenadas](docs/TAREAS.md).
- Diseño previsto: [arquitectura](docs/ARQUITECTURA.md) y [hardware](docs/HARDWARE.md).
- Comprobaciones: [validación](docs/VALIDACION.md) y [procedimiento Blueprint](docs/BLUEPRINTS.md).
- Historial: [decisiones](docs/DECISIONES.md), [procedencia de assets](docs/ASSETS.md) y [repositorio](docs/REPOSITORIO.md).

## Abrir el proyecto

1. Instala Unreal Engine 5.6; la sesión observada usa 5.6.1. No aceptes una conversión a 5.8 por defecto.
2. Si obtienes el proyecto mediante Git, instala Git LFS y ejecuta `git lfs pull` después de clonar.
3. Abre `RV_PIA.uproject`. La carga inicial puede generar shaders y cachés.
4. Revisa el mapa activo y los errores del registro. En esta base todavía abre la plantilla de RV.
5. Antes de trabajar con el visor identifica marca, modelo, modo de conexión y runtime; no selecciones un perfil de Quest por suposición.

## Herramientas incluidas

`Scripts/Get-ProjectInventory.ps1` produce un inventario de archivos y configuración sin abrir Unreal. `Scripts/Audit-Unreal.ps1` prepara una auditoría de assets y actores dentro del motor; exige cerrar el editor después de guardar. Su ejecución interna aún debe verificarse en este equipo. Ambas escriben evidencia derivada bajo `Saved/AgentReports/`, fuera de Git.

El repositorio es local y utiliza Git LFS para los assets binarios. No tiene respaldo remoto hasta conectarlo y subirlo a un servicio. No se ha publicado el proyecto ni otorgado una licencia de redistribución de sus recursos.
