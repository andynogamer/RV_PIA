# Desarrollo y pruebas de conducción

## Ciudad urbana (mapa inicial)

Play abre el menú de RUTA VIRTUAL con cursor visible y el mundo detenido. Puedes elegir «Comenzar práctica», «Cambiar a carretera», «Ver controles» o «Salir del simulador». El botón principal recibe foco para activarlo también con Enter; Tab permite recorrer opciones. Al comenzar, el cursor se oculta y el juego captura la entrada sin un clic adicional en la carretera.

Durante la conducción, P o M abre la pausa y muestra el cursor. Ofrece continuar, reiniciar, volver al inicio y controles; al reanudar, libera un acelerador que siguiera pulsado. Volver al inicio reinicia vehículo/progreso y deja la simulación en pausa hasta comenzar otra vez. Salir termina PIE durante las pruebas, conservando abierto Unreal; en el juego solicita cerrar la aplicación. Escape mantiene el atajo habitual de terminar PIE en editor; utiliza P/M para probar el menú de pausa. En ejecución independiente Escape también solicita pausa.

`Config/DefaultEditorPerProjectUserSettings.ini` activa `GameGetsMouseControl` como valor inicial para PIE. Las preferencias locales guardadas pueden prevalecer; en esta computadora se actualizó también ese valor local, con copia previa en Saved/AgentReports. Shift+F1 sigue liberando el cursor para usar el editor. Menú y HUD son de escritorio: aún falta interacción espacial y validación con visor.

Nivel: `/Game/RutaVirtual/Maps/L_CiudadUrbana`. Comienzas estacionado con freno de mano aplicado. Pulsa y suelta Espacio, observa el carril e incorpórate. El HUD muestra distancia al siguiente objetivo y orientación aproximada; recorre cuatro objetivos hacia el norte y detente en la bahía verde final. Home reinicia al estacionamiento y limpia los avisos de rojo. Después de completar se permite explorar dentro de los límites.

Hay 12 autos sobre cuatro rutas y ocho cruces con semáforos. Los dos carriles norte-sur se prolongan por el Distrito Norte. El recorrido guiado hasta la bahía final mide unos 238 m. Se puede seguir explorando al terminar.

Respeta la señalización y deja distancia: los autos frenan y esperan, pero su movimiento es cinemático. No hacen todavía giros entre rutas, adelantamientos ni maniobras de conductores humanos. Las señales físicas ALTO y límite sirven de referencia; no existe aún un evaluador completo de normas. El contador de rojo es una ayuda de prototipo.

H activa claxon manual con cooldown de 1 s. Un auto bloqueado por el jugador más de 5 s solicita un claxon contextual, con cooldown de 10 s. No solicita claxon por esperar correctamente en rojo o por no tener espacio para salir del cruce. V silencia/restaura sonido. Motor, rodadura y ambiente son timbres originales de prototipo, con variación de régimen/velocidad y atenuación en el tráfico.

El HUD muestra seguridad (0–100), errores y reconocimientos. Rojo, ALTO de salida, contacto significativo, exceso de velocidad y otras condiciones sostenidas producen mensaje correctivo y descuento; una conducción cuidadosa puede recuperar puntos. La pausa muestra el balance del intento. No es una evaluación clínica ni un reglamento legal completo. Detalles: SONIDO_Y_RETROALIMENTACION.md. No existe vibración física todavía.

## Circuito corto

Nivel: `/Game/RutaVirtual/Maps/L_CiudadPractica`. Vehículo compartido: `/Game/RutaVirtual/Vehicles/BP_VehiculoPractica`. El trabajo nuevo está separado de la sala original de RV.

## Probar en Unreal

1. Abrir el proyecto con UE 5.6 y cargar `L_CiudadPractica` si no aparece automáticamente.
2. Pulsar Play y elegir «Comenzar práctica»; la captura de entrada es automática.
3. Avanzar entre los conos y detenerse en la zona verde al final. Es una práctica corta, sin competición ni límite de tiempo.
4. Salir de Play con Escape. En editor, Shift+F1 libera el ratón.

Controles de escritorio:

- W o flecha arriba: acelerar en la marcha seleccionada.
- S o flecha abajo: frenar. El freno no selecciona reversa.
- A/D o flechas izquierda/derecha: dirección.
- R: alternar avance y reversa con teclado, solo casi detenido y con acelerador liberado.
- B elige el retrovisor izquierdo, central o derecho con el coche detenido en vista interior. Ajusta con flechas. Enter termina o pulsa B después del derecho. El perfil automático también admite ambas levas juntas para seleccionar y la cruceta para ajustar. La orientación se guarda en la configuración local. En escritorio la cámara gira desde el asiento hacia el espejo; en RV se mantiene el control de la cabeza.
- G oculta o muestra las instrucciones de conducción. La tarjeta inicial se recoge al superar 4 km/h y la guía manual se puede recuperar en marcha.
- J: calibrar G920 y Driving Force Shifter con el vehículo detenido. Enter registra cada posición indicada en el HUD. K cambia entre teclado y G920 detenido.
- El modo G920 se activa automáticamente con G HUB y la práctica en primer plano. La palanca selecciona 1–5, N o R. La posición 6 acciona la reversa sin hundir la palanca. Pisa el embrague a fondo para cambiar. Si la palanca llegó primero, déjala en la marcha y pisa el pedal. Para un cambio de sentido rechazado en movimiento, detente y vuelve a N. Consulta [los pasos de calibración](HARDWARE.md).
- Espacio: freno de mano.
- P/M: abrir pausa o continuar. En el menú puedes reiniciar, volver al inicio y consultar controles.
- Home/Inicio: reiniciar la práctica desde el punto de partida.
- Tab: alternar cámaras. L: luces del vehículo de plantilla.
- H: claxon manual. V: silenciar/restaurar el sonido, también desde los menús.
- Retroceso conserva la función de enderezar el vehículo de la plantilla; Home es el reinicio completo.

El circuito de conos conserva el corte de 30 km/h y la protección de reversa es de 5 km/h. En la ciudad y la carretera el límite señalizado es de 50 y 80 km/h, respectivamente. Superarlo de forma sostenida genera un aviso. En las vías abiertas se midieron picos de 59,2 km/h en ciudad y 87,5 km/h en carretera.

## Archivos de lógica

- `RVDrivingAssistComponent`: condiciones de cambio de marcha, límite de velocidad, pausa, reinicio, límites por mapa, objetivos de la práctica y volante visual. Detecta el escenario urbano para activar freno inicial y objetivos de mayor alcance.
- `RVPracticePlayerController`: estados Inicio/Conducción/Pausa, captura y liberación del cursor, eventos de teclado R, P/M, Home, H y V. El Blueprint derivado conserva los mapeos Enhanced Input de la plantilla.
- `RVSessionMenuWidget`: menú UMG nativo con botones, foco de teclado, controles y escalado a la ventana. No necesita un Blueprint nuevo ni lógica Python para ejecutarse.
- `RVPracticeHUD`: instrucciones y estado para la prueba de escritorio. No es una interfaz de RV validada; adaptar a widgets espaciales al probar el visor.
- `RVTrafficSystem`: rutas, vehículos autónomos cinemáticos, semáforos y escenario urbano; ver ARQUITECTURA.md.
- `RVDrivingFeedbackComponent`: reglas de entrenamiento, contacto, puntuación, reconocimientos y eventos para el futuro háptico. `ARVStopRule` marca el ALTO de salida.
- `RVVehicleAudioComponent`: mezcla de motor/rodadura/ambiente y efectos de claxon, contacto y reconocimiento. Sonidos propios importados por `import_driving_audio.py` desde `SourceAudio/`.

La física y las acciones de conducción básicas provienen de la plantilla oficial Chaos de UE 5.6. La lógica de sesión nueva se ejecuta en C++; Python se utiliza para construir/verificar contenido dentro del editor.

## Herramientas para agentes

`Scripts/Build-Project.ps1` compila el objetivo Editor con el editor cerrado; `-Target Game` compila el objetivo de juego. El desarrollo necesita compilador C++ y Windows SDK. La sesión actual usó MSVC 14.51 y SDK 10.0.22621; UBT advierte que prefiere MSVC 14.38. No se modificó el motor para permitir esta compilación.

`Scripts/Open-Editor.ps1` abre el mapa inicial configurado. `-Map /Game/RutaVirtual/Maps/L_CiudadPractica` permite abrir el circuito corto. Su opción `-EnableAgentBridge` inicia una cola temporal de tareas locales, sin escucha de red. `Scripts/Invoke-Unreal.ps1` ejecuta scripts de editor dentro de esa sesión; `-Stop` desactiva la cola. No ejecuta automáticamente tareas pendientes de una sesión anterior. Si se inició el editor con la opción de script, la notificación de ejecución puede permanecer hasta cerrar esa sesión; abrir normalmente para trabajar sin esa notificación.

Orden de construcción desde la base, sin sobrescribir un mapa existente: instalar recursos con `Install-VehicleTemplate.ps1`, ejecutar `build_city.py`, `setup_vehicle.py`, `import_cockpit.py`, `configure_runtime.py` y `build_training_route.py` dentro del editor. Los plugins y el módulo deben estar habilitados/compilados primero. Las fuentes importadas y los assets resultantes ya están incluidos en el repositorio.

Para la ampliación: compilar las clases de tráfico, ejecutar `prepare_traffic_material.py`, `build_urban_scene.py` una sola vez, `polish_urban_scene.py` y `bind_city_signals.py`. Estos assets ya están guardados: no reconstruirlos para probar. El último script enlaza las lentes de los semáforos originales con el ciclo funcional; gobierna las 150 lentes de los ocho cruces. La ampliación y sus recursos se regeneran con el procedimiento de [entorno urbano](ENTORNO_URBANO.md). El mapa se crea con `LevelEditorSubsystem.new_level_from_template`; no duplicar un `World` mediante `EditorAssetLibrary.duplicate_asset` para cargarlo después: esa vía produjo un error de referencias de mundo en UE 5.6 durante el desarrollo.

`test_driving.py` requiere PIE activo, Unreal en primer plano y un único jugador local; inyecta acciones Enhanced Input durante una prueba controlada y deja un JSON en `Saved/AgentReports/driving_test.json`. La reducción de CPU del editor en segundo plano puede invalidar los tiempos de la prueba física. No ejecutarlo mientras una persona conduzca o haya un dispositivo físico de fuerza activo.

Las pruebas de conducción/tráfico comienzan explícitamente la sesión mediante el controlador antes de inyectar acciones. `test_session_menu.py` se ejecuta partiendo del menú inicial: comprueba transiciones, bloqueo de aceleración, pausa del tráfico, cursor y repetición de sesión. Complementar con clics reales y navegación por teclado para verificar foco/captura y legibilidad; su JSON por sí solo no prueba posición física del cursor.

Pruebas urbanas, ejecutadas de una en una con PIE urbano activo y Unreal en primer plano:

- `test_urban_traffic.py`: estacionamiento, fases, parada/reanudación ante señal/jugador, evento de claxon y pausa de tráfico/reloj. Cambia posiciones durante la prueba.
- `test_traffic_recycling.py`: espera ante inicio ocupado, recirculación sin bloqueo del carril contrario y protección cuando el jugador está cerca.
- `test_urban_route.py`: recorrido físico guiado hasta la detención final; oculta y desactiva temporalmente IA y fuerza semáforos verdes para aislar la transitabilidad del recorrido. Restaura al finalizar.
- `audit_urban_roads.py`: 216 muestras de suelo y barridos con volumen de automóvil a lo largo de cuatro rutas. También funciona en editor sin PIE.

Para audio y consecuencias: `test_traffic_refinements.py`, `test_driving_feedback.py` y `test_audio_mix.py`, secuencialmente. Este último exporta WAV del bus maestro; analizar después con `Scripts/Audio/check_recorded_mix.py`. Reglas, construcción de sonidos y límites en [sonido y retroalimentación](SONIDO_Y_RETROALIMENTACION.md).

`play_current_level.py` inicia PIE conservando PlayerStart. Los scripts de prueba antiguos preparados en `Saved/AgentBridge/tasks` son temporales, pueden referirse al punto de partida anterior y no deben usarse como herramientas permanentes.

## Límites de esta versión

La zona practicable depende del mapa: corredor acotado en el circuito corto, rectángulo amplio con recuperación fuera de límites en el urbano. El urbano añade colisión compleja en copias de 227 mallas de edificios; accesorios, otras calles y bordes no tienen validación exhaustiva. Los originales permanecen intactos. No presentar la ciudad entera como circuito terminado.

El interior principal incluye asientos, consola y palanca móvil, cuadro de marcha y velocidad, volante y tres retrovisores regulables. Faltan validación del puesto sentado e interfaz espacial, pulir timbres de sonido, prueba de conducción con los mandos físicos del G920, visor real, hápticos físicos, medición de rendimiento en RV y prueba de ejecutable empaquetado. La prueba de escritorio no cubre esos requisitos.

Para reproducir la carrocería de la vista interior, ejecuta `Scripts/Editor/export_cabin_shell.py` en el editor y `Scripts/Blender/build_cabin_shell.py` con Blender. El primero extrae los dos grupos de materiales de `SM_SportsCar` a `Saved/CabinSource`. El segundo recorta el cofre y la parte trasera y mantiene normales y coordenadas UV. `Scripts/Editor/import_cabin_detail.py` asigna los materiales originales al asset nuevo. En la cámara exterior el coche muestra la carrocería completa.

En este equipo se comprobó también el arranque de escritorio con `Saved\StagedBuilds\Windows\RV_PIA.exe -d3d11 -nohmd`. Esta opción se aplica solo a esa ejecución. La prueba con el visor sigue pendiente.
