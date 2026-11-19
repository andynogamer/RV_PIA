# Validación y criterios de cierre

Cada tarea registra fecha, versión/commit cuando exista, comando o pasos, resultado observado y limitaciones. No ejecutar toda la batería por un cambio documental; seleccionar checks proporcionales.

## Documentos y gobernanza

- Verificar rutas y enlaces locales; nombres de assets existentes frente a propuestos; concordancia de requisitos con las fuentes.
- Ejecutar `git diff --check` y, si hay cambios preparados, `git diff --cached --check`.
- Comprobar que no se versionan cachés, credenciales, datos personales ni resultados físicos inventados.
- Para documentos finales PDF/DOCX: inspección visual de todas las páginas. La extracción de texto no demuestra calidad de maquetación.

## Scripts y configuración

- Parsear PowerShell y Python sin ejecutarlos para detectar errores de sintaxis.
- Ejecutar herramientas de inventario y comprobar que la salida es reciente, tiene el esquema previsto y no modifica assets.
- `Scripts/Audit-Unreal.ps1` requiere editor cerrado tras guardar; no se cierra automáticamente. Un bloqueo por editor abierto es una protección correcta, pero no valida el script interno.
- Un script Python de Unreal necesita ejecutarse con el motor y su plugin correspondiente; compilar sintaxis con Python externo no valida sus APIs.
- Revisar diff de `.ini`/`.uproject`, reapertura del editor y que los plugins requeridos estén realmente disponibles. La presencia de una carpeta de plugin no prueba que esté habilitado.

## Código y Blueprint

- C++: compilación UE 5.6 con el toolchain real; revisar UHT/UBT y reapertura. No confiar únicamente en Live Coding para cambios estructurales de clases.
- Blueprint: compilar los assets modificados, revisar errores y probar posesión/eventos en la escena adecuada.
- Priorizar pruebas de invariantes: acelerador cero al perder entrada; cambio de reversa bloqueado en movimiento; parser de hardware tolerante a fragmentos y timeout. Evitar tests que solo repitan asignaciones triviales.

## Escenario conducible

1. Abrir el mapa nuevo guardado; confirmar que la ciudad está colocada, no solo importada al Content Browser.
2. Comprobar escala con dimensiones conocidas, alineación de suelo y altura inicial.
3. Revisar colisión transitable de calles y obstáculos, incluyendo uniones, rampas, esquinas y límites. Colisión compleja o formas simples requieren comprobación en la física real del vehículo.
4. Recorrer la ruta prevista, detenerse y reiniciar; no caer bajo la calle ni quedar atrapado.
5. Revisar materiales/texturas y avisos de referencias ausentes.
6. Medir CPU/GPU y memoria durante el recorrido. No fijar calidad visual solo por el aspecto en editor.

## Conducción y sesión

- Menú: probar `test_session_menu.py` desde el inicio y luego interacción real: Play → Enter sin clic en la escena, botones de controles/volver/inicio, pausa P/M, reinicio, continuación y salida. La captura debe verificarse mediante foco y uso real, no únicamente con `show_mouse_cursor`.
- Confirmar que abrir inicio/pausa congela tráfico y reloj, que el acelerador queda a cero y que continuar con acelerador sostenido no produce movimiento. Probar el menú también en el circuito corto.
- En editor, Escape puede terminar PIE; P/M es el atajo estable del menú. El botón de salida debe terminar la prueba sin cerrar Unreal. La salida de un ejecutable empaquetado se valida por separado.

- Dirección izquierda/derecha con centro estable; pedales sueltos sin aceleración; aceleración/frenado previsibles.
- Reversa de teclado solo desde condición segura. En modo G920, ejecutar `Scripts/Editor/test_manual_shifter.py` en `L_CiudadPractica`. Comprueba 1–5, la posición física 6 asignada a reversa y el enclavamiento R de fábrica, desacople al pisar el embrague, neutral, bloqueo de cambios de sentido, pérdida de muestras y rearme tras pausa/reinicio.
- En una práctica desocupada del circuito corto, `Scripts/Editor/test_driver_cabin.py` comprueba el desplazamiento superior del aro con el coche frenado, la imagen y orientación de referencias traseras en los tres cristales, el ajuste, las superficies del tablero, el cambio de carrocería entre cámaras y la recuperación de la vista del asiento. Ejecución por el puente en el editor aislado. Informe en `Saved/AgentReports/driver_cabin_test.json`. Las referencias de color existen solo en PIE. Mantén la ventana activa para que el editor no limite la prueba a 3 FPS.
- Después de un informe reciente con `passed=true`, reiniciar PIE para `test_driving.py` y comprobar la regresión de teclado sin muestras de volante. Ejecutar las pruebas de entrada de una en una, sin nadie conduciendo ni salida física de fuerza activa. Usar un editor de pruebas iniciado con `Scripts/Open-Editor.ps1 -DisableWheelForTests -EnableAgentBridge -Map /Game/RutaVirtual/Maps/L_CiudadPractica` o desconectar el volante. El parámetro de editor aísla las muestras físicas durante la conducción sintética. Reabrir el proyecto normalmente para conducir con hardware.
- Pausa, pérdida de dispositivo, reinicio y salida llevan entrada y hápticos a estado seguro.
- Instrucciones legibles, actividad guiada completada y sonido ambiente audible sin tapar indicaciones.

## Ampliación urbana en escritorio

- Ejecutar `audit_urban_roads.py`: soporte de suelo y ausencia de obstáculos estáticos con volumen de tráfico a lo largo de los carriles. No sustituye recorrer otras calles o medir suspensión en bordes.
- En PIE urbano y de forma secuencial, ejecutar `test_urban_traffic.py`, `test_traffic_recycling.py` y `test_urban_route.py`. Esperar su JSON de resultado antes de iniciar otra prueba. Comprobar fecha y `passed=true`; un resultado anterior no valida cambios nuevos.
- El test de ruta usa física del jugador y desactiva IA temporalmente. El test de tráfico verifica reacción a señales y al jugador por separado; no afirmar por esa combinación que se validaron todas las situaciones de una ciudad real.
- Confirmar en imagen: inicio estacionado, señalización visible, orientación de autos y lámparas del semáforo coherentes con su fase.
- Volver a `L_CiudadPractica` y ejecutar `test_driving.py` como regresión tras cambios compartidos. Conservar mapa y assets originales.
- Claxon: distinguir evento, señal del mezclador y escucha física con altavoz/auriculares. No convertir un contador de eventos en evidencia acústica.

## Sonido, consecuencias y tráfico refinado

- Ejecutar `test_traffic_refinements.py`: rojo sin obstáculo adicional, retardo de reacción al verde, no entrar con salida ocupada, seguir un precedente móvil/detenido y luces de freno. No valida giros entre rutas ni incorporaciones complejas.
- Ejecutar `test_driving_feedback.py`: movimiento real para rojo y ALTO, detención correcta, choque contra pared, velocidad sostenida, sentido contrario y distancia. Comprobar límites de puntos, cooldown de contacto, reinicio, recompensa y bloqueo en menús. La solicitud háptica normalizada debe durar como máximo lo establecido, y existir cancelación; no equivale a vibración física.
- Contacto: no basta llamar manualmente `RegisterIncident`. Probar física real, incluida recreación del estado de Chaos tras reinicio; el detector combina notificaciones de cuerpos con pérdida brusca de velocidad confirmada contra geometría bloqueante. Apoyo en suelo, contacto estacionario y frenado normal no deben acumular penalizaciones.
- Ejecutar `test_audio_mix.py` dentro del editor y luego `Scripts/Audio/check_recorded_mix.py` con Python externo. Este último exige muestra audible con señal y sin saturación, y silencio en muestras mute/pausa. Registra WAV y métricas fuera de Git. Verificar también variación de tono con régimen y salida de claxon/recompensa; escuchar y ajustar posteriormente con el dispositivo de audio final.
- Revisar legibilidad del aviso correctivo y resumen de pausa; recompensa reciente no debe borrar un mensaje de error antes de poder leerlo. Evitar spam acústico. Repetir menú/conducción tras modificar componentes compartidos.

## RV y hardware reales

Para repetir la comparación estéreo sin visor, usa `-emulatestereo -RVRequireStereo`. Añade `-TestLoops=2` para repetir el recorrido en el mismo proceso. Consulta `r.pso.evictiontime` en la consola o al principio de `-ExecCmds` y comprueba que da cero en la configuración de entrega. Registra el pico y la tendencia de memoria privada al prolongar la sesión o añadir rutas.

- Para comprobar la estabilidad de la ciudad en el paquete Win64 Development, entra en Quest Link y ejecuta `RV_PIA.exe -vr -RVNoWheelInput -RVRequireStereo -ExecCmds="Automation RunTests RutaVirtual.City.DrivingStability" -TestExit="Automation Test Queue Empty"`. Deja desocupados el asiento y el volante mientras se ejecuta la conducción automática. El ensayo alterna la salida estacionada y la avenida del Distrito Norte con tráfico, una barrera temporal, pausas y reinicios. Exige al menos 180 s, cinco contactos y un tramo de 70 m o más. Para una comparación de escritorio, sustituye `-vr -RVRequireStereo` por `-nohmd`. Comprueba en el registro el mapa, `rhiname="D3D11"`, el tipo de vista, las distancias y `Result={Success}`. La prueba física del conductor se hace en otro arranque, sin aislamiento.

- Para reproducir el caso de carretera en un ejecutable Development desocupado, ejecutar `RV_PIA.exe /Game/RutaVirtual/Maps/L_Carretera -vr -RVNoWheelInput -ExecCmds="r.pso.EnableAsyncCacheConsolidation,Automation RunTests RutaVirtual.Road.HighwayShoulder" -TestExit="Automation Test Queue Empty"`. El ensayo toma el vehículo Chaos, sale del carril hacia un pino, regresa al asfalto y hace contacto con un segundo tronco. Comprueba el evento de choque y mantiene la simulación activa durante al menos 135 segundos desde el inicio de la prueba. El volante físico se aísla durante esta prueba. Confirmar aparte el pulso al conducir con el montaje real.

- Para repetir la regresión del menú y del asiento, parte de una sesión desocupada con Link activo y el visor quieto con seguimiento. Ejecuta `UnrealEditor.exe "<ruta a RV_PIA.uproject>" /Game/RutaVirtual/Maps/L_CiudadUrbana -game -vr -RVNoWheelInput -ExecCmds="Automation RunTests RutaVirtual.Session.StereoMenu" -TestExit="Automation Test Queue Empty"`. Espera `Result={Success}` y revisa la captura de la textura en `Saved/AgentReports/quest_menu.png`. La prueba usa pulsaciones sintéticas con el volante aislado. Comprueba botones sostenidos, navegación, inicio, pausa y freno de mano. Con OpenXR mide distancia entre cámara y asiento, y diferencia de rumbo respecto al coche tras ocho actualizaciones. Para probar composición y navegación sin visor, sustituye `-vr` por `-emulatestereo -nohmd`. [Capas estéreo de OpenXR en UE 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/openxr-stero-layers-overview-in-unreal-engine?application_version=5.6).
- En una sesión normal usa la cruceta física para resaltar **Ver controles**, pulsa A para abrirla y B para volver. Siéntate mirando al volante y confirma **Comenzar práctica** con A. Asegúrate de que ves el parabrisas al frente y que puedes mirar a cada lado. Con el coche parado, repite el ajuste con Y. Pulsa ☰ para pausar, B para continuar y X para liberar el freno de mano. Registra la observación de la persona antes de afirmar que la alineación o los botones físicos son correctos.

- Visor: runtime identificado, imagen estéreo y tracking reales, postura sentada, recentrado, ausencia de movimiento artificial de cabeza obligatorio.
- Rendimiento: frecuencia objetivo del visor y tiempos CPU/GPU medidos durante la ruta; declarar equipo, calidad y condiciones.
- Para registrar una lectura física en Unreal, abre Play en la ciudad, comienza la práctica y deja el juego en primer plano. Con el puente activo, ejecuta `Scripts/Invoke-Unreal.ps1 -Script Scripts/Editor/check_g920.py`. El informe `Saved/AgentReports/g920_real_pie.json` distingue `physical_shifter` (palanca), `gear` (selección aceptada) y `chaos_gear` (caja física). Con la palanca en quinta se puede añadir `-ArgumentsJson '{"expected_gear":"5"}'`. El informe requiere una muestra física válida para aprobar. Para observar varias posiciones, pasa `-ArgumentsJson '{"watch":true}'`. El registro `Saved/AgentReports/palanca_en_unreal.json` anota cambios durante la práctica, suspende la lectura en pausa y termina al salir de Play. Para aislar Windows usa `Scripts/Probar-Palanca.cmd`.
- G920 y Driving Force Shifter: comprobar entradas en `joy.cpl` y verificar que el HUD cambie a G920 automáticamente. Con el freno de mano aplicado, comprobar las tres barras de pedal. Para probar el modo DirectInput alternativo, completar **J** en el juego. Con el freno de mano aplicado, verificar **N**, **1–5** y **R** en el HUD, con **6 → R** al mover la palanca a la esquina inferior derecha. Luego verificar arranque en primera, dirección, frenado, rechazo de R en avance, pausa con el acelerador pisado y desconexión/reconexión a baja velocidad. La prueba PIE con muestras sintéticas cubre la integración Chaos y requiere esta comprobación física adicional.
- Háptico: identificar acción de entrada y evento de salida física; intensidad regulable y apagado probado. Distinguir autocentrado del driver de FFB enviado por el juego.
- Arduino: watchdog ante caída de la aplicación, arranque a cero y errores de comunicación; montaje revisado por el equipo.
- Registrar comodidad solo como observación del participante voluntario, sin convertirla en eficacia clínica. Detener la prueba si la persona lo solicita o presenta malestar.

## Ejecutable y entrega

Iniciar desde la carpeta empaquetada en el equipo de revisión, sin depender de editor, rutas de Downloads o archivos fuente externos. Confirmar plugins/drivers requeridos, mapa inicial, sonidos, controles, visor y háptico. Repetir inicio/práctica/salida y revisar logs.

Reunir documentación, video, publicidad y créditos conforme a `PROYECTO.md`. Un video de escritorio puede respaldar progreso; para la final no sustituye comprobar RV y háptico reales.

## Estados que se pueden reportar

- **Propuesto:** diseño sin implementación.
- **Implementado:** archivos o assets creados; describir qué validación falta.
- **Verificado en editor:** compilación y comportamiento observado en Unreal.
- **Verificado con hardware:** dispositivo, versión y prueba identificados.
- **Listo para entrega:** ejecutable y requisitos aplicables comprobados en conjunto.


## Vías abiertas y palanca en 6

Con el editor fuera de Play, `Scripts/Editor/audit_road_maps.py` recorre ambos sentidos de la carretera o la ampliación urbana en pasos de cuatro metros. Comprueba suelo, gálibo del vehículo, límites del escenario y recuento de señales. En una sesión de Play aislada con `-RVNoWheelInput`, `Scripts/Editor/test_open_road_drive.py` verifica el aviso de velocidad y la conducción real por la vía seleccionada. El test rural regresa al apartadero y completa los cinco objetivos. El urbano recorre el Distrito Norte. Espera el JSON reciente en `Saved/AgentReports` antes de ejecutar otra prueba.

En la ciudad, `test_urban_traffic.py` comprueba las 150 lentes durante las seis fases y el frenado del tráfico. En el circuito de conos, `test_manual_shifter.py` prueba ambas entradas de reversa y el rechazo de la posición 6 en movimiento. En el paquete Win64, abre el menú de los dos mapas, cambia de nivel, comienza la conducción y comprueba la señalización y el suelo. Para la sesión con hardware, abre Unreal sin el parámetro de aislamiento.

## Superficies visibles y patio de estacionamiento

Con el mapa urbano o rural abierto y fuera de Play, ejecuta `Scripts/Editor/audit_visible_ground.py` mediante `Scripts/Invoke-Unreal.ps1 -Script`. La prueba usa las secciones y los materiales efectivos de la malla renderizada. En la ciudad comprueba que la ampliación cubra la zona de las grietas. En la carretera toma siete muestras transversales cada dos metros y busca tierra sobre el pavimento o a menos de cinco centímetros por debajo. Complétala con `Scripts/Editor/audit_road_maps.py` para comprobar la geometría de colisión. La asociación entre sección y material se consulta con `get_lod_material_slot`. [Referencia de Epic para UE 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMeshEditorSubsystem?application_version=5.6)

Para reproducir la autoría, ejecuta `Scripts/Blender/build_roads.py` en Blender y luego `Scripts/Editor/import_roads.py` en Unreal. Usa la opción `meshes` para importar solo las mallas afectadas y `refresh` al actualizar una existente. Con la ciudad abierta, `Scripts/Editor/blend_city_ground.py` genera la máscara de las calles y los materiales de suelo. El argumento `{"parking":true}` de `Scripts/Editor/build_road_maps.py` construye el patio y guarda el mapa.

Para probar la maniobra, abre `L_Estacionamiento` con `Scripts/Open-Editor.ps1 -EnableAgentBridge -DisableWheelForTests -Map /Game/RutaVirtual/Maps/L_Estacionamiento`. Inicia Play y ejecuta `Scripts/Editor/test_parking.py` mediante el puente. Espera un `Saved/AgentReports/estacionamiento_pie.json` reciente con `pass=true`. El ensayo incluye cinco aproximaciones con los pedales virtuales y comprueba las colisiones de los coches estacionados, la duración de la parada y el reinicio. Espera el informe antes de ejecutar `Scripts/Editor/test_parking_route.py` en la misma sesión. Este segundo ensayo conduce desde el punto de salida y comprueba la secuencia completa en `Saved/AgentReports/patio_recorrido.json`. En el ejecutable Development empaquetado, `Scripts/Probar-EstacionamientoWin64.ps1` comprueba los cambios de mapa y captura la cabina del patio.
