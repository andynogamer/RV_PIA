# Arquitectura de implementación propuesta

Estado: arquitectura parcialmente implementada. La base de escritorio ya existe; las integraciones de hardware y RV real siguen pendientes. Consultar `ESTADO.md` y `DESARROLLO.md` antes de extenderla.

## Implementación actual

El vehículo Blueprint propio deriva de la plantilla Chaos y conserva su física y Enhanced Input. `URVDrivingAssistComponent` aplica reglas de sesión, marcha, velocidad y límites de práctica; `ARVPracticePlayerController` maneja eventos de teclado que siguen disponibles al pausar; `ARVPracticeHUD` presenta instrucciones de escritorio. Python construye assets y ejecuta pruebas de editor; no se usa para gameplay del producto. El suelo y los conos tienen colisión; la ciudad restante es visual. Las secciones siguientes orientan la ampliación, no certifican funciones de hardware.

## Plataforma y organización

Unreal Engine 5.6, Windows PCVR como plataforma provisional. Es la vía inicial para integrar un volante USB con el PC y un visor compatible; confirmar el visor antes de cerrar el objetivo de compilación. No prometer ejecución autónoma en el visor con G920/Arduino conectados al PC.

Contenido nuevo en `/Game/RutaVirtual/`, dividido en Maps, Vehicles, Input, UI, Audio y Data cuando existan assets que lo justifiquen. Material importado en `/Game/Ciudad/`. Lógica C++ futura en `Source/RV_PIA/`; firmware futuro en `Firmware/`. No crear módulos vacíos como prueba de implementación.

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

Vehículo base; tecnología concreta de FFB; tipo de conexión Arduino; modelo de visor/runtime; frecuencia objetivo; disponibilidad de compilación C++; y ruta del nivel final. Documentar cada elección en `DECISIONES.md` al contar con evidencia.
