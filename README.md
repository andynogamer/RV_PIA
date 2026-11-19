# RUTA VIRTUAL

Simulador académico inmersivo de familiarización con la conducción, desarrollado en Unreal Engine 5.6. Proyecto técnico: `RV_PIA`. El objetivo es practicar controles y maniobras sencillas en un entorno controlado. No es un tratamiento clínico ni sustituye a un instructor.

## Clonar el proyecto desde GitHub

La versión que probamos en la ciudad con las Meta Quest 3 y el Logitech G920 está en la rama **`main`**. Estos pasos descargan el proyecto en tu PC con Windows y te permiten jugar desde Unreal.

### 1. Prepara tu PC

- Instala Git para Windows. Abre PowerShell y ejecuta `git lfs version`. Si el comando no se reconoce, instala Git LFS y vuelve a abrir PowerShell. El proyecto usa LFS para guardar los mapas, modelos y sonidos. [Git para Windows](https://git-scm.com/downloads/win) · [Git LFS](https://git-lfs.com/)
- En la biblioteca de Epic Games Launcher instala **Unreal Engine 5.6.1**, la versión que usó el equipo.
- Instala **Visual Studio 2022**. En Visual Studio Installer selecciona **Desarrollo de juegos con C++** e incluye las herramientas **MSVC v143** y el **SDK de Windows**. El proyecto compila un módulo de C++. [Configuración de Visual Studio para Unreal Engine 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine?application_version=5.6)

### 2. Descarga la rama main

Abre en el Explorador la carpeta donde quieres guardar el proyecto. Escribe `powershell` en la barra de direcciones y pulsa **Enter**. Ejecuta estos comandos en orden y espera a que termine cada uno.

```powershell
git lfs install
git clone --branch main https://github.com/andynogamer/RV_PIA.git
cd RV_PIA
git lfs pull
```

Al terminar tendrás una carpeta `RV_PIA` con el archivo `RV_PIA.uproject`. Para comprobar que llegó el mapa de la ciudad, ejecuta `git lfs ls-files --include="Content/RutaVirtual/Maps/L_CiudadUrbana.umap"`. Un asterisco `*` junto a la ruta indica que el mapa está descargado. Si aparece un guion `-`, vuelve a ejecutar `git lfs pull`.

### 3. Abre la ciudad por primera vez

1. Cierra Unreal Editor si está abierto. En la misma ventana de PowerShell, desde la carpeta `RV_PIA`, ejecuta `.\Scripts\Build-Project.ps1`. Si instalaste Unreal en otra unidad, indica su carpeta. Por ejemplo, `.\Scripts\Build-Project.ps1 -EngineRoot 'D:\Epic Games\UE_5.6'`.
2. Cuando termine la compilación, haz doble clic en `RV_PIA.uproject` y espera a que el editor termine de preparar los shaders. Se abrirá el mapa **L_CiudadUrbana**.
3. Para probar en el monitor, pulsa **Play ▶** y elige **Comenzar práctica**. Con las Quest 3, entra primero en **Quest Link** desde el visor y selecciona **VR Preview** en el menú de Play. Con el G920, **A verde** confirma y **X azul** libera el freno de mano. [Guía del visor y el volante](docs/HARDWARE.md)

**Si ya tienes una copia**, abre PowerShell en la carpeta `RV_PIA` y actualízala con estos comandos.

```powershell
git switch main
git pull --ff-only
git lfs pull
```

## Estado actual

Entrega indicada por el usuario: jueves 19 de noviembre de 2026, zona America/Mexico_City. Falta confirmar si corresponde al segundo avance o a la entrega final; se planifica conservadoramente para la final.

La práctica comienza en `/Game/RutaVirtual/Maps/L_CiudadUrbana`, en el monitor o en las Quest 3 mediante Link. El coche sale estacionado en una ciudad con cuatro carriles, 12 autos de tráfico, ocho cruces semaforizados y un recorrido guiado de unos 238 metros. Conserva el vehículo Chaos, cabina creada en Blender, límites de velocidad, pausa y reinicio. El circuito de 40 metros (`L_CiudadPractica`) y la sala original de RV se conservan.

Desde el menú inicial puedes conducir en la ciudad o elegir **Cambiar a carretera** para abrir Sierra del Pino. Para volver desde la carretera, abre la pausa, elige **Volver al inicio** y después **Cambiar a ciudad**.

Elige **Practicar estacionamiento** en el menú inicial para abrir el patio. Entra de frente en **A1** y luego de reversa en **B2**. En cada cajón, centra el coche, enderézalo y aplica el freno de mano con **X del G920** al detenerte y déjalo puesto hasta que cambie la indicación. En teclado, mantén **Espacio**. **Home** repite la práctica.

Para probar, pulsa Play y elige «Comenzar práctica». Comienzas con freno de mano. Pulsa **X en el G920** para liberarlo. En teclado, pulsa y suelta **Espacio**. Con G HUB activo, el G920 y su palanca se leen mediante Windows.Gaming.Input. Suelta el acelerador para comenzar. Si la palanca ya está en una marcha, puedes mantenerla y pisar el embrague a fondo con el coche detenido. J abre la calibración manual opcional. Pisa el embrague para elegir una de las cinco marchas de avance o la reversa en la posición 6. Para desarrollo con teclado, W acelera, S frena, A/D giran y R alterna avance/reversa casi detenido. P/M abre la pausa e Inicio/Home reinicia. Consulta la [calibración del G920](docs/HARDWARE.md) y la [guía de controles](docs/DESARROLLO.md).

El coche del jugador tiene retrovisor interior y dos laterales con imagen de la escena. Detenido y en cámara de cabina, **B** selecciona cada espejo, las **flechas** ajustan el ángulo y **Enter** guarda y termina. Con el perfil automático del G920 también puedes seleccionar con las dos levas juntas y ajustar con la cruceta. **G** oculta o recupera la guía, que se recoge al comenzar a circular. Si mueves la palanca antes de pisar el embrague, mantén la marcha y pisa el pedal para engranarla.

Hay sonido de motor/rodadura/ambiente, claxon y retroalimentación educativa con puntos, avisos y reconocimientos. H activa claxon y V silencia/restaura sonido. Los timbres son síntesis original de prototipo. La IA conserva rutas por splines, con seguimiento, reacción al verde y luces de freno; los giros entre rutas y maniobras complejas siguen pendientes. Ver [sonido y retroalimentación](docs/SONIDO_Y_RETROALIMENTACION.md).

Con el Quest 3 en Meta Horizon Link, abre **VR Preview** o el ejecutable Windows. Siéntate frente al volante físico y mira al frente. En el G920, usa **↑ / ↓ de la cruceta** para elegir y **A** para confirmar. Al elegir **Comenzar práctica**, el juego centra el asiento. **B** vuelve, **☰** pausa la práctica, **Y** vuelve a centrar la vista estando detenido y **X** pone o quita el freno de mano al parar. Para desarrollar con teclado siguen disponibles las flechas, Enter, P, C y Espacio. Consulta [hardware](docs/HARDWARE.md) y [estado de las pruebas](docs/ESTADO.md).

En RV, mira al cuadro detrás del volante para consultar los km/h y la marcha. La pantalla de la consola muestra una indicación cada vez. En el G920, **A verde** confirma, **B rojo** vuelve, **X azul** acciona el freno de mano e **Y amarillo** centra el asiento. Los choques significativos activan un pulso breve en el aro.

La ciudad es el mapa de inicio en Windows PCVR. Para sentir el retorno del volante al entrar en la cabina, deja la palanca en **N**, suelta el acelerador y pon el aro cerca del centro.

**Prueba rápida de la palanca.** Abre [Probar-Palanca.cmd](Scripts/Probar-Palanca.cmd) con doble clic. La ventana muestra cada marcha física y las posiciones vistas. Dentro de la cabina, el indicador **Palanca** muestra la posición física incluso cuando la caja sigue en N.

## Punto de entrada

- Agentes: [AGENTS.md](AGENTS.md), aplicable a todo el proyecto.
- Equipo: [alcances y límites de los agentes](docs/ALCANCE_AGENTES.md).
- Producto y rúbrica: [proyecto](docs/PROYECTO.md).
- Trabajo siguiente: [tareas ordenadas](docs/TAREAS.md).
- Diseño previsto: [arquitectura](docs/ARQUITECTURA.md) y [hardware](docs/HARDWARE.md).
- Comprobaciones: [validación](docs/VALIDACION.md) y [procedimiento Blueprint](docs/BLUEPRINTS.md).
- Historial: [decisiones](docs/DECISIONES.md), [procedencia de assets](docs/ASSETS.md) y [repositorio](docs/REPOSITORIO.md).

## Herramientas incluidas

`Scripts/Get-ProjectInventory.ps1` produce un inventario de archivos y configuración sin abrir Unreal. `Scripts/Audit-Unreal.ps1` prepara una auditoría de assets y actores dentro del motor; exige cerrar el editor después de guardar. Su ejecución interna aún debe verificarse en este equipo. Ambas escriben evidencia derivada bajo `Saved/AgentReports/`, fuera de Git.

La rama de referencia del equipo es `main`. Consulta el registro del proyecto para conocer la procedencia y las condiciones de uso de los recursos. [Procedencia de los recursos](docs/ASSETS.md)
