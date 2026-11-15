# Procedencia y manejo de recursos

No se ha otorgado una licencia general a este proyecto. Registrar autor, URL de origen, licencia o comprobante aplicable, restricciones de redistribución, cambios y crédito requerido antes de publicar recursos de terceros.

## Ciudad

Ubicación en proyecto: `/Game/Ciudad/Downtown_YongeDundas/`. Hay 1,194 archivos de mallas estáticas, 99 de materiales y 124 de texturas. El registro de importación refiere una fuente local GLTF/GLB; no demuestra su licencia.

Se verificó la carpeta externa `C:/Users/T450SCMPTRC/Desktop/downtown-toronto-yonge-and-dundas/`. En `escenario-asset/` existen GLTF, GLB, BIN, Blender y texturas. El GLTF declara una escena, 1,832 nodos, 1,194 mallas, 99 materiales y 91 imágenes. El generador declarado es Khronos glTF Blender I/O v3.6.27. Tiene jerarquía y nodos raíz: es candidato para recuperar la distribución de la ciudad mediante importación de escena, aún sin probar.

`unreal.zip` contiene 99 entradas, incluyendo GLTF/GLB y material asociado; no se encontró `.umap` ni `.uproject` en su listado. No es evidencia de un mapa Unreal ya ensamblado.

No se localizaron archivos de licencia, créditos o README en esa carpeta. Autor, plataforma de adquisición, URL y condiciones siguen pendientes del equipo. No se infiere permiso de redistribución por tratarse de una tarea escolar.

Las fuentes externas no se copiaron a este repositorio. El commit inicial preserva lo que ya existe en Content; no preserva por sí solo esas fuentes ni su distribución original fuera de Unreal. Antes de importarlas, conservar respaldo privado y documentar una ubicación reproducible según sus condiciones de uso.

## Plantillas y recursos de Unreal

Hay contenido bajo `VRTemplate`, `VRSpectator`, `Characters`, `LevelPrototyping` y `Weapons`. Se conserva para evitar romper referencias durante el diagnóstico. No incluir armas ni recursos ajenos al entrenamiento en la experiencia final por el hecho de estar disponibles. Revisar referencias antes de retirar recursos en una tarea futura.

## Audio, vehículo y publicidad

Se incorporaron dependencias oficiales de `UE_5.6/Templates/TP_VehicleAdvBP` y sus packs compartidos Vehicles/Input. Destinos: `/Game/VehicleTemplate`, `/Game/Vehicles`, `/Game/Input`. Se copiaron paquetes existentes preservando referencias, sin modificar los originales del motor. No se añadieron mapas ni variantes de competición. La fuente de referencia es la [plantilla oficial de vehículos UE 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/vehicle-template-in-unreal-engine?application_version=5.6); sus recursos siguen sujetos a los términos de Epic, sin relicenciarlos como contenido libre.

La cabina y el volante son arte original generado mediante `Scripts/Blender/build_cockpit.py`; fuentes bajo `SourceArt/Cockpit` y resultados en `/Game/RutaVirtual/Vehicles/Interior`. Los conos y carteles reutilizan formas básicas del motor con materiales propios. No se descargaron assets externos en esta implementación.

La ampliación urbana reutiliza `SM_SportsCar`, cristales, ruedas y materiales del pack oficial ya instalado para los agentes de tráfico. El mapa nuevo utiliza copias de 227 mallas originales de ciudad en `/Game/RutaVirtual/Environment/UrbanCollision` con colisión compleja; no se alteran los originales ni su titularidad. Las lámparas de los dos cruces cambian de material en ejecución mediante un material unlit propio; postes, cabezales y lentes conservan sus mallas originales. Se retiraron del mapa nuevo las piezas de vehículos decorativos; los assets siguen disponibles. El uso académico privado no otorga una licencia general para publicar esos recursos.

Audio: seis WAV originales generados por síntesis matemática con `Scripts/Audio/generate_audio.py`, sin grabaciones descargadas. Fuentes y manifiesto en `SourceAudio/`; paquetes importados en `/Game/RutaVirtual/Audio/`. Son timbres de prototipo de motor, rodadura, ambiente, claxon, impacto y reconocimiento. Los WAV se versionan por LFS. No atribuirlos a un fabricante ni describirlos como grabaciones reales. La grabación de la mezcla de Unreal se guarda como evidencia derivada fuera de Git.

Publicidad nueva sigue pendiente. Cada incorporación debe registrar origen, autorización, modificaciones y crédito. El equipo debe distinguir material propio, de terceros y generado con asistencia de IA según las reglas académicas aplicables, sin inventar una política institucional.

## Registro mínimo por incorporación

Identificador; carpetas/assets; nombre del recurso; autor; enlace de origen; fecha de obtención; licencia/comprobante; permiso de distribución del ejecutable y de las fuentes por separado; texto de atribución; modificaciones; responsable y estado de revisión.

Los comprobantes con datos personales se conservan fuera del repositorio público. Antes de compartir un remoto, revisar también logos, anuncios y otras imágenes incorporadas en el escenario.
