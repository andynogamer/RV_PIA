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

- Dirección izquierda/derecha con centro estable; pedales sueltos sin aceleración; aceleración/frenado previsibles.
- Reversa solo desde condición segura; botón sostenido no alterna marcha continuamente.
- Pausa, pérdida de dispositivo, reinicio y salida llevan entrada y hápticos a estado seguro.
- Instrucciones legibles, actividad guiada completada y sonido ambiente audible sin tapar indicaciones.

## Ampliación urbana en escritorio

- Ejecutar `audit_urban_roads.py`: soporte de suelo y ausencia de obstáculos estáticos con volumen de tráfico a lo largo de los carriles. No sustituye recorrer otras calles o medir suspensión en bordes.
- En PIE urbano y de forma secuencial, ejecutar `test_urban_traffic.py`, `test_traffic_recycling.py` y `test_urban_route.py`. Esperar su JSON de resultado antes de iniciar otra prueba. Comprobar fecha y `passed=true`; un resultado anterior no valida cambios nuevos.
- El test de ruta usa física del jugador y desactiva IA temporalmente. El test de tráfico verifica reacción a señales y al jugador por separado; no afirmar por esa combinación que se validaron todas las situaciones de una ciudad real.
- Confirmar en imagen: inicio estacionado, señalización visible, orientación de autos y lámparas del semáforo coherentes con su fase.
- Volver a `L_CiudadPractica` y ejecutar `test_driving.py` como regresión tras cambios compartidos. Conservar mapa y assets originales.
- Claxon: distinguir el evento emitido del sonido audible; hasta conectar y escuchar audio, registrar solo el evento como verificado.

## RV y hardware reales

- Visor: runtime identificado, imagen estéreo y tracking reales, postura sentada, recentrado, ausencia de movimiento artificial de cabeza obligatorio.
- Rendimiento: frecuencia objetivo del visor y tiempos CPU/GPU medidos durante la ruta; declarar equipo, calidad y condiciones.
- G920: calibración, mapeo de cada eje/botón y desconexión/reconexión observados.
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
