# Repositorio y colaboración

Este proyecto utiliza un repositorio Git local, rama inicial `main`, con Git LFS para paquetes binarios de Unreal y determinados formatos fuente. No se creó remoto ni se subieron archivos. Un historial en el mismo disco no protege frente a pérdida del equipo.

## Qué se conserva

`RV_PIA.uproject`, `Config/`, `Content/`, gobernanza, documentación y scripts. Cuando se añadan, también `Source/`, `Build/`, fuentes de plugins propias y firmware. No excluir `Build/` indiscriminadamente: puede contener recursos necesarios de plataforma.

`Saved/`, `Intermediate/`, `DerivedDataCache/`, binarios regenerables y datos locales no pertenecen al historial. `Content/Developers/` y `Content/Collections/` son locales; mover un asset necesario a una carpeta compartida y corregir referencias desde Unreal antes de depender de él.

Un plugin comercial/binario puede requerir conservar sus binarios y reglas particulares. No borrar ni ignorar archivos necesarios basándose únicamente en una plantilla genérica.

## Flujo por tarea

1. Revisar `git status` y guardar lo pertinente en Unreal. No incluir cambios de otra persona o tarea sin entenderlos.
2. Trabajar en una rama descriptiva si hay colaboración. Separar cambios de lógica, assets y documentación cuando facilite la revisión.
3. Acordar responsable único para cada `.umap`/`.uasset`. Un conflicto binario se resuelve eligiendo una versión o reconstruyendo en editor, nunca con merge textual.
4. Ejecutar la validación adecuada y revisar el diff de archivos textuales y lista de assets modificados.
5. Hacer commits pequeños y descriptivos. Actualizar estado y decisiones cuando correspondan.

Comandos de revisión: `git status --short`, `git diff --stat`, `git diff --check`, `git lfs ls-files`, `git log --oneline -5` y `git remote -v`.

## Git LFS

`.gitattributes` define paquetes `.uasset`, `.umap`, `.uexp` y `.ubulk` como LFS, además de algunos formatos pesados. No editar los punteros LFS manualmente.

En otro equipo: instalar Git y Git LFS, ejecutar `git lfs install`, clonar el remoto autorizado y ejecutar `git lfs pull`. Comprobar que Unreal recibe assets binarios reales, no pequeños archivos de puntero.

No se utiliza `lockable` inicialmente porque todavía no hay servidor remoto de locks. Se coordina edición exclusiva de assets por tarea. Si se habilita locking más adelante, comprobar que el proveedor y el flujo del editor lo soportan.

## Respaldo remoto pendiente

Antes de configurar/subir: elegir proveedor y cuenta, preferir privacidad acorde al trabajo académico, revisar licencia de assets y datos incluidos, y comprobar cuotas/costos de LFS para aproximadamente 655 MiB de contenido inicial y su crecimiento. La elección de proveedor no está decidida.

No añadir el DOCX original con matrículas ni registros de participantes por comodidad. La rúbrica pide Google Drive como destino de entrega, lo cual no sustituye un remoto Git ni autoriza subir esta carpeta completa.

## Recuperación

Para recuperar una versión, primero guardar y respaldar trabajo actual, identificar commit y revisar qué se reemplazará. Preferir recuperación de archivos concretos en una rama. No usar limpieza destructiva ni reset general para resolver una duda de estado.
