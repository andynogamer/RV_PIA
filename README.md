# RUTA VIRTUAL

Simulador académico inmersivo de familiarización con la conducción, desarrollado en Unreal Engine 5.6. Proyecto técnico: `RV_PIA`. El objetivo es practicar controles y maniobras sencillas en un entorno controlado. No es un tratamiento clínico ni sustituye a un instructor.

Entrega indicada por el usuario: jueves 19 de noviembre de 2026, zona America/Mexico_City. Falta confirmar si corresponde al segundo avance o a la entrega final; se planifica conservadoramente para la final.

## Estado actual

Existe un prototipo urbano de escritorio en `/Game/RutaVirtual/Maps/L_CiudadUrbana`: salida estacionada, ciudad ampliada, cuatro carriles con 12 autos autónomos de prototipo, dos cruces semaforizados y recorrido guiado de unos 250 metros. Conserva el vehículo Chaos, cabina creada en Blender, límites de velocidad, pausa y reinicio. El circuito de 40 metros (`L_CiudadPractica`) y la sala original de RV se conservan.

Para probar: pulsa Play y haz clic en la vista. Comienzas con freno de mano: pulsa y suelta Espacio para salir. W acelera, S frena, A/D giran, R cambia marcha casi detenido, P pausa e Inicio/Home reinicia. Consulta la [guía de desarrollo y controles](docs/DESARROLLO.md) y el [alcance urbano](docs/ENTORNO_URBANO.md).

El G920 está pendiente de llegar y el visor será prestado; su modelo no se conoce. No se han validado periféricos, hápticos ni ejecutable empaquetado. La IA sigue rutas por splines: aún faltan giros entre rutas, maniobras complejas y sonido. Los eventos de claxon están preparados, sin audio conectado. El detalle está en [estado del proyecto](docs/ESTADO.md).

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
2. Si obtienes el proyecto mediante Git, instala Git LFS y ejecuta `git lfs pull` después de clonar. El proyecto ahora incluye C++: necesita compilador y Windows SDK compatibles; ver la guía de desarrollo.
3. Con el editor cerrado, `Scripts/Build-Project.ps1` compila el módulo Editor. Abre `RV_PIA.uproject`; la carga inicial puede generar shaders y cachés.
4. Revisa el mapa activo y los errores del registro. La ciudad urbana debe ser la escena de inicio; el circuito corto y la sala original siguen disponibles por separado.
5. Antes de trabajar con el visor identifica marca, modelo, modo de conexión y runtime; no selecciones un perfil de Quest por suposición.

## Herramientas incluidas

`Scripts/Get-ProjectInventory.ps1` produce un inventario de archivos y configuración sin abrir Unreal. `Scripts/Audit-Unreal.ps1` prepara una auditoría de assets y actores dentro del motor; exige cerrar el editor después de guardar. Su ejecución interna aún debe verificarse en este equipo. Ambas escriben evidencia derivada bajo `Saved/AgentReports/`, fuera de Git.

El repositorio utiliza Git LFS. Ya existe un remoto `origin` configurado por el usuario; la ampliación se prepara en la rama local `feature/ciudad-trafico` y no se sube automáticamente. No se ha otorgado una licencia general de redistribución de los recursos.
