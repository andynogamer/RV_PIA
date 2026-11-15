# Integración del hardware

Este documento define el procedimiento previsto. No contiene calibraciones reales ni prueba de conexión realizada.

## Logitech G920

El usuario cuenta con autorización del profesor para comprarlo y está pendiente de recibirlo. Se usará en PC con volante y pedales. Primero integrar dirección, acelerador y freno; embrague y cambio manual completo quedan fuera del mínimo.

Al recibirlo:

1. Sujetar volante y pedales, revisar conexiones y alimentación según el fabricante. Mantener libre el volante durante su calibración automática.
2. Confirmar detección en Windows y software oficial. Anotar driver/software, VID/PID observados y rango de giro configurado.
3. Verificar ejes y botones sin Unreal: reposo, centro y extremos. Registrar acelerador/freno independientes, inversión y ruido.
4. Activar y probar RawInput en una escena de diagnóstico. Identificar los índices reales y traducirlos a acciones lógicas; no confundir índice de dispositivo, posición de array y Generic USB Controller Axis.
5. Calibrar dirección y pedales en sus rangos normalizados; verificar valor cero en pedales sueltos y ausencia de aceleración al iniciar.
6. Integrar el vehículo, comprobar frenado y reversa primero a velocidad baja.
7. Integrar y probar fuerza como tarea separada. Un volante que se centra por configuración del driver no demuestra fuerza producida por la simulación.
8. Desconectar y reconectar durante una prueba controlada: acelerador cero, aviso y salida de fuerza cero; requerir confirmación/calibración para reanudar si el estado es incierto.

La documentación de [RawInput 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/rawinput-plugin-in-unreal-engine?application_version=5.6) incluye un ejemplo con G920. Úsalo como referencia; conserva el registro de lo medido en este dispositivo. Para fuerza, consultar el [SDK oficial Logitech](https://www.logitechg.com/en-us/programs/partner-developer-lab), revisar requisitos y compatibilidad antes de integrar. No se ha descargado ni instalado aquí.

## Visor prestado

Visor previsto: **Meta Quest 3**, indicado como probable por el usuario el 14 de noviembre de 2026. Falta confirmar préstamo, fecha, controles disponibles y forma de conexión. No registrar el dispositivo como conectado ni probado.

Vía recomendada para este proyecto: Windows PCVR mediante Meta Horizon/Quest Link y OpenXR; mantener G920 y Arduino conectados al PC. Para las primeras pruebas se propone cable compatible, sujeto a disponibilidad y comprobación; la conexión inalámbrica queda como alternativa. No se cambia el destino a Android autónomo por identificar Quest 3.

Siguiente hito propuesto: cámara sentada con mirada independiente y recentrado; menú/HUD espacial con entrada accesible sin ratón; pausa y retorno seguros; luego medición en visor y calibración del volante. El menú actual y las pruebas de monitor no validan esas funciones.

La configuración actual ya habilita OpenXR. Se observó una NVIDIA GeForce RTX 3060 Ti mediante inventario local; no se ha medido rendimiento de esta escena en RV ni se garantiza una frecuencia por el nombre de la GPU. Fuentes oficiales consultadas el 14 de noviembre: [PCVR y cámaras en Unreal, Meta](https://developers.meta.com/vr/documentation/unreal/unreal-engine-basics/) y [requisitos OpenXR UE 5.6, Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/openxr-prerequisites-in-unreal-engine?application_version=5.6).

Cuando se identifique:

1. Confirmar que el visor puede ejecutar la experiencia elegida y cómo recibe imagen/tracking desde Windows.
2. Preparar el runtime OpenXR adecuado siguiendo al fabricante. Comprobar runtime activo antes de interpretar errores de Unreal.
3. Verificar que VR Preview está disponible y se ve realmente dentro del visor; una imagen duplicada en monitor no basta.
4. Ajustar postura sentada, altura de cámara, escala y recentrado dentro de la cabina.
5. Medir tiempos de CPU/GPU y fluidez con el recorrido real; fijar presupuesto de fotograma conforme a la frecuencia elegida del visor, no a un número genérico.
6. Validar salida rápida, pausa y prueba del ejecutable que se usará en la revisión.

## Arduino y hápticos externos

La placa está disponible según el usuario; modelo, motores, driver, fuente y circuito pendientes. No asumir que cualquier Arduino puede presentarse como dispositivo USB HID. Serial USB es un candidato; decidir según placa y latencia observada.

Definir antes de escribir firmware final:

- Entradas: botón de reversa con antirrebote y cambio por flanco, no por cada frame mientras esté pulsado. Posibilidad de pausa física si el montaje lo permite.
- Salida: vibración por un evento congruente de la simulación; intensidad limitada, apagada por defecto y configurable.
- Comunicación: protocolo versionado con tipo de mensaje, secuencia, longitud/límite de mensaje, unidades y rangos. Parser no bloqueante, descarte de mensajes inválidos y prevención de acumulación de comandos antiguos.
- Seguridad funcional: salida cero al iniciar, pausar, salir, perder conexión o vencer timeout. Watchdog también en firmware para que una caída del juego no deje la vibración activa.
- Prueba sin hardware: backend simulado y casos de fragmentación, mensajes inválidos, repetidos y desconexión. No llamarlo validación física.

Antes de cablear, registrar modelo de placa, actuador, driver, fuente, tensión/corriente y esquema revisado. El motor requiere una etapa de potencia apropiada; el GPIO solo proporciona señal. El equipo humano revisa y energiza el circuito.

El G920 comercial es la decisión vigente. La aportación del equipo puede incluir integración software, calibración y complemento Arduino; documentar exactamente lo construido y probado, sin describirlo como volante fabricado desde cero.

## Registro para completar con el hardware real

- G920: detección, software, VID/PID, ejes, inversión, centro/extremos y resultado de FFB.
- Visor: modelo, conexión, runtime activo, frecuencia, postura/recentrado y tiempos de CPU/GPU.
- Arduino: placa, circuito, puerto observado, versión de firmware, protocolo, latencia y timeout comprobado.
- Cada registro incluye fecha, responsable, versión del proyecto y resultado observable. Guardar evidencia técnica bajo `Saved/AgentReports/` y resumir la conclusión en `ESTADO.md`.
