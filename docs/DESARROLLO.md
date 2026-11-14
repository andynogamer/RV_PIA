# Primera práctica de conducción

Nivel: `/Game/RutaVirtual/Maps/L_CiudadPractica`. Vehículo: `/Game/RutaVirtual/Vehicles/BP_VehiculoPractica`. El trabajo nuevo está separado de la sala original de RV.

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
- Retroceso conserva la función de enderezar el vehículo de la plantilla; Home es el reinicio completo.

Los límites iniciales son 30 km/h en avance y 5 km/h en reversa, con corte de aceleración. Son parámetros de prototipo y deben comprobarse de nuevo al cambiar física, vehículo o entradas; la prueba final observó picos de aproximadamente 30.6 y 5.33 km/h.

## Archivos de lógica

- `RVDrivingAssistComponent`: condiciones de cambio de marcha, límite de velocidad, pausa, reinicio, límites del corredor, objetivos de la práctica y volante visual.
- `RVPracticePlayerController`: eventos de teclado R, P y Home, incluida reanudación durante pausa. El Blueprint derivado conserva los mapeos Enhanced Input de la plantilla.
- `RVPracticeHUD`: instrucciones y estado para la prueba de escritorio. No es una interfaz de RV validada; adaptar a widgets espaciales al probar el visor.

La física y las acciones de conducción básicas provienen de la plantilla oficial Chaos de UE 5.6. La lógica de sesión nueva se ejecuta en C++; Python se utiliza para construir/verificar contenido dentro del editor.

## Herramientas para agentes

`Scripts/Build-Project.ps1` compila el objetivo Editor con el editor cerrado; `-Target Game` compila el objetivo de juego. El desarrollo necesita compilador C++ y Windows SDK. La sesión actual usó MSVC 14.51 y SDK 10.0.22621; UBT advierte que prefiere MSVC 14.38. No se modificó el motor para permitir esta compilación.

`Scripts/Open-Editor.ps1` abre la escena. Su opción `-EnableAgentBridge` inicia una cola temporal de tareas locales, sin escucha de red. `Scripts/Invoke-Unreal.ps1` ejecuta scripts de editor dentro de esa sesión; `-Stop` desactiva la cola. No ejecuta automáticamente tareas pendientes de una sesión anterior. Si se inició el editor con la opción de script, la notificación de ejecución puede permanecer hasta cerrar esa sesión; abrir normalmente para trabajar sin esa notificación.

Orden de construcción desde la base, sin sobrescribir un mapa existente: instalar recursos con `Install-VehicleTemplate.ps1`, ejecutar `build_city.py`, `setup_vehicle.py`, `import_cockpit.py`, `configure_runtime.py` y `build_training_route.py` dentro del editor. Los plugins y el módulo deben estar habilitados/compilados primero. Las fuentes importadas y los assets resultantes ya están incluidos en el repositorio.

`test_driving.py` requiere PIE activo, Unreal en primer plano y un único jugador local; inyecta acciones Enhanced Input durante una prueba controlada y deja un JSON en `Saved/AgentReports/driving_test.json`. La reducción de CPU del editor en segundo plano puede invalidar los tiempos de la prueba física. No ejecutarlo mientras una persona conduzca o haya un dispositivo físico de fuerza activo.

## Límites de esta versión

La zona practicable está acotada. El suelo y los conos tienen colisión; el resto de la ciudad se conserva inicialmente como escenario visual y no ofrece colisión completa. Salir del corredor provoca reinicio seguro. No presentar la ciudad entera como circuito terminado.

La cabina es una primera malla de referencia creada en Blender, con volante visual. Faltan refinamiento interior, interfaz espacial, sonido, calibración de G920, visor real, hápticos, medición de rendimiento en RV y prueba de ejecutable empaquetado. La prueba de escritorio no cubre esos requisitos.
