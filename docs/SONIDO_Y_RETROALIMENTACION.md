# Sonido y retroalimentación de práctica

## Alcance

Solicitud del usuario del 14 de noviembre: introducir sonido, refinar tráfico y dar consecuencias negativas ante infracciones/siniestros, con vibración posterior y algún reconocimiento de conducción cuidadosa. Es una práctica académica introductoria. La puntuación no diagnostica ansiedad, no mide eficacia terapéutica y no certifica aptitud ni cumplimiento legal.

La sesión empieza con **100 puntos de seguridad**, limitados a 0–100. Los errores descuentan, y las acciones cuidadosas permiten recuperar puntos y sumar reconocimientos. No hay clasificación entre personas, presión de tiempo para terminar ni derrota por llegar a cero. Reiniciar inicia un intento nuevo y limpia el balance. Se priorizan mensajes que describen el error y cómo corregirlo.

## Reglas implementadas

- Rojo: −12, al cruzar con el frente del vehículo la línea de un cruce controlado. Usa desplazamiento continuo y dirección de movimiento; descarta saltos de posición para evitar evaluar un teletransporte.
- ALTO de salida: −10, si se cruza el marcador sin permanecer al menos 1 s detenido en su zona previa. Solo se evalúa el marcador nuevo de salida, no todos los letreros decorativos del escenario.
- Contacto: −20, con umbral de impacto horizontal de 2.5 km/h, velocidad previa e impulso físico. También se comprueba una pérdida brusca de velocidad contra geometría bloqueante para cubrir la recreación del estado de Chaos. Se ignoran apoyo normal en suelo, frenado normal y contactos muy pequeños; no se atribuye culpa legal a un conductor.
- Exceso de velocidad: −5, por más de 2.5 s superando el límite más 2 km/h de tolerancia. El límite del evaluador es configurable, por defecto 30 km/h.
- Seguimiento cercano: −4, tras más de 1.5 s de proximidad directa a otro automóvil, avanzando a más de 8 km/h.
- Sentido contrario: −8, tras más de 2.5 s avanzando a más de 8 km/h contra una ruta próxima. Se excluyen zonas de cruce y la reversa lenta; solo se evalúan carriles definidos.

Cada tipo de error tiene cooldown de 10 s; contacto usa 4 s. El aviso acústico tiene además su propio límite para que distintos errores no produzcan una secuencia continua de claxones. Los mensajes se mantienen visibles varios segundos y no destellan.

Reconocimientos: detención correcta en rojo o ALTO (+3), objetivos urbanos (+3), conducción mantenida sin incidentes (+2 cada 20 s de movimiento válido) y final de recorrido (+10). Las paradas se reconocen una vez por marcador y sesión. A 100 puntos el reconocimiento se muestra sin inventar un incremento; un mensaje correctivo reciente conserva prioridad para poder leerlo.

## Sonidos y controles

Motor según régimen/carga, rodadura según velocidad, ambiente suave, motores de tráfico con atenuación espacial, claxon manual/contextual, sonido de contacto y tono breve de reconocimiento. V silencia/restaura el conjunto; H activa claxon. La pausa silencia los bucles y corta efectos transitorios para no reproducir avisos antiguos al continuar.

Los seis WAV son **síntesis original de prototipo**, no grabaciones de un automóvil real. Fuentes en `SourceAudio/`, generador reproducible `Scripts/Audio/generate_audio.py`, assets en `/Game/RutaVirtual/Audio/`. Se pueden reemplazar sus timbres posteriormente por grabaciones con procedencia y autorización documentadas. Volumen maestro inicial 0.7, editable en el componente; no equivale a una medida de presión sonora en los auriculares.

## Preparación para vibración

`URVDrivingFeedbackComponent.OnFeedback` expone código, mensaje, tipo, cambio real de puntos y solicitud háptica lógica normalizada. Las penalizaciones solicitan un pulso de 0.15 s; los reconocimientos tienen solicitud cero. `OnFeedbackStopped` ordena cancelar la salida al pausar, reiniciar y terminar.

**No existe salida física todavía.** La solicitud normalizada no es voltaje, corriente ni duty cycle listo para conectar. El backend futuro debe limitar intensidad/duración, arrancar a cero, implementar watchdog y desconexión, respetar cancelación y validarse con el montaje real según HARDWARE.md. No se alimenta un motor desde un GPIO.

## Tráfico refinado

Los agentes conservan movimiento cinemático por splines. Tienen preferencias reproducibles de velocidad, aceleración y reacción; mantienen espacio dependiente de velocidad y vehículo precedente, esperan brevemente antes de arrancar en verde y muestran luces de freno. Se comprueba espacio al otro lado del cruce antes de entrar. El claxon contextual se reserva al bloqueo prolongado por el jugador, sin hacerlo por esperar correctamente ante rojo o falta de salida.

Siguen pendientes rutas conectadas con giros, cesión completa al incorporarse, adelantamientos y direccionales. No describir esta versión como un modelo completo de conductores humanos.

## Validación reproducible

Ejecutar de una en una, con Unreal en primer plano, sin persona conduciendo ni salida física de fuerza activa:

- `test_urban_traffic.py`: regresión de señales, obstáculos, pausa y claxon.
- `test_traffic_refinements.py`: señal sin obstáculo adicional, reacción, espacio de salida, seguimiento y luces.
- `test_driving_feedback.py`: física del jugador para rojo, ALTO, contacto, velocidad, sentido contrario, seguimiento y recompensas. Modifica posiciones y parámetros temporalmente y los restaura.
- `test_audio_mix.py`: graba la salida maestra del juego en estados audible, silenciado y pausado.
- `Scripts/Audio/check_recorded_mix.py`: verifica señal, ausencia de saturación en la muestra y silencio de las grabaciones. No sustituye escuchar y ajustar volumen con las personas y auriculares reales.

Los informes se guardan en Saved/AgentReports fuera de Git. Consultar ESTADO.md para resultados efectivos; esta especificación no convierte por sí sola una función en verificada.
