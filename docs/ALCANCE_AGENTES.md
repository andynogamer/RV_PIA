# Alcances de los agentes de IA

Los agentes deben asumir el trabajo técnico que puedan ejecutar y comprobar. Su alcance depende de las herramientas disponibles en cada sesión; la capacidad de escribir una solución no prueba que haya sido integrada en Unreal o validada con el hardware.

## Trabajo que pueden realizar directamente

- Leer proyecto, configuración, documentos autorizados, registros, código e historial Git; detectar faltantes y proponer tareas concretas.
- Escribir lógica C++ de conducción, estados de sesión, rutas, controles, interfaces Blueprint y comunicación con hardware; compilarla cuando exista la cadena de herramientas.
- Crear scripts del editor para inventarios, colocación de actores, ajustes de propiedades o generación de assets cuando la API de UE 5.6 lo soporte. Verificar cada script en el motor.
- Preparar firmware Arduino, parser y protocolo, simuladores de mensajes y pruebas de desconexión, cuando se conozcan placa y circuito.
- Ajustar `.ini`, `.uproject`, módulos, empaquetado y herramientas dentro de la tarea autorizada, con cambios mínimos y reversibles.
- Crear documentación de arquitectura, guiones de prueba, materiales técnicos y borradores académicos; localizar y verificar bibliografía en fuentes originales.
- Preparar un ejecutable y revisar registros si motor, compiladores, plugins y recursos están disponibles.
- Administrar ramas y commits locales; preparar un remoto o publicación cuando el usuario lo solicite con el destino definido.

## Trabajo posible mediante Unreal Editor

No existe una prohibición general de que una IA trabaje con Blueprint. El acceso depende de una API, plugin, herramienta o interfaz gráfica que permita la operación concreta.

Python puede crear o modificar determinados assets, colocar actores y configurar propiedades expuestas. No se debe prometer construir cualquier grafo Blueprint mediante la API estándar. Para automatizar grafos puede ser necesario desarrollar una herramienta de editor C++, usar una integración ya instalada o realizar operaciones por interfaz. Su costo debe justificarlo frente a implementar la lógica directamente en C++.

Si hay control visual del editor, el agente puede inspeccionar escenas, configurar componentes y operar nodos visibles. Debe confirmar el resultado, compilar, guardar lo pertinente y comprobar el comportamiento. En esta sesión solo se inspeccionó visualmente el mapa; no se editaron assets.

## Trabajo que requiere al equipo humano

- Conectar, sujetar y calibrar volante, pedales y visor; verificar cables, alimentación y espacio de uso.
- Confirmar comodidad, orientación, escala percibida, vibración y fuerza física. La IA no puede sentir mareo o torque.
- Montar y revisar el circuito Arduino antes de energizarlo; confirmar su comportamiento real.
- Proporcionar accesos a hardware no conectado y resolver autenticación cuando corresponda.
- Confirmar criterios del profesor y responsables del equipo; presentar el proyecto y responder preguntas.
- Revisar autoría, referencias y adecuación académica. El equipo debe comprender y poder explicar el trabajo asistido por IA.

## Cómo reducir las tareas manuales

1. El agente inspecciona la base y ejecuta todo trabajo textual y automatizable de la tarea.
2. Usa APIs y herramientas existentes; no presupone una integración Unreal/MCP ausente.
3. Mantiene lógica reutilizable en C++ cuando haya compilación verificada y Blueprint para composición y ajustes visuales.
4. Si necesita una operación de interfaz, la realiza con herramientas disponibles sin descartar trabajo ajeno.
5. Si queda una intervención humana, entrega instrucciones de `docs/BLUEPRINTS.md` y continúa los trabajos independientes.

No se fija un porcentaje prometido de automatización: en código y documentos puede ser amplio; en montaje físico y validación con visor seguirá siendo indispensable el equipo.

## Roles de trabajo sugeridos

Se pueden asignar tareas de implementación Unreal, integración de hardware, revisión de escena/rendimiento y documentación/QA. Son responsabilidades, no agentes persistentes ya configurados. Crear o coordinar varios agentes solo cuando el usuario lo solicite y las herramientas lo permitan; asignar archivos distintos y un responsable por asset binario.

## Fuentes técnicas

- [Python en Unreal Editor 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python?application_version=5.6): automatización de producción; no lenguaje de gameplay del ejecutable.
- [RawInput en Unreal Engine 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/rawinput-plugin-in-unreal-engine?application_version=5.6): dispositivos, ejes y botones; ejemplo de G920.
- [SDK de volantes Logitech](https://www.logitechg.com/en-us/programs/partner-developer-lab): efectos de fuerza e integración DirectInput. Disponibilidad y licencia deben revisarse antes de incorporarlo.

Fuentes consultadas el 14 de noviembre de 2026. Estos enlaces no implican que esas integraciones estén habilitadas o probadas en RV_PIA.
