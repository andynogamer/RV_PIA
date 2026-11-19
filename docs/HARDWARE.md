# Integración del hardware

El 17 de noviembre el usuario confirmó el G920 con pedales conectado y la incorporación de un Driving Force Shifter. Windows detectó el volante como `G920 Driving Force Racing Wheel for Xbox One` (`046D:C261`), con controlador Microsoft 10.0.26100.9818 en el dispositivo USB. Inicialmente DirectInput y GameInput carecían de lecturas. Se instaló Logitech G HUB con WinGet. La base pasó a `046D:C262`. DirectInput pudo adquirirla y GameInput devolvió dirección, acelerador, freno, embrague y posición de palanca por nombre. La salida inicial era cero. En la prueba posterior con posiciones conocidas, Windows.Gaming.Input registró N y 1–6, incluida quinta con el botón físico 17 (numeración desde uno). Para repetir el diagnóstico, comprueba el nombre del dispositivo y los valores en G HUB y en el simulador.

## Logitech G920 y Driving Force Shifter

1. Sujeta volante, palanca y pedales. Conecta los pedales y la palanca a la base del G920 según el fabricante. Comprueba alimentación, USB y reconocimiento en G HUB. Mantén libre el volante durante su calibración automática. [Guía de conexión de Logitech](https://support.logi.com/hc/en-001/articles/4404187621911-Troubleshoot-G923-G29-G920-for-connection-Issues)
2. En Windows, pulsa **Win + R**, escribe `joy.cpl` y abre **Propiedades** del G920. Comprueba que el volante y los tres pedales cambian de valor por separado. Recorre las siete posiciones de la palanca y vuelve a neutral. Si falta el G920 o no cambian lecturas, comprueba primero el controlador y el montaje con la guía anterior.
3. En Unreal 5.6.1 abre `L_CiudadUrbana`, pulsa **Play** y elige **Comenzar práctica**. El HUD debe pasar a **G920** y mostrar las barras de embrague, freno y acelerador. La lectura automática de Windows.Gaming.Input usa ejes con nombre y posiciones de palanca. Para crear un perfil DirectInput alternativo, con el auto estacionado pulsa **J**. Sigue el texto del HUD: neutral con todo suelto y volante centrado, extremos izquierdo y derecho, acelerador, freno, embrague, marchas 1 a 6, R y vuelta a neutral. Mantén el control en la posición indicada al pulsar **Enter** en cada paso. Durante la calibración solo, para registrar el enclavamiento R de fábrica oprime la palanca hacia abajo y llévala a la posición de sexta. Para cancelar, pulsa **J** otra vez. [Driving Force Shifter](https://www.logitechg.com/en-us/shop/p/driving-force-shifter)
4. Si completas la calibración manual, el perfil tiene prioridad sobre el modo automático y se guarda localmente en `GameUserSettings.ini`. Pulsa **X** en el G920 para liberar el freno de mano al estar detenido. En teclado, pulsa y suelta **Espacio**. Pisa el embrague a fondo, selecciona **1**, suelta el embrague de forma gradual y acelera suavemente. El HUD superior indica la marcha seleccionada (**N**, **1–5** o **R**). El indicador de la cabina muestra la marcha acoplada y pasa a **N** al pisar el embrague a fondo. Para retroceder, detente, pisa el embrague y lleva la palanca a la posición marcada **6** sin hundirla. El enclavamiento R de fábrica también selecciona reversa. Si la marcha llegó antes que el embrague, mantén la posición y pisa a fondo. Si intentaste invertir el sentido en movimiento, detente y pasa por N antes de reintentarlo.
5. Prueba pausa, reinicio, pérdida de foco y desconexión a baja velocidad en un espacio libre. Debe haber acelerador cero y marcha neutral al perder entrada. Para rearmar, suelta el acelerador. Puedes usar N o, estando detenido en una marcha, pisar el embrague a fondo. **K** alterna entre teclado y G920 con el vehículo detenido. Repite **J** si cambias el rango de giro o el controlador.

El modo automático usa RacingWheel de Windows.Gaming.Input con el SDK de Windows elegido por Unreal. Comprueba la identidad de la base y espera una lectura con marca temporal. En la cabina, **Palanca** indica la posición física y el indicador de marcha conserva la selección aceptada por el simulador. El modo de calibración manual utiliza DirectInput en primer plano y aprende los índices, signo y recorrido de las muestras. El perfil requiere cuatro ejes independientes y siete botones de marcha distintos. El cambio de marcha por palanca usa solo lecturas, sin enviar fuerza. [Lectura inmediata de joystick con DirectInput](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee418260%28v%3Dvs.85%29)

### Probar solo la palanca

Pausa la práctica y abre `Scripts/Probar-Palanca.cmd` con doble clic. Deja la ventana de prueba en primer plano y recorre N, 1–6 y R. Puedes dejar los pedales sueltos. El número grande es la posición que comunica la base. La lectura física **6** corresponde a **R** en el juego. Debajo aparecen los botones activos y las posiciones ya observadas. Un guion indica que todavía falta una lectura válida. En esta máquina se observó que una aplicación recién abierta puede empezar con valor cero y marca temporal cero aunque la palanca esté en una marcha.

Las transiciones se guardan en `Saved/AgentReports/prueba_palanca.json`. En Unreal, compara **Palanca** con la marcha del coche. Para engranar, inicia en N con el acelerador suelto, pisa el embrague a fondo y selecciona la marcha. [Lecturas de RacingWheel en Windows](https://learn.microsoft.com/en-us/windows/uwp/gaming/racing-wheel-and-force-feedback)

## Visor prestado

El equipo dispone de un **Meta Quest 3** prestado. El 19 de noviembre Windows tenía el servicio Meta Link activo y Unreal inicializó OpenXR con el runtime Oculus 1.208.0. El usuario vio el menú anterior recortado en el ojo derecho y confirmó que pudo entrar en la cabina con la corrección.

Vía recomendada para este proyecto: Windows PCVR mediante Meta Horizon/Quest Link y OpenXR; mantener G920 y Arduino conectados al PC. Para las primeras pruebas se propone cable compatible, sujeto a disponibilidad y comprobación; la conexión inalámbrica queda como alternativa. No se cambia el destino a Android autónomo por identificar Quest 3.

Para situar el **límite físico**, siéntate en tu silla frente al volante y, en las Quest, abre **Controles rápidos → Configuración → Configuración del entorno → Límite → Crear nuevo límite → Estacionario**. Si el menú de las Quest mira hacia otra dirección, apunta el mando derecho hacia delante y mantén pulsado el botón Meta hasta que la vista se centre. [Límite estacionario de Meta](https://www.meta.com/help/quest/463504908043519/) y [recentrado de la vista de Meta](https://www.meta.com/help/quest/149215193811647/).

Para situar el **asiento virtual**, vuelve a Link, siéntate en postura de conducción y mira hacia el centro del volante físico. **A** en **Comenzar práctica** centra la posición de los ojos y alinea la mirada con el frente del coche. Durante la práctica puedes repetirlo con **Y** estando detenido o en pausa. Si el visor pierde el seguimiento antes de empezar, el menú espera a que te lo pongas y vuelvas a confirmar.

En inicio y pausa, la cruceta **↑ / ↓** selecciona, **A** confirma y **B** vuelve. En cabina, **☰** abre la pausa. Para salir estacionado, deja la palanca en N y el acelerador suelto, pulsa **X** para liberar el freno de mano y pisa el embrague para engranar primera. La versión de desarrollo admite también teclado. Windows expuso en este G920 las etiquetas de navegación Xbox A/B, Menú y Xbox X/Y a través de `UINavigationController`. [Navegación con volantes en Windows](https://learn.microsoft.com/en-us/windows/uwp/gaming/racing-wheel-and-force-feedback).

La configuración actual ya habilita OpenXR. Se observó una NVIDIA GeForce RTX 3060 Ti mediante inventario local; no se ha medido rendimiento de esta escena en RV ni se garantiza una frecuencia por el nombre de la GPU. Fuentes oficiales consultadas el 14 de noviembre: [PCVR y cámaras en Unreal, Meta](https://developers.meta.com/vr/documentation/unreal/unreal-engine-basics/) y [requisitos OpenXR UE 5.6, Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/openxr-prerequisites-in-unreal-engine?application_version=5.6).

Para la primera sesión por cable, instala Meta Horizon Link en Windows y conecta el visor encendido con un cable de datos USB 3 de al menos 5 Gbit/s. Dentro del visor, pulsa el botón Meta del mando derecho, abre los controles rápidos desde la barra de estado y elige **Link → tu PC → Iniciar**. La primera comprobación es ver el entorno Link dentro del visor. [Descarga para Windows](https://www.meta.com/quest/setup/) y [guía de conexión de Meta](https://www.meta.com/help/quest/509273027107091/).

Comprobaciones de la sesión en Unreal

1. Confirmar que el visor puede ejecutar la experiencia elegida y cómo recibe imagen/tracking desde Windows.
2. Preparar el runtime OpenXR adecuado siguiendo al fabricante. Comprobar runtime activo antes de interpretar errores de Unreal.
3. Verificar que VR Preview está disponible y se ve realmente dentro del visor; una imagen duplicada en monitor no basta.
4. Ajustar postura sentada, altura de cámara, escala y recentrado dentro de la cabina.
5. Medir tiempos de CPU/GPU y fluidez con el recorrido real; fijar presupuesto de fotograma conforme a la frecuencia elegida del visor, no a un número genérico.
6. Validar salida rápida, pausa y prueba del ejecutable que se usará en la revisión.

## Retorno al centro y choque en el G920

El conductor sintió el golpe al chocar, pero el aro quedó suelto desde el comienzo de la práctica. Logitech indica que, cuando el juego toma el control de la fuerza, le corresponde generar el retorno al centro. El simulador carga un resorte simétrico sobre el eje X al recibir una lectura física válida en conducción. Para activarlo al entrar o reanudar, deja la palanca en N, suelta el acelerador y acerca el aro al centro. El efecto de choque funciona de forma independiente. [Comportamiento del resorte en G HUB](https://support.logi.com/hc/en-ca/articles/15583220063255-The-force-feedback-on-my-wheel-has-changed-and-I-can-no-longer-use-the-center-spring-force-setting-in-G-HUB-to-adjust-the-strength) y [parámetros del resorte en Windows](https://learn.microsoft.com/en-us/uwp/api/windows.gaming.input.forcefeedback.conditionforceeffect.setparameters).

La ganancia inicial del resorte es 0,25. La variable `rv.G920.CenteringGain` permite ajustarla entre 0 y 0,35. El juego detiene el resorte al pausar, perder el foco o cambiar de modo. Un temporizador independiente comprueba cada 50 ms si han pasado 300 ms sin actualizar la conducción. La base aceptó cargar el resorte y la onda a la vez, detenidos. Para comprobar la sensación, gira un poco el aro a cada lado con el coche parado y en N, prueba la pausa con ☰ y vuelve al centro antes de reanudar.

Windows enumeró el G920 conectado con motor habilitado y eje de fuerza X. El controlador aceptó cargar y descargar una onda senoidal en estado detenido. En conducción con una muestra física válida, el juego inicia en el motor un pulso de 150 ms a 20 Hz cuando detecta un golpe lateral o frontal significativo. Distingue impactos sucesivos de los contactos repetidos de la física. El suelo y los avisos de tráfico tienen salida cero. La ganancia inicial del efecto es 0,15, configurable en `rv.G920.CollisionGain` entre 0 y 0,25. El juego respeta la ganancia maestra de G HUB. La pausa, el cambio de modo, la pérdida de entrada y el cierre cancelan el efecto. La duración está programada también en el dispositivo. [Force feedback con Windows.Gaming.Input](https://learn.microsoft.com/en-us/windows/uwp/gaming/racing-wheel-and-force-feedback).

Para comprobar el resultado físico, con el G920 activo y la base sujeta, prueba un contacto a baja velocidad en el simulador. Comprueba que se siente un golpe breve, que el aro deja de vibrar y que otro contacto separado puede volver a producirlo. El registro indica el inicio del efecto si el controlador acepta la orden.

## Arduino y hápticos externos

La placa está disponible según el usuario; modelo, motores, driver, fuente y circuito pendientes. No asumir que cualquier Arduino puede presentarse como dispositivo USB HID. Serial USB es un candidato; decidir según placa y latencia observada.

La simulación ahora expone `OnFeedback` con motivo y solicitud de pulso lógico normalizado de 0.15 s, y `OnFeedbackStopped` para cancelar al pausar, reiniciar o terminar. El backend del G920 se describe arriba. La salida hacia Arduino permanece desactivada. Su implementación futura debe traducir y limitar esa solicitud después de revisar el montaje; no se debe usar el valor como PWM directo ni dejar una salida sostenida hasta el siguiente evento. Ver SONIDO_Y_RETROALIMENTACION.md.

Definir antes de escribir firmware final:

- Entradas opcionales: pausa física si el montaje lo permite. La reversa se selecciona con el Driving Force Shifter.
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
