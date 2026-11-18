# Registro de decisiones

Registrar decisiones que cambien el alcance, dependencias o forma de implementar. Una propuesta se acepta solo al contar con la autorización o evidencia pertinente. Las entradas siguientes reflejan la sesión del 14 de noviembre de 2026.

## D-001 — Entrenamiento introductorio

Estado: vigente. Fuente: propuesta y objetivo del usuario. RUTA VIRTUAL ofrece familiarización con conducción; no promete tratamiento, diagnóstico ni eficacia clínica. Consecuencia: prácticas guiadas y retroalimentación descriptiva, sin afirmaciones médicas en interfaz o publicidad.

## D-002 — G920 comercial y Arduino complementario

Estado: decisión del usuario. El profesor autorizó comprar Logitech G920. Se reemplaza la premisa de construir todo el volante descrita en el primer avance. Arduino puede añadir vibración. La selección de reversa pasa al Driving Force Shifter por D-013. Documentar la integración construida por el equipo.

## D-003 — Conservar Unreal 5.6

Estado: base vigente. Descriptor 5.6 y registro de sesión 5.6.1. No migrar automáticamente a otra versión instalada. Consecuencia: validar plugins, API y toolchain para esta versión.

## D-004 — Windows PCVR como punto de partida

Estado: propuesta provisional, sujeta al visor. Justificación: volante USB y Arduino en PC. No se ha definido ejecución autónoma Android. Consecuencia: identificar visor y conexión antes de cerrar plataforma, runtime y empaquetado.

Actualización del usuario del 14 de noviembre: probablemente se utilizará Meta Quest 3. Se mantiene como recomendación PCVR por Link/OpenXR, pendiente de confirmar préstamo y conexión. Esta información no demuestra imagen, tracking ni rendimiento en el dispositivo.

## D-005 — Lógica automatizable y composición Blueprint

Estado: criterio de trabajo. Maximizar implementación por agentes con C++/configuración/herramientas verificables, usar Blueprint para composición y parámetros, sin conversión masiva obligatoria. C++ se incorpora cuando el toolchain esté probado. Python es herramienta de editor, no gameplay del ejecutable.

## D-006 — Circuito corto antes de ciudad completa

Estado: primera etapa cumplida; ampliada por D-010 por solicitud posterior del usuario. Se resolvió un circuito corto y se conservará como entrenamiento y regresión. La comprobación en Unreal mostró que las transformaciones de ciudad ya estaban incorporadas en las mallas importadas, por lo que no fue necesario reimportar su fuente GLTF.

## D-007 — Git local y LFS

Estado: preparación autorizada por el usuario. Versionar proyecto y assets existentes; excluir cachés y evidencia privada. No crear repositorio público ni subir contenido sin definir destino y alcance. No aplicar licencia de software libre a assets ajenos de procedencia pendiente.

## D-008 — Fecha y revisión

Estado: fecha objetivo, tipo de revisión pendiente. El usuario dijo «jueves» el sábado 14 de noviembre de 2026; se interpreta como 19 de noviembre, America/Mexico_City. Hasta aclararlo, el plan contempla requisitos de la final. No presentar esta interpretación como fecha escrita en la rúbrica, cuyas revisiones aparecen por anunciar.

## D-009 — Primer prototipo de escritorio

Estado: implementado el 14 de noviembre de 2026, autorizado por el usuario al iniciar desarrollo. Reutilizar la plantilla Chaos instalada de UE 5.6 y complementar con C++ y arte de cabina generado en Blender. Conservar ciudad y sala originales; ensamblar mallas con transformaciones ya incorporadas en un mapa propio. Acotar un corredor de 40 m mientras se desarrollan colisiones del resto del entorno.

Las pruebas detectaron la reversa automática del freno de plantilla; se desactivó y se agregó selector R con condición de parada y acelerador liberado. El reinicio usa teleport físico y reinicializa el estado asíncrono de Chaos. Se autoriza reiniciar el editor dentro del desarrollo tras comprobar/guardar paquetes, sin descartar trabajo.

El usuario permite recursos externos para uso privado académico. Para esta implementación se usaron recursos oficiales ya instalados y arte original; no se descargaron recursos nuevos ni se hizo push. El remoto `origin` fue configurado por el usuario entre sesiones.

## D-010 — Playground urbano y tráfico autónomo ligero

Estado: primera implementación verificada en editor el 14 de noviembre de 2026. Fuente: petición expresa del usuario de ampliar el playground, comenzar estacionado y sumar IA, señales, semáforos y pitidos contextuales al integrar sonido. Amplía la priorización D-006 sin eliminar los requisitos de RV, G920 y háptico.

Se mantiene separado `L_CiudadUrbana`, con límites de 770 × 760 m, ruta guiada de unos 245 m y tráfico cinemático sobre cuatro splines. Se clonaron 227 mallas para colisión compleja sin alterar los originales y se retiraron del mapa urbano las piezas de autos decorativos que ocupaban los carriles. Los 12 agentes reutilizan carrocerías oficiales instaladas. Dos cruces usan verde/ámbar y separación en rojo para movimientos incompatibles.

La decisión de movimiento cinemático permite tráfico ligero controlable antes de simular cada vehículo con Chaos. No incluye aún giros entre rutas, cesión completa, cambios de carril o conductores humanos realistas. Los claxones se expusieron como eventos con espera y cooldown; el audio se conectó en D-012. La recirculación se protege por ocupación de carril y distancia al jugador, sin afirmar invisibilidad de la reaparición.

La validación detectó bloqueo mutuo entre el inicio de una ruta y el final del carril contrario; se corrigió midiendo ocupación dentro del carril. La clonación segura del mapa usa `new_level_from_template`; la duplicación directa de World provocó un fallo del editor y no se conserva como procedimiento. Ver ESTADO.md para evidencia y límites.

La inspección visual detectó lámparas decorativas fijas que contradecían el controlador. Se enlazaron 36 lentes originales (24 sur, 12 norte), se ocultaron los postes auxiliares y se ubicó el controlador norte en el cruce real del modelo. Las pruebas comprueban ahora correspondencia entre fase lógica y color visible. Los demás semáforos del escenario se mantienen como decoración y requieren integración si se amplían las rutas.

## D-011 — Inicio y pausa con interfaz de escritorio

Estado: implementación del 14 de noviembre de 2026, solicitada por el usuario para entrar con el ratón dentro del juego y disponer de menú inicial. Se agrega UMG nativo, sin modificar los mapas/Blueprints existentes. El controlador separa Inicio, Conducción y Pausa; los menús detienen el mundo y la conducción captura la entrada de forma explícita. El foco y posición del cursor se establecen después del layout inicial del viewport para evitar las dimensiones provisionales de PIE.

La configuración predeterminada de PIE activa GameGetsMouseControl y se actualizó también la preferencia local de esta computadora, con copia previa fuera de Git. La interfaz sigue siendo plana; no se presenta como menú de RV validado. Las APIs se contrastaron con los headers instalados de UE 5.6, la compilación y las referencias oficiales de [FInputModeGameOnly](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FInputModeGameOnly?application_version=5.6) y [UUserWidget](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/Blueprint/UUserWidget?application_version=5.6). Ver ESTADO.md para resultados.

## D-012 — Audio, tráfico y consecuencias educativas

Solicitud del usuario del 14 de noviembre de 2026: introducir sonido y refinar tráfico, con respuesta negativa ante infracciones/contactos y vibración posterior. Se implementan puntos de seguridad limitados a 0–100, avisos descriptivos, recuperación por acciones cuidadosas y resumen por intento. No se crea ranking, presión por terminar rápido ni afirmación de eficacia terapéutica. Los umbrales son reglas explícitas del prototipo, sin atribución de culpa legal.

Los timbres iniciales son síntesis matemática original, con fuentes reproducibles y seis assets propios. El claxon y los avisos tienen cooldown, mute V y silencio/cancelación al pausar. Se verifica técnicamente el bus de mezcla de Unreal mediante grabación; la escucha con auriculares reales queda separada de esa comprobación.

La retroalimentación expone solicitudes lógicas de pulso de 0.15 s y cancelación al pausar/reiniciar/terminar. No se envía fuerza ni vibración al hardware. Se refinan seguimiento, reacción y espacio de salida del tráfico, conservando splines y declarando pendientes los giros conectados. Ver SONIDO_Y_RETROALIMENTACION.md y ESTADO.md.

## D-013 — Palanca en H y caja manual de práctica

El 17 de noviembre de 2026 el usuario confirmó el G920 conectado y pidió usar el Driving Force Shifter en lugar de un botón de cambio. Se adoptan seis marchas adelante, neutral y reversa. El modo teclado conserva D/R para desarrollo.

Se lee DirectInput en Windows. El estado completo permite distinguir una palanca sostenida, neutral y una lectura fallida. J captura ejes, extremos y posiciones del equipo, sin un mapa de índices predeterminado. El perfil queda en la configuración local. Se verificó el contrato de lectura con [Microsoft](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee417897%28v%3Dvs.85%29) y las llamadas de transmisión contra el código de Chaos Vehicles instalado en UE 5.6.1. [Posiciones de la palanca según Logitech](https://www.logitechg.com/en-us/shop/p/driving-force-shifter)

Se selecciona con el embrague al 80 % o más. En ese punto la transmisión física queda en neutral y la marcha elegida se conserva para el acople. La zona intermedia reduce el acelerador efectivo como aproximación al punto de acople. Chaos conserva las relaciones existentes y extiende hasta seis si el vehículo trae menos. La práctica conserva los límites de avance y reversa. Calado, sincronizadores y fuerza física siguen fuera de este cambio.

## D-014 — Lectura automática del G920 en Windows

El 17 de noviembre se identificaron la base Logitech 046D:C262 y sus controles con nombre después de instalar G HUB. El modo predeterminado lee dirección, tres pedales y marcha de GameInput. Se conserva el perfil calibrado de DirectInput como alternativa cuando el equipo lo necesita. La interfaz de cabina ilustra el mando correspondiente y muestra el movimiento de la palanca en H. [Estado del volante en GameInput](https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/input/gameinput/structs/gameinputracingwheelstate)

## D-015 — Validar la posición física antes de mostrar la marcha

El 17 de noviembre, con el G920 conectado, el diagnóstico Windows.Gaming.Input capturó N y 1–6. Quinta produjo el valor 5 y el botón 17. La sonda GameInput v0 en segundo plano se mantuvo en cero. Se adoptó la lectura RacingWheel con marca temporal para la ruta automática de UE. El HUD distingue la posición de la palanca de la marcha aceptada por las reglas de embrague y rearme. El perfil manual de DirectInput sigue disponible con J. [RacingWheelReading](https://learn.microsoft.com/en-us/uwp/api/windows.gaming.input.racingwheelreading)

## D-016 — Puesto del conductor y cambios con embrague

El 17 de noviembre el usuario pidió corregir el sentido del volante y conducir con una cabina de mayor detalle y espejos ajustables. El sentido visual se mide desde el asiento por el desplazamiento del punto superior del aro. La selección de marchas reconoce el embrague durante la entrada en la ranura y lo vuelve a comprobar mientras se mantiene la posición. La inversión de sentido en movimiento conserva el bloqueo.

El coche del jugador incorpora tres capturas de escena sobre los cristales. Cada captura refleja la posición del observador en el plano del espejo y ajusta la proyección a los bordes del cristal. Es una aproximación monoscópica compartida por ambos ojos. Se alternan las capturas visibles con un máximo de una por fotograma y 45 por segundo en total. No hay capturas en los vehículos de tráfico. La calidad y el presupuesto final en PCVR requieren medición con el visor objetivo. [SceneCaptureComponent2D en UE 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/SceneCaptureComponent2D?application_version=5.6)

## D-017 — Velocidades señalizadas, ciudad y carretera

El 17 de noviembre se adoptó México como referencia visual para la práctica. La avenida usa 50 km/h y Sierra del Pino representa una carretera estatal fuera de zona urbana con 80 km/h. Son los valores elegidos para estos dos escenarios ficticios. El conductor regula la velocidad y recibe avisos de exceso sostenido. El circuito de conos conserva el limitador de 30 km/h. Referencias del diseño y de las categorías de vía. [NOM-034-SCT2/SEDATU-2022](https://sidof.segob.gob.mx/notas/docFuente/5702233) y [Ley General de Movilidad y Seguridad Vial, artículo 49](https://www.diputados.gob.mx/LeyesBiblio/pdf/LGMSV.pdf).

Las señales se modelaron en Blender. Las placas de velocidad de la ciudad sustituyen las caras importadas en su ubicación y orientación originales. Los semáforos obtienen los sentidos regulados de las normales de sus lentes. Un acceso sin cabezal visible queda fuera de la evaluación por semáforo.

## D-018 — Reversa en la posición física 6

Por petición expresa del usuario, el cambio manual tiene cinco marchas de avance. Tanto la posición inferior derecha sin hundir como el enclavamiento R de fábrica solicitan reversa. La calibración conserva las dos entradas físicas y los perfiles existentes. El bloqueo de inversión y el embrague se evalúan después de traducir la posición.

## D-019 — Práctica de estacionamiento

El 18 de noviembre de 2026 se añadió un patio independiente accesible desde ciudad y carretera. Las dos maniobras comparten el vehículo y los controles del simulador. Para avanzar hay que cruzar la entrada del cajón en el sentido solicitado, colocar la carrocería completa dentro de las marcas con hasta 10 grados de desviación y mantener el freno de mano durante 1,5 segundos por debajo de 0,5 km/h. Reiniciar devuelve al conductor al acceso.

## Registro de decisiones futuras

Identificador y título; fecha; estado; problema; evidencia/fuente; decisión; consecuencias; validación; alternativa relevante si explica el costo; condición de revisión. Evitar abrir un registro nuevo por cambios triviales.
