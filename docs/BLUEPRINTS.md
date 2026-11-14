# Procedimiento para tareas con Blueprint

El objetivo es que el agente complete las operaciones que sus herramientas permiten y que cualquier intervención humana sea corta, precisa y verificable. Los nombres de assets del diseño propuesto no deben confundirse con assets existentes.

## Antes de entregar instrucciones manuales

1. Identificar asset real, ruta completa de Unreal, clase padre, versión del motor y estado actual.
2. Revisar si se puede resolver con propiedades expuestas, una API de editor soportada, una utilidad ya instalada o C++ expuesto a Blueprint.
3. Usar el editor visual si está disponible y la operación puede verificarse; proteger cambios sin guardar.
4. Completar archivos, lógica y pruebas independientes antes de pedir intervención.

No generar un `.uasset` de texto, pegar secuencias de nodos inventadas ni afirmar que un grafo compila sin abrirlo/compilarlo.

## Contenido obligatorio de una guía manual

- **Objetivo y resultado esperado:** qué hará la persona y cómo reconocerá el éxito.
- **Asset y contexto:** ruta `/Game/...`, clase padre y si hay que crearlo o editarlo; indicar explícitamente si el nombre es propuesto.
- **Componentes:** nombre, tipo, jerarquía, transformaciones y propiedades que cambian.
- **Variables:** nombre exacto, tipo, unidad, valor inicial y exposición.
- **Eventos y funciones:** nombre que se busca en la versión instalada y panel donde se encuentra; no traducir libremente nombres de nodos.
- **Conexiones:** cada enlace de ejecución y de datos con nodo, pin de origen, nodo y pin de destino. Describir ramas, estado falso/verdadero y valores.
- **Configuración externa:** input mapping, modo de juego, posesión, colisión, plugin y mapa si intervienen.
- **Compilar y guardar:** asset concreto y mensajes esperados; explicar cómo reconocer errores.
- **Prueba:** acciones ordenadas, resultado visible y caso de fallo/desconexión relevante.
- **Reversión:** valores anteriores o respaldo/commit para recuperar el estado sin borrar trabajo ajeno.

Si se necesita una captura, pedir solo el panel o grafo relevante. No descargar todo el trabajo sobre el usuario con «conecta los nodos necesarios».

## Ejemplo de criterio, sin implementación implícita

Para una acción de reversa, la guía debe indicar dónde entra el evento, de dónde se lee la velocidad longitudinal, qué umbral comprobado se usa, qué ocurre si el vehículo se mueve y qué función establece la marcha. Debe especificar cómo se muestra la marcha y probar tanto pulsación como botón sostenido. No basta mencionar un nodo «Reverse» genérico ni asumir una clase de movimiento que no existe en el proyecto.

## Evidencia de cierre

Registrar ruta del Blueprint, compilación, ejecución observada y si se probó en ejecutable. Una captura de nodos prueba estructura visible, no respuesta del G920 ni funcionamiento en el visor.
