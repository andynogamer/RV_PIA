# Estado comprobado del proyecto

Fecha: 14 de noviembre de 2026, America/Mexico_City. Hito actual: ciudad ampliada con tráfico autónomo ligero, verificada en el editor de escritorio. No está listo para la entrega final de RV.

## Implementado y comprobado

- Unreal Engine 5.6.1. Módulo C++ RV_PIA, objetivos Editor/Game, asistencia de práctica, controlador de jugador, HUD y cuatro actores de tráfico/escenario.
- Compilaciones Editor y Game Win64 Development correctas mediante UHT/UBT con MSVC 14.51.36256 y Windows SDK 10.0.22621. Comandos: `Scripts/Build-Project.ps1` y `Scripts/Build-Project.ps1 -Target Game`, con editor cerrado. Logs: `Saved/AgentReports/urban_build_verified.log` y `urban_game_build.log`. UBT advierte que el compilador preferido es 14.38; no se alteró el motor para compilar. Compilar Game genera el binario, pero no cocina ni empaqueta el contenido.
- /Game/RutaVirtual/Maps/L_CiudadPractica: 1,194 actores de mallas de ciudad, iluminación y cielo. Las posiciones ya estaban incorporadas en la importación; no se reimportaron los originales.
- Suelo conducible duplicado con colisión compleja, preservando el original. Recorrido recto de 40 m, 22 conos, zona final de detención y señal de fin.
- Vehículo Chaos derivado de la plantilla oficial instalada de UE 5.6. Blueprints propios de vehículo, modo y controlador de práctica.
- Cabina y volante originales creados con Blender 3.6; fuentes .blend, FBX y generador Python incluidos. Cámara de cabina activa y volante visual ligado a dirección.
- W/S aceleran/frenan; A/D dirigen; R selecciona avance/reversa solo casi detenido y sin acelerar; P pausa; Home reinicia. Se mantiene Tab para cámara y L para luces de plantilla.
- Límite nominal de avance de 30 km/h y de reversa de 5 km/h, con corte de aceleración. Picos observados en la prueba final: aproximadamente 30.6 y 5.33 km/h. No es un limitador certificado para pendientes o vehículos diferentes.
- Práctica con instrucciones, objetivo de avance y detención final. Salir del corredor reinicia de forma segura. Reanudar/reiniciar exige liberar un acelerador que permanecía pulsado.
- Nuevo mapa inicial `/Game/RutaVirtual/Maps/L_CiudadUrbana`, separado del circuito corto y de la sala RV. Límites de exploración de 770 × 760 m; suelo existente y copias de 227 mallas de edificios con colisión compleja. Se retiraron del mapa urbano 88 piezas de autos decorativos para liberar carriles, sin modificar sus fuentes.
- Salida estacionada con freno de mano, liberación con pulsación y suelta de Espacio; cuatro objetivos urbanos y detención en bahía final, orientación aproximada y distancia en HUD. Home vuelve al estacionamiento y limpia progreso/avisos.
- Cuatro rutas de tráfico de aproximadamente 500–560 m, con 12 autos cinemáticos, aceleración/frenado gradual, detección de jugador/vehículos y barridos para evitar atravesar obstáculos. Recirculación condicionada a carril libre y distancia del jugador a ambos extremos mayor de 70 m.
- Dos cruces semaforizados: 12 s verde, 3 s ámbar, 2 s de separación en rojo por movimiento. Ejes incompatibles nunca comparten verde. Se usan las lentes originales del escenario enlazadas al controlador; las señales auxiliares se ocultan al enlazarlas.
- Eventos Blueprint de claxon (`manual`, `player_obstruction`) y contador de avisos de rojo. El auto solicita claxon tras más de 5 s bloqueado por el jugador, con 10 s de cooldown y sin hacerlo por esperar en rojo. H solicita evento manual con cooldown de 1 s. **No hay sonido audible conectado todavía.**

## Evidencia de validación

- Scripts/Editor/test_driving.py: 15 comprobaciones de integración correctas mediante física real de PIE e inyección de acciones Enhanced Input. Incluyen acelerar, frenar, detener, girar, marcha/reversa, cambio bloqueado en movimiento, límites efectivamente alcanzados, apoyo en suelo, pausa y reinicio con acelerador sostenido. Informe del 14 de noviembre, con Unreal en primer plano: Saved/AgentReports/driving_test.json, passed=true.
- Scripts/Editor/test_practice_route.py: recorrido completo mediante acciones reales; estados observados 0 → 1 → 2, detención final a unos 0.29 km/h e instrucción de práctica completada. Duración simulada aproximada de 15.2 s a baja velocidad. Informe: Saved/AgentReports/practice_route_test.json, passed=true.
- Inspección visual de ciudad, cabina, conos, instrucciones y final del recorrido. Prueba con teclado por interfaz: Home reinicia, R selecciona reversa, P pausa y reanuda; Home devuelve avance y etapa inicial.
- Sintaxis PowerShell/Python revisada. Hashes de los 1,615 archivos originales de Content: intactos, incluida la sala original de RV y la ciudad importada.
- `test_urban_traffic.py`: 21 comprobaciones correctas, incluidas parada/reanudación ante rojo y ante jugador, partida estacionada, fases incompatibles, correspondencia de las 36 lentes visibles con el ciclo, claxon sin spam y pausa del tráfico/reloj. Informe `Saved/AgentReports/urban_traffic_test.json`, passed=true.
- `test_traffic_recycling.py`: cuatro comprobaciones correctas, incluyendo inicio ocupado, carril contrario contiguo y jugador cerca. Se corrigió un bloqueo mutuo entre extremos de carriles opuestos. Informe `Saved/AgentReports/traffic_recycling_test.json`.
- `audit_urban_roads.py`: 216 muestras de suelo y barridos con volumen de automóvil a lo largo de las cuatro rutas; sin huecos detectados ni obstáculos estáticos. Informe `Saved/AgentReports/urban_road_audit.json`, passed=true.
- `test_urban_route.py`: seis comprobaciones correctas mediante Chaos/Enhanced Input. Distancia física observada 245.29 m, cuatro objetivos y estados 0 → 1 → 2 → 3, final detenido, unos 58.3 s simulados. IA desactivada y señales verdes durante esta prueba para aislar la carretera; la interacción de tráfico se prueba por separado. Informe `Saved/AgentReports/urban_route_test.json`, passed=true.
- Contenido nuevo en carpetas propias y dependencias oficiales en VehicleTemplate, Vehicles e Input. Los originales de ciudad y sala RV se conservan.
- Regresión posterior a la ampliación: las 15 comprobaciones de `test_driving.py` volvieron a pasar en el circuito corto. Inspección visual de señales originales controladas, postes auxiliares ocultos y circulación en el cruce sur. No se observaron modificaciones de los 1,615 archivos originales de Content por comparación SHA-256. Inventario actual: 1,965 archivos de contenido, 764,055,061 bytes y tres mapas.

Las pruebas de inyección son herramientas del editor, no lógica Python del producto. Reportes, logs y binarios compilados quedan fuera de Git; los scripts para reproducirlos y el código sí se versionan.

## Límites actuales

- Colisión ampliada y rutas comprobadas; otras calles, accesorios y bordes no se han recorrido exhaustivamente. Los 770 × 760 m son límites, no superficie certificada como transitable.
- El tráfico es cinemático y sigue splines: faltan giros entre rutas, cesión completa al incorporarse, cambios de carril, variedad de carrocerías y luces de freno/direccionales. La reaparición comprueba distancia, no oclusión. Los semáforos fuera de los dos cruces enlazados son decorativos. El contador de rojo no es un evaluador completo de normas.
- Cabina y HUD son prototipos de escritorio. Falta adaptar/verificar interfaz espacial, postura, recentrado y comodidad con visor real.
- No se han probado G920, force feedback ni Arduino. RawInput está habilitado, pero no se configuró un perfil de ejes por suposición ni se declaró respuesta háptica funcional.
- Faltan sonido del vehículo/ambiente, medición CPU/GPU en RV y prueba de ejecutable empaquetado. Las compilaciones Editor/Game no demuestran cocina, empaquetado ni funcionamiento en otro equipo.
- Visor sin identificar. El registro de la base avisaba de runtime OpenXR no disponible; debe verificarse con hardware real.
- Se conservó la configuración de render heredada. Claves repetidas, Lumen/ray tracing y opciones Android/Meta no se consideran configuración final.
- Procedencia/licencia de ciudad pendiente de completar; el usuario autorizó uso privado académico. No se subió este desarrollo a servicios externos desde esta sesión.

## Repositorio

Base de gobernanza: cd30fcc y 9731d0f; circuito corto: 7c37ebb. El usuario configuró el remoto GitHub origin y guardó un punto de interrupción dbf8944. La ampliación se desarrolla en feature/ciudad-trafico; no se hizo push automático. Consultar git log y git status para el cierre actual.

## Siguiente hito

Completar sonido y conectar los eventos de claxon; identificar visor y probar PCVR; calibrar G920 cuando llegue; integrar y demostrar salida háptica congruente; verificar empaquetado. La ampliación urbana solicitada ya tiene una primera versión; continuar su evolución sin dar por cumplidos esos requisitos indispensables.
