# Desarrollo y pruebas de conducción

## Ciudad urbana (mapa inicial)

Nivel: `/Game/RutaVirtual/Maps/L_CiudadUrbana`. Comienzas estacionado con freno de mano aplicado. Pulsa y suelta Espacio, observa el carril e incorpórate. El HUD muestra distancia al siguiente objetivo y orientación aproximada; recorre cuatro objetivos hacia el norte y detente en la bahía verde final. Home reinicia al estacionamiento y limpia los avisos de rojo. Después de completar se permite explorar dentro de los límites.

Hay 12 autos sobre cuatro rutas de unos 500–560 m cada una, y dos cruces con semáforos. La zona delimitada mide 770 × 760 m; esa cifra es el límite de exploración, no la longitud de calles verificadas. El recorrido guiado comprobado con física mide aproximadamente 245 m. Las rutas de tráfico se comprobaron por muestras de suelo y barridos de colisión; no se certificó cada rincón de la ciudad.

Respeta la señalización y deja distancia: los autos frenan y esperan, pero su movimiento es cinemático. No hacen todavía giros entre rutas, adelantamientos ni maniobras de conductores humanos. Las señales físicas ALTO y límite sirven de referencia; no existe aún un evaluador completo de normas. El contador de rojo es una ayuda de prototipo.

H solicita un evento de claxon manual con cooldown de 1 s. Un auto bloqueado por el jugador más de 5 s solicita un claxon contextual, con cooldown de 10 s. **Todavía no se oye:** el evento está preparado para el sistema de sonido posterior. No solicita claxon por esperar correctamente en rojo.

## Circuito corto

Nivel: `/Game/RutaVirtual/Maps/L_CiudadPractica`. Vehículo compartido: `/Game/RutaVirtual/Vehicles/BP_VehiculoPractica`. El trabajo nuevo está separado de la sala original de RV.

## Probar en Unreal

1. Abrir el proyecto con UE 5.6 y cargar `L_CiudadPractica` si no aparece automáticamente.
2. Pulsar Play y hacer clic dentro de la vista para capturar teclado.
3. Avanzar entre los conos y detenerse en la zona verde al final. Es una práctica corta, sin competición ni límite de tiempo.
4. Salir de Play con Escape. En editor, Shift+F1 libera el ratón.

Controles de escritorio:

- W o flecha arriba: acelerar en la marcha seleccionada.
- S o flecha abajo: frenar. El freno no selecciona reversa.
- A/D o flechas izquierda/derecha: dirección.
- R: alternar avance y reversa, solo casi detenido y con acelerador liberado.
- Espacio: freno de mano.
- P: pausar o continuar.
- Home/Inicio: reiniciar la práctica desde el punto de partida.
- Tab: alternar cámaras. L: luces del vehículo de plantilla.
- H: evento de claxon manual en el mapa urbano; audio pendiente.
- Retroceso conserva la función de enderezar el vehículo de la plantilla; Home es el reinicio completo.

Los límites iniciales son 30 km/h en avance y 5 km/h en reversa, con corte de aceleración. Son parámetros de prototipo y deben comprobarse de nuevo al cambiar física, vehículo o entradas; la prueba final observó picos de aproximadamente 30.6 y 5.33 km/h.

## Archivos de lógica

- `RVDrivingAssistComponent`: condiciones de cambio de marcha, límite de velocidad, pausa, reinicio, límites por mapa, objetivos de la práctica y volante visual. Detecta el escenario urbano para activar freno inicial y objetivos de mayor alcance.
- `RVPracticePlayerController`: eventos de teclado R, P, Home y H, incluida reanudación durante pausa. El Blueprint derivado conserva los mapeos Enhanced Input de la plantilla.
- `RVPracticeHUD`: instrucciones y estado para la prueba de escritorio. No es una interfaz de RV validada; adaptar a widgets espaciales al probar el visor.
- `RVTrafficSystem`: rutas, vehículos autónomos cinemáticos, semáforos y escenario urbano; ver ARQUITECTURA.md.

La física y las acciones de conducción básicas provienen de la plantilla oficial Chaos de UE 5.6. La lógica de sesión nueva se ejecuta en C++; Python se utiliza para construir/verificar contenido dentro del editor.

## Herramientas para agentes

`Scripts/Build-Project.ps1` compila el objetivo Editor con el editor cerrado; `-Target Game` compila el objetivo de juego. El desarrollo necesita compilador C++ y Windows SDK. La sesión actual usó MSVC 14.51 y SDK 10.0.22621; UBT advierte que prefiere MSVC 14.38. No se modificó el motor para permitir esta compilación.

`Scripts/Open-Editor.ps1` abre el mapa inicial configurado. `-Map /Game/RutaVirtual/Maps/L_CiudadPractica` permite abrir el circuito corto. Su opción `-EnableAgentBridge` inicia una cola temporal de tareas locales, sin escucha de red. `Scripts/Invoke-Unreal.ps1` ejecuta scripts de editor dentro de esa sesión; `-Stop` desactiva la cola. No ejecuta automáticamente tareas pendientes de una sesión anterior. Si se inició el editor con la opción de script, la notificación de ejecución puede permanecer hasta cerrar esa sesión; abrir normalmente para trabajar sin esa notificación.

Orden de construcción desde la base, sin sobrescribir un mapa existente: instalar recursos con `Install-VehicleTemplate.ps1`, ejecutar `build_city.py`, `setup_vehicle.py`, `import_cockpit.py`, `configure_runtime.py` y `build_training_route.py` dentro del editor. Los plugins y el módulo deben estar habilitados/compilados primero. Las fuentes importadas y los assets resultantes ya están incluidos en el repositorio.

Para la ampliación: compilar las clases de tráfico, ejecutar `prepare_traffic_material.py`, `build_urban_scene.py` una sola vez, `polish_urban_scene.py` y `bind_city_signals.py`. Estos assets ya están guardados: no reconstruirlos para probar. El último script enlaza las lentes de los semáforos originales con el ciclo funcional; evita luces decorativas contradictorias en los dos cruces controlados. Los de otras calles aún son decorativos. El mapa se crea con `LevelEditorSubsystem.new_level_from_template`; no duplicar un `World` mediante `EditorAssetLibrary.duplicate_asset` para cargarlo después: esa vía produjo un error de referencias de mundo en UE 5.6 durante el desarrollo.

`test_driving.py` requiere PIE activo, Unreal en primer plano y un único jugador local; inyecta acciones Enhanced Input durante una prueba controlada y deja un JSON en `Saved/AgentReports/driving_test.json`. La reducción de CPU del editor en segundo plano puede invalidar los tiempos de la prueba física. No ejecutarlo mientras una persona conduzca o haya un dispositivo físico de fuerza activo.

Pruebas urbanas, ejecutadas de una en una con PIE urbano activo y Unreal en primer plano:

- `test_urban_traffic.py`: estacionamiento, fases, parada/reanudación ante señal/jugador, evento de claxon y pausa de tráfico/reloj. Cambia posiciones durante la prueba.
- `test_traffic_recycling.py`: espera ante inicio ocupado, recirculación sin bloqueo del carril contrario y protección cuando el jugador está cerca.
- `test_urban_route.py`: recorrido físico guiado hasta la detención final; oculta y desactiva temporalmente IA y fuerza semáforos verdes para aislar la transitabilidad del recorrido. Restaura al finalizar.
- `audit_urban_roads.py`: 216 muestras de suelo y barridos con volumen de automóvil a lo largo de cuatro rutas. También funciona en editor sin PIE.

`play_current_level.py` inicia PIE conservando PlayerStart. Los scripts de prueba antiguos preparados en `Saved/AgentBridge/tasks` son temporales, pueden referirse al punto de partida anterior y no deben usarse como herramientas permanentes.

## Límites de esta versión

La zona practicable depende del mapa: corredor acotado en el circuito corto, rectángulo amplio con recuperación fuera de límites en el urbano. El urbano añade colisión compleja en copias de 227 mallas de edificios; accesorios, otras calles y bordes no tienen validación exhaustiva. Los originales permanecen intactos. No presentar la ciudad entera como circuito terminado.

La cabina es una primera malla de referencia creada en Blender, con volante visual. Faltan refinamiento interior, interfaz espacial, sonido, calibración de G920, visor real, hápticos, medición de rendimiento en RV y prueba de ejecutable empaquetado. La prueba de escritorio no cubre esos requisitos.
