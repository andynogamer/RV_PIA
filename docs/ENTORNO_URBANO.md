# Ampliación a conducción urbana

Solicitud vigente del usuario del 14 de noviembre de 2026: ampliar sustancialmente el área manejable, comenzar estacionado e incorporar vehículos con comportamiento autónomo, semáforos y señales. Los pitidos y demás sonidos deben responder a situaciones concretas cuando se integre audio.

## Resultado esperado

El usuario comienza con el vehículo detenido junto a la acera, libera el freno de mano, observa el tráfico y se incorpora. Puede recorrer varias calles y cruces, detenerse en rojo o ante señales y completar un recorrido o volver a estacionar. La simulación mantiene su propósito de familiarización gradual, sin competición.

## Primera implementación

- Nuevo mapa urbano separado del circuito de 40 m, conservando su utilidad de entrenamiento y regresión.
- Área amplia dentro de la ciudad y recuperación segura fuera de sus límites; colisiones comprobadas en edificios próximos y suelo.
- Tráfico ligero que sigue carriles definidos, acelera/frena de forma gradual, conserva distancia y se detiene ante el jugador u obstáculos.
- Ciclo semafórico con verde, ámbar y separación en rojo entre movimientos incompatibles. No dar verde simultáneo a los dos ejes del cruce.
- Inicio estacionado con freno aplicado, instrucciones de incorporación y objetivos urbanos.
- Eventos de claxon con motivo, espera y cooldown; sin audio ficticio ni pitidos repetidos por detenerse correctamente en rojo.

Los vehículos pueden usar inicialmente rutas guiadas por splines y carrocerías reutilizadas. Esto representa tráfico autónomo de prototipo, no una simulación completa de conductores humanos ni dinámica Chaos para cada automóvil.

La primera versión tenía cuatro carriles y 12 agentes en una zona de 770 × 760 m. Su recorrido guiado medía 245 m y conectaba 36 lentes en dos cruces. La ampliación vigente enlaza las 150 lentes de la ciudad, prolonga la avenida y adelanta la bahía de destino para dejar libre la línea de detención. El recorrido guiado actual mide unos 238 m.

Los eventos de claxon ahora tienen audio. Se agregó un marcador de evaluación para el ALTO de salida y reglas educativas de velocidad, rojo y otras condiciones, con avisos y puntuación; las demás señales decorativas no se convierten automáticamente en reglas. El tráfico añade seguimiento adaptativo, reacción al verde, espera de espacio de salida y luces de freno. Ver SONIDO_Y_RETROALIMENTACION.md, DESARROLLO.md y ESTADO.md para límites y pruebas.

## Evolución posterior

Rutas conectadas con giros, selección de destinos, cesión de paso al incorporarse, luces de freno/direccionales, densidad regulable, distintos tipos de vehículo y maniobras de estacionamiento. Los pitidos se conectarán a obstrucciones prolongadas en verde o situaciones justificadas, con frecuencia e intensidad moderadas.

La señalización debe ser congruente dentro del entorno. No afirmar que replica íntegramente reglamentos de una jurisdicción sin definirla y comprobarlos. El escenario visual de Toronto se usa como base de práctica.

## Validación

Comprobar en ejecución: partida inmóvil, liberación y salida; trayectos mayores que el circuito inicial; circulación por carril; parada y reanudación de IA ante rojo; seguimiento sin superposición; reacción al jugador; pausa conjunta; claxon como evento sin spam; reinicio seguro y conservación del circuito original. Separar resultados probados de funciones todavía previstas en ESTADO.md.

La integración de RV, G920, hápticos y sonido sigue siendo indispensable y no se da por completada al ampliar el tráfico.


## Distrito Norte y Sierra del Pino

En la ciudad, sigue la avenida hacia el norte, pasa el letrero del distrito y continúa hasta el ensanchamiento para dar vuelta. El recorrido guiado corto también sigue disponible. El distrito añade 520 m de avenida, banquetas, luminarias, naves industriales y árboles. Dos de los cuatro carriles de tráfico se prolongan hasta allí.

En el menú inicial, **Cambiar a carretera** lleva al apartadero de Sierra del Pino. La carretera de doble sentido forma un circuito de 3,09 km con curvas, desniveles, acotamientos, defensas metálicas y un apartadero para completar la vuelta. Circulan diez vehículos guiados. Las texturas del terreno y el cielo fotográfico proceden de Poly Haven bajo CC0. [Terreno de Rob Tuytel](https://polyhaven.com/a/aerial_grass_rock) y [cielo de Greg Zaal y Jarod Guest](https://polyhaven.com/a/kloofendal_48d_partly_cloudy_puresky).

Para regenerar la geometría, ejecuta Blender 3.6 con `--background --python Scripts/Blender/build_roads.py`. Desde Unreal y fuera de Play, ejecuta `Scripts/Editor/import_roads.py`, `build_road_maps.py` y `bind_city_signals.py` en la ciudad. Ejecuta después `build_road_maps.py` con el argumento JSON `{"highway":true}` para la carretera. Los mapas y materiales guardados en Content ya funcionan sin ejecutar estos pasos.
