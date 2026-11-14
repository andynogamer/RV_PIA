# Estado comprobado del proyecto

Fecha: 14 de noviembre de 2026, America/Mexico_City. Hito actual: primer prototipo de conducción verificado en el editor de escritorio. No está listo para la entrega final de RV.

## Implementado y comprobado

- Unreal Engine 5.6.1. Se agregó el módulo C++ RV_PIA, sus objetivos Editor/Game y tres clases: reglas de práctica, controlador de jugador y HUD.
- Compilación Editor correcta mediante UHT/UBT con MSVC 14.51.36256 y Windows SDK 10.0.22621. UBT advierte que el compilador preferido es 14.38; no se alteró el motor para compilar.
- /Game/RutaVirtual/Maps/L_CiudadPractica: 1,194 actores de mallas de ciudad, iluminación y cielo. Las posiciones ya estaban incorporadas en la importación; no se reimportaron los originales.
- Suelo conducible duplicado con colisión compleja, preservando el original. Recorrido recto de 40 m, 22 conos, zona final de detención y señal de fin.
- Vehículo Chaos derivado de la plantilla oficial instalada de UE 5.6. Blueprints propios de vehículo, modo y controlador de práctica.
- Cabina y volante originales creados con Blender 3.6; fuentes .blend, FBX y generador Python incluidos. Cámara de cabina activa y volante visual ligado a dirección.
- W/S aceleran/frenan; A/D dirigen; R selecciona avance/reversa solo casi detenido y sin acelerar; P pausa; Home reinicia. Se mantiene Tab para cámara y L para luces de plantilla.
- Límite nominal de avance de 30 km/h y de reversa de 5 km/h, con corte de aceleración. Picos observados en la prueba final: aproximadamente 30.6 y 5.33 km/h. No es un limitador certificado para pendientes o vehículos diferentes.
- Práctica con instrucciones, objetivo de avance y detención final. Salir del corredor reinicia de forma segura. Reanudar/reiniciar exige liberar un acelerador que permanecía pulsado.

## Evidencia de validación

- Scripts/Editor/test_driving.py: 15 comprobaciones de integración correctas mediante física real de PIE e inyección de acciones Enhanced Input. Incluyen acelerar, frenar, detener, girar, marcha/reversa, cambio bloqueado en movimiento, límites efectivamente alcanzados, apoyo en suelo, pausa y reinicio con acelerador sostenido. Informe del 14 de noviembre, con Unreal en primer plano: Saved/AgentReports/driving_test.json, passed=true.
- Scripts/Editor/test_practice_route.py: recorrido completo mediante acciones reales; estados observados 0 → 1 → 2, detención final a unos 0.29 km/h e instrucción de práctica completada. Duración simulada aproximada de 15.2 s a baja velocidad. Informe: Saved/AgentReports/practice_route_test.json, passed=true.
- Inspección visual de ciudad, cabina, conos, instrucciones y final del recorrido. Prueba con teclado por interfaz: Home reinicia, R selecciona reversa, P pausa y reanuda; Home devuelve avance y etapa inicial.
- Sintaxis PowerShell/Python revisada. Hashes de los 1,615 archivos originales de Content: intactos, incluida la sala original de RV y la ciudad importada.
- Inventario antes del cierre: 1,736 archivos Content, 733,441,145 bytes y dos mapas guardados. Contenido nuevo en carpetas propias y dependencias de plantilla en VehicleTemplate, Vehicles e Input.

Las pruebas de inyección son herramientas del editor, no lógica Python del producto. Reportes, logs y binarios compilados quedan fuera de Git; los scripts para reproducirlos y el código sí se versionan.

## Límites actuales

- El suelo y los conos tienen colisión. La ciudad fuera del corredor es escenario visual y todavía no tiene colisión completa; no ofrecerla como circuito libre terminado.
- Cabina y HUD son prototipos de escritorio. Falta adaptar/verificar interfaz espacial, postura, recentrado y comodidad con visor real.
- No se han probado G920, force feedback ni Arduino. RawInput está habilitado, pero no se configuró un perfil de ejes por suposición ni se declaró respuesta háptica funcional.
- Faltan sonido del vehículo/ambiente, medición CPU/GPU en RV y prueba de ejecutable empaquetado. Compilar Editor no demuestra empaquetado ni funcionamiento en otro equipo.
- Visor sin identificar. El registro de la base avisaba de runtime OpenXR no disponible; debe verificarse con hardware real.
- Se conservó la configuración de render heredada. Claves repetidas, Lumen/ray tracing y opciones Android/Meta no se consideran configuración final.
- Procedencia/licencia de ciudad pendiente de completar; el usuario autorizó uso privado académico. No se subió este desarrollo a servicios externos desde esta sesión.

## Repositorio

Base de gobernanza: cd30fcc y 9731d0f. El usuario configuró después el remoto GitHub origin. Este hito se desarrolla en feature/primer-circuito; no se hizo push automático. Consultar git log y git status para el cierre actual.

## Siguiente hito

Completar sonido y pulir presentación; identificar visor y probar PCVR; calibrar G920 cuando llegue; integrar y demostrar salida háptica congruente; después verificar empaquetado. No ampliar tráfico o ciudad antes de resolver esos requisitos.
