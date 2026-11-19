# RUTA VIRTUAL

Simulador académico inmersivo de familiarización con la conducción, desarrollado en Unreal Engine 5.6. Proyecto técnico: `RV_PIA`. El objetivo es practicar controles y maniobras sencillas en un entorno controlado. No es un tratamiento clínico ni sustituye a un instructor.

Entrega indicada por el usuario: jueves 19 de noviembre de 2026, zona America/Mexico_City. Falta confirmar si corresponde al segundo avance o a la entrega final; se planifica conservadoramente para la final.

## Estado actual

Existe un prototipo urbano de escritorio en `/Game/RutaVirtual/Maps/L_CiudadUrbana`: salida estacionada, ciudad ampliada, cuatro carriles con 12 autos autónomos de prototipo, ocho cruces semaforizados y recorrido guiado de unos 238 metros. Conserva el vehículo Chaos, cabina creada en Blender, límites de velocidad, pausa y reinicio. El circuito de 40 metros (`L_CiudadPractica`) y la sala original de RV se conservan.

Desde el menú inicial puedes conducir en la ciudad o elegir **Cambiar a carretera** para abrir Sierra del Pino. Para volver desde la carretera, abre la pausa, elige **Volver al inicio** y después **Cambiar a ciudad**.

Elige **Practicar estacionamiento** en el menú inicial para abrir el patio. Entra de frente en **A1** y luego de reversa en **B2**. En cada cajón, centra el coche, enderézalo y aplica el freno de mano con **X del G920** al detenerte y déjalo puesto hasta que cambie la indicación. En teclado, mantén **Espacio**. **Home** repite la práctica.

Para probar, pulsa Play y elige «Comenzar práctica». Comienzas con freno de mano. Pulsa **X en el G920** para liberarlo. En teclado, pulsa y suelta **Espacio**. Con G HUB activo, el G920 y su palanca se leen mediante Windows.Gaming.Input. Suelta el acelerador para comenzar. Si la palanca ya está en una marcha, puedes mantenerla y pisar el embrague a fondo con el coche detenido. J abre la calibración manual opcional. Pisa el embrague para elegir una de las cinco marchas de avance o la reversa en la posición 6. Para desarrollo con teclado, W acelera, S frena, A/D giran y R alterna avance/reversa casi detenido. P/M abre la pausa e Inicio/Home reinicia. Consulta la [calibración del G920](docs/HARDWARE.md) y la [guía de controles](docs/DESARROLLO.md).

El coche del jugador tiene retrovisor interior y dos laterales con imagen de la escena. Detenido y en cámara de cabina, **B** selecciona cada espejo, las **flechas** ajustan el ángulo y **Enter** guarda y termina. Con el perfil automático del G920 también puedes seleccionar con las dos levas juntas y ajustar con la cruceta. **G** oculta o recupera la guía, que se recoge al comenzar a circular. Si mueves la palanca antes de pisar el embrague, mantén la marcha y pisa el pedal para engranarla.

Hay sonido de motor/rodadura/ambiente, claxon y retroalimentación educativa con puntos, avisos y reconocimientos. H activa claxon y V silencia/restaura sonido. Los timbres son síntesis original de prototipo. La IA conserva rutas por splines, con seguimiento, reacción al verde y luces de freno; los giros entre rutas y maniobras complejas siguen pendientes. Ver [sonido y retroalimentación](docs/SONIDO_Y_RETROALIMENTACION.md).

Con el Quest 3 en Meta Horizon Link, abre **VR Preview** o el ejecutable Windows. Siéntate frente al volante físico y mira al frente. En el G920, usa **↑ / ↓ de la cruceta** para elegir y **A** para confirmar. Al elegir **Comenzar práctica**, el juego centra el asiento. **B** vuelve, **☰** pausa la práctica, **Y** vuelve a centrar la vista estando detenido y **X** pone o quita el freno de mano al parar. Para desarrollar con teclado siguen disponibles las flechas, Enter, P, C y Espacio. Consulta [hardware](docs/HARDWARE.md) y [estado de las pruebas](docs/ESTADO.md).

En RV, mira al cuadro detrás del volante para consultar los km/h y la marcha. La pantalla de la consola muestra una indicación cada vez. En el G920, **A verde** confirma, **B rojo** vuelve, **X azul** acciona el freno de mano e **Y amarillo** centra el asiento. Los choques significativos activan un pulso breve en el aro.

**Prueba rápida de la palanca.** Abre [Probar-Palanca.cmd](Scripts/Probar-Palanca.cmd) con doble clic. La ventana muestra cada marcha física y las posiciones vistas. Dentro de la cabina, el indicador **Palanca** muestra la posición física incluso cuando la caja sigue en N.

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

El repositorio utiliza Git LFS. Ya existe un remoto `origin` configurado por el usuario; el trabajo de sonido y retroalimentación se prepara en la rama local `feature/audio-retroalimentacion` y no se sube automáticamente. No se ha otorgado una licencia general de redistribución de los recursos.
