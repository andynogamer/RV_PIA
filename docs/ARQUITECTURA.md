# Arquitectura de implementación propuesta

Estado: arquitectura parcialmente implementada. La base de escritorio ya existe; las integraciones de hardware y RV real siguen pendientes. Consultar `ESTADO.md` y `DESARROLLO.md` antes de extenderla.

## Implementación actual

El vehículo Blueprint propio deriva de la plantilla Chaos y conserva su física y Enhanced Input. `URVDrivingAssistComponent` aplica reglas de sesión, marcha, velocidad y límites de práctica; `ARVPracticePlayerController` maneja eventos de teclado que siguen disponibles al pausar; `ARVPracticeHUD` presenta instrucciones de escritorio. Python construye assets y ejecuta pruebas de editor; no se usa para gameplay del producto. En el mapa urbano, suelo y 227 mallas de edificios cuentan con colisión. Las secciones siguientes orientan la ampliación, no certifican funciones de hardware.

`RVTrafficSystem.h/.cpp` aporta cuatro actores de ejecución: `ARVTrafficRoute` define un carril mediante spline y su población/velocidad; `ARVTrafficVehicle` sigue esa ruta con movimiento cinemático, barridos de colisión y detección de obstáculos; `ARVTrafficSignal` gobierna dos ejes incompatibles con seis fases; `ARVUrbanScenario` crea población, define límites/objetivos y registra avisos y eventos de claxon. La presencia del escenario activa el modo urbano de la asistencia; su ausencia conserva el circuito corto.

Los autos de tráfico no son vehículos Chaos ni usan navegación de peatones. Frenan según espacio disponible y señal, y solo reaparecen si el inicio está libre y el jugador está a más de 70 m de ambos extremos. Ese criterio usa distancia, no oclusión visual. Los semáforos y el tráfico usan tiempo del mundo y se detienen con la pausa.

Los dos controladores semafóricos enlazan mediante `FRVSignalLampBinding` las lentes originales de la ciudad (rojo/ámbar/verde y eje de circulación). Cambian sus materiales solo en el mundo de juego, sin modificar mallas ni materiales fuente. Cuando existen esos enlaces se ocultan los postes auxiliares del prototipo; los modelos originales siguen visibles y comparten el mismo ciclo que la IA. Los materiales solo se actualizan cuando cambia la fase. Los semáforos de otras zonas sin controlador siguen siendo decorativos.

`OnHornRequested(Vehicle, Reason)` está expuesto a Blueprint tanto en vehículo como escenario. `URVVehicleAudioComponent` escucha únicamente al escenario para no duplicar claxones. `manual` tiene cooldown de 1 s; `player_obstruction` requiere más de 5 s de bloqueo y tiene cooldown de 10 s. No se emite por la espera normal en rojo o falta de espacio de salida. El reproductor añade un límite global entre claxones.

La asistencia incorpora en ejecución `URVDrivingFeedbackComponent` y `URVVehicleAudioComponent` si todavía no existen. El primero evalúa reglas, colisiones, puntuación y reconocimientos; expone eventos Blueprint para audio y un futuro backend háptico. `ARVStopRule` define una línea de ALTO de práctica, distinta de las señales decorativas. El segundo controla bucles, pitch por régimen, rodadura, ambiente y efectos; pausa/mute cortan sonidos transitorios. Los motores de IA son componentes espaciales de los propios vehículos y obedecen el volumen/silencio del jugador.

El detector de rojo usa posición previa, dirección de desplazamiento y frente del vehículo; descarta saltos de posición. El contacto usa notificaciones de todos los cuerpos de la malla física, velocidad previa e impulso, excluyendo apoyo en suelo. También confirma pérdidas bruscas de velocidad contra geometría bloqueante, para cubrir recreaciones del estado de Chaos tras reinicio. Los detectores sostenidos usan ventanas temporales y cooldown por motivo. Las puntuaciones y mensajes son de entrenamiento, no un reglamento completo. Ver SONIDO_Y_RETROALIMENTACION.md.

## Plataforma y organización

La interfaz de sesión usa `ERVSessionScreen` (Start/Driving/Paused) en el controlador y `URVSessionMenuWidget` construido con UMG nativo. Inicio y pausa detienen el mundo mediante la asistencia, ponen acelerador a cero, muestran el cursor y usan entrada solo de UI. Conducción retira el widget y aplica `FInputModeGameOnly`, captura permanente del ratón y foco al viewport. El widget gestiona botones y navegación, mientras la asistencia conserva la seguridad de pausa/reinicio y liberación del acelerador. El HUD de conducción se oculta mientras está el menú. No hay dependencia del módulo UnrealEd en ejecución.

El inicio del menú espera al primer tick con Pawn/asistencia válidos. La pausa también se sincroniza si otra parte de la simulación llama a la asistencia directamente. La preferencia de PIE `GameGetsMouseControl` acompaña la captura; `DefaultEditorPerProjectUserSettings` no reemplaza preferencias locales ya guardadas. Esta implementación de pantalla plana deberá adaptarse a interacción espacial al integrar RV.

Unreal Engine 5.6, Windows PCVR como plataforma provisional. Es la vía inicial para integrar un volante USB con el PC y un visor compatible; confirmar el visor antes de cerrar el objetivo de compilación. No prometer ejecución autónoma en el visor con G920/Arduino conectados al PC.

Contenido nuevo en `/Game/RutaVirtual/`, dividido en Maps, Vehicles, Environment, Materials y otras carpetas cuando existan assets que lo justifiquen. Material importado en `/Game/Ciudad/`. Lógica C++ en `Source/RV_PIA/`; firmware futuro en `Firmware/`. No crear módulos vacíos como prueba de implementación.

## Separación funcional

### Entrada

Un adaptador por teclado, G920 y Arduino produce acciones lógicas: dirección, acelerador, freno, reversa, pausa y recentrado. La física recibe dirección normalizada en [-1, 1] y pedales en [0, 1], con límites, zona muerta y calibración explícitos.

Enhanced Input es el candidato para las acciones del juego. RawInput es el candidato a leer el volante en Windows. Validar su activación real y el mapeo al llegar el G920; no crear perfiles con índices copiados sin medirlos.

### Vehículo

Un Pawn de vehículo mantiene estado de marcha, cámara/cabina y componente de movimiento. Evaluar primero Chaos Vehicles y un vehículo base compatible. Las ruedas decorativas de la ciudad no constituyen un rig de Chaos. Antes de elegir el modelo revisar esqueleto, huesos, ruedas, Physics Asset, colisión, escala y licencia.

Evitar una simulación de neumáticos propia para esta entrega. Si no hay un vehículo utilizable, registrar la alternativa y sus límites antes de construirla. Mantener frenado y cambio de marcha deterministas; seleccionar reversa solo casi detenido y con acelerador liberado, con umbrales ajustables y probados.

### Vista de RV

Cabina fija como referencia, origen de tracking y cámara HMD dentro del vehículo. El giro del volante dirige al vehículo; la orientación de cabeza permite mirar de forma independiente. No rotar artificialmente la cámara para simular la mirada. Validar posición sentada, escala, recenter y composición con el visor real.

### Sesión de práctica

Estados previstos: inicio, calibración, preparado, práctica, pausa y resumen/salida. Pausar y perder dispositivo relevante llevan acelerador y hápticos a cero. Mantener reinicio seguro y ruta delimitada. Los puntos de recorrido aportan instrucciones y retroalimentación, sin diagnosticar capacidad real de conducción.

### Hápticos

Una interfaz de salida recibe eventos/intensidad de la simulación. Dos implementaciones independientes: fuerza en G920 y vibración externa Arduino. Ninguna debe bloquear el hilo de juego durante comunicación con dispositivos.

RawInput no se toma como backend probado de fuerza. Evaluar SDK oficial Logitech/DirectInput o plugin compatible con UE 5.6, licencia y empaquetado revisados. Arduino puede ofrecer una salida congruente adicional, pero debe demostrarse físicamente.

### Evidencia y datos

Mensajes técnicos de conexión, marcha, calibración y eventos en logs. Cualquier registro persistente de práctica se limita a datos técnicos anónimos y optativos. No recopilar identidad, diagnósticos, video o datos sensibles de participantes por defecto.

## Orden para introducir C++

1. Confirmar compilador Visual Studio/Build Tools, workload de C++, Windows SDK y compatibilidad con UE 5.6. Tener `vswhere` no prueba que el compilador esté instalado.
2. Crear módulo mínimo solo para una función necesaria, compilar desde editor cerrado y comprobar reapertura.
3. Exponer parámetros y funciones mediante propiedades y funciones Blueprint apropiadas; evitar lógica concentrada en Level Blueprint.
4. Validar una integración pequeña antes de extenderla. Si compilar falla, resolver o registrar la dependencia; no entregar archivos C++ como funcionalidad integrada.

## Decisiones aún abiertas

Tecnología concreta de FFB; tipo de conexión Arduino; modelo de visor/runtime; frecuencia objetivo; y selección final de recorrido para la demostración. El vehículo base Chaos y compilación C++ ya están comprobados. Documentar decisiones nuevas en `DECISIONES.md`.
