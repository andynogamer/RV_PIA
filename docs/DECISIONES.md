# Registro de decisiones

Registrar decisiones que cambien el alcance, dependencias o forma de implementar. Una propuesta se acepta solo al contar con la autorización o evidencia pertinente. Las entradas siguientes reflejan la sesión del 14 de noviembre de 2026.

## D-001 — Entrenamiento introductorio

Estado: vigente. Fuente: propuesta y objetivo del usuario. RUTA VIRTUAL ofrece familiarización con conducción; no promete tratamiento, diagnóstico ni eficacia clínica. Consecuencia: prácticas guiadas y retroalimentación descriptiva, sin afirmaciones médicas en interfaz o publicidad.

## D-002 — G920 comercial y Arduino complementario

Estado: decisión del usuario. El profesor autorizó comprar Logitech G920. Se reemplaza la premisa de construir todo el volante descrita en el primer avance. Arduino puede añadir vibración o botón de reversa; circuito y alcance pendientes. Documentar exactamente la aportación del equipo y no atribuirle fabricación del G920.

## D-003 — Conservar Unreal 5.6

Estado: base vigente. Descriptor 5.6 y registro de sesión 5.6.1. No migrar automáticamente a otra versión instalada. Consecuencia: validar plugins, API y toolchain para esta versión.

## D-004 — Windows PCVR como punto de partida

Estado: propuesta provisional, sujeta al visor. Justificación: volante USB y Arduino en PC. No se ha definido ejecución autónoma Android. Consecuencia: identificar visor y conexión antes de cerrar plataforma, runtime y empaquetado.

## D-005 — Lógica automatizable y composición Blueprint

Estado: criterio de trabajo. Maximizar implementación por agentes con C++/configuración/herramientas verificables, usar Blueprint para composición y parámetros, sin conversión masiva obligatoria. C++ se incorpora cuando el toolchain esté probado. Python es herramienta de editor, no gameplay del ejecutable.

## D-006 — Circuito corto antes de ciudad completa

Estado: primera etapa cumplida; ampliada por D-010 por solicitud posterior del usuario. Se resolvió un circuito corto y se conservará como entrenamiento y regresión. La comprobación en Unreal mostró que las transformaciones de ciudad ya estaban incorporadas en las mallas importadas, por lo que no fue necesario reimportar su fuente GLTF.

## D-007 — Git local y LFS

Estado: preparación autorizada por el usuario. Versionar proyecto y assets existentes; excluir cachés y evidencia privada. No crear repositorio público ni subir contenido sin definir destino y alcance. No aplicar licencia de software libre a assets ajenos de procedencia pendiente.

## D-008 — Fecha y revisión

Estado: fecha objetivo, tipo de revisión pendiente. El usuario dijo «jueves» el sábado 14 de noviembre de 2026; se interpreta como 19 de noviembre, America/Mexico_City. Hasta aclararlo, el plan contempla requisitos de la final. No presentar esta interpretación como fecha escrita en la rúbrica, cuyas revisiones aparecen por anunciar.

## D-009 — Primer prototipo de escritorio

Estado: implementado el 14 de noviembre de 2026, autorizado por el usuario al iniciar desarrollo. Reutilizar la plantilla Chaos instalada de UE 5.6 y complementar con C++ y arte de cabina generado en Blender. Conservar ciudad y sala originales; ensamblar mallas con transformaciones ya incorporadas en un mapa propio. Acotar un corredor de 40 m mientras se desarrollan colisiones del resto del entorno.

Las pruebas detectaron la reversa automática del freno de plantilla; se desactivó y se agregó selector R con condición de parada y acelerador liberado. El reinicio usa teleport físico y reinicializa el estado asíncrono de Chaos. Se autoriza reiniciar el editor dentro del desarrollo tras comprobar/guardar paquetes, sin descartar trabajo.

El usuario permite recursos externos para uso privado académico. Para esta implementación se usaron recursos oficiales ya instalados y arte original; no se descargaron recursos nuevos ni se hizo push. El remoto `origin` fue configurado por el usuario entre sesiones.

## D-010 — Playground urbano y tráfico autónomo ligero

Estado: primera implementación verificada en editor el 14 de noviembre de 2026. Fuente: petición expresa del usuario de ampliar el playground, comenzar estacionado y sumar IA, señales, semáforos y pitidos contextuales al integrar sonido. Amplía la priorización D-006 sin eliminar los requisitos de RV, G920 y háptico.

Se mantiene separado `L_CiudadUrbana`, con límites de 770 × 760 m, ruta guiada de unos 245 m y tráfico cinemático sobre cuatro splines. Se clonaron 227 mallas para colisión compleja sin alterar los originales y se retiraron del mapa urbano las piezas de autos decorativos que ocupaban los carriles. Los 12 agentes reutilizan carrocerías oficiales instaladas. Dos cruces usan verde/ámbar y separación en rojo para movimientos incompatibles.

La decisión de movimiento cinemático permite tráfico ligero controlable antes de simular cada vehículo con Chaos. No incluye aún giros entre rutas, cesión completa, cambios de carril o conductores humanos realistas. Los claxones se exponen como eventos con espera y cooldown; audio pendiente. La recirculación se protege por ocupación de carril y distancia al jugador, sin afirmar invisibilidad de la reaparición.

La validación detectó bloqueo mutuo entre el inicio de una ruta y el final del carril contrario; se corrigió midiendo ocupación dentro del carril. La clonación segura del mapa usa `new_level_from_template`; la duplicación directa de World provocó un fallo del editor y no se conserva como procedimiento. Ver ESTADO.md para evidencia y límites.

La inspección visual detectó lámparas decorativas fijas que contradecían el controlador. Se enlazaron 36 lentes originales (24 sur, 12 norte), se ocultaron los postes auxiliares y se ubicó el controlador norte en el cruce real del modelo. Las pruebas comprueban ahora correspondencia entre fase lógica y color visible. Los demás semáforos del escenario se mantienen como decoración y requieren integración si se amplían las rutas.

## D-011 — Inicio y pausa con interfaz de escritorio

Estado: implementación del 14 de noviembre de 2026, solicitada por el usuario para entrar con el ratón dentro del juego y disponer de menú inicial. Se agrega UMG nativo, sin modificar los mapas/Blueprints existentes. El controlador separa Inicio, Conducción y Pausa; los menús detienen el mundo y la conducción captura la entrada de forma explícita. El foco y posición del cursor se establecen después del layout inicial del viewport para evitar las dimensiones provisionales de PIE.

La configuración predeterminada de PIE activa GameGetsMouseControl y se actualizó también la preferencia local de esta computadora, con copia previa fuera de Git. La interfaz sigue siendo plana; no se presenta como menú de RV validado. Las APIs se contrastaron con los headers instalados de UE 5.6, la compilación y las referencias oficiales de [FInputModeGameOnly](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FInputModeGameOnly?application_version=5.6) y [UUserWidget](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/Blueprint/UUserWidget?application_version=5.6). Ver ESTADO.md para resultados.

## Registro de decisiones futuras

Identificador y título; fecha; estado; problema; evidencia/fuente; decisión; consecuencias; validación; alternativa relevante si explica el costo; condición de revisión. Evitar abrir un registro nuevo por cambios triviales.
