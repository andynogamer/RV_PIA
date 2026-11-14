# Repositorio y colaboración

Este proyecto utiliza Git y Git LFS. La base está en `main`; el usuario configuró el remoto `origin` en GitHub después de la gobernanza inicial. El primer desarrollo está en la rama local `feature/primer-circuito` y esta sesión no hizo push. Visibilidad y cuotas del remoto no se verificaron. Los cambios locales no quedan respaldados remotamente hasta subirlos.

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

## Respaldo remoto del avance

Antes de subir nuevas ramas: comprobar destino y privacidad, licencia de assets, datos incluidos y cuotas/costos de LFS. Ya existe un remoto GitHub, pero no se asume autorización para publicar cada avance por el hecho de que esté configurado.

No añadir el DOCX original con matrículas ni registros de participantes por comodidad. La rúbrica pide Google Drive como destino de entrega, lo cual no sustituye un remoto Git ni autoriza subir esta carpeta completa.

## Recuperación

Para recuperar una versión, primero guardar y respaldar trabajo actual, identificar commit y revisar qué se reemplazará. Preferir recuperación de archivos concretos en una rama. No usar limpieza destructiva ni reset general para resolver una duda de estado.
