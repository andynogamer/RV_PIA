# Trabajo ordenado hasta la entrega

Fecha objetivo indicada: jueves 19 de noviembre de 2026. Este orden prioriza una demostración final; no es una promesa de fechas de funciones aún no probadas. Confirmar si el jueves corresponde al segundo avance. Las tareas de web, documental y documentos son requisitos de entrega, no autorización para publicarlos desde esta sesión.

## P0 — Simulación e integración indispensables

- [x] **UI-01 Menú y captura de entrada.** Menú inicial con controles/salida, pausa con continuar/reiniciar/inicio y resumen, cursor visible en interfaz y captura automática al conducir. Comprobación funcional en editor; interfaz espacial de RV pendiente.

- [x] **GOV-01 Gobernanza y línea base.** Leer propuesta/rúbrica, registrar estado, alcance y reglas, preparar Git/LFS y herramientas de inventario. Cierre: commit local y checks documentales correctos.
- [x] **SCN-01 Ensamblar el área demostrable.** Revisar si existe fuente GLTF/GLB con transformaciones originales. Importar escena o reconstruir una zona pequeña sin reimportar destructivamente las mallas. Guardar mapa nuevo bajo `/Game/RutaVirtual/Maps/`; definir calles, límites, escala, suelo, colisiones y punto de inicio. Cierre: recorrido inspeccionado, sin huecos ni caída bajo el suelo, materiales válidos y zona de acción identificable.
- [x] **VEH-01 Vehículo con teclado.** Seleccionar base física/visual y licencia; verificar cadena C++ si se usará. Implementar posesión, dirección, aceleración, frenado, reversa bloqueada en movimiento, cabina y reinicio. Cierre: avanzar, girar, detener y retroceder en SCN-01, sin depender del visor ni del G920 para desarrollar.
- [x] **URB-01 Primer playground urbano.** Nuevo mapa separado, inicio estacionado, límites de exploración de 770 × 760 m, colisión de suelo y 227 mallas de edificios, señalización y ruta guiada más extensa. Verificado en editor; no equivale a certificar cada calle de toda el área.
- [x] **TRA-01 Tráfico autónomo de prototipo.** Cuatro carriles, 12 autos, dos cruces semaforizados con ámbar y separación en rojo, frenado ante vehículos, reanudación y recirculación segura por distancia. Claxon audible con espera/cooldown. Refinado con seguimiento, reacción al verde, salida libre y luces de freno. Validación de escritorio; conducción humana compleja pendiente.
- [ ] **TRA-02 Evolución de circulación.** Conectar rutas con giros y cesión de paso, variar carrocerías y añadir direccionales. Luces de freno y claxon/audio contextual ya integrados y verificados en editor. Cierre: cruces y maniobras probados sin conflictos ni bloqueos permanentes. No cerrar por existir TRA-01.
- [x] **SES-01 Práctica guiada y sonido de escritorio.** Instrucciones, inicio, recorrido/objetivos, pausa/salida/reinicio, retroalimentación y sonidos de entorno/vehículo. Sesión repetible verificada en editor. La validación conjunta con RV/salida física queda en HMD-01/HAP-01/PKG-01.
- [x] **AUD-01 Mezcla y claxon.** Motor por régimen/carga, rodadura, ambiente, tráfico espacial y efectos sintetizados originales; H claxon y V silencio, pausa sin efectos atrasados. Assets en `/Game/RutaVirtual/Audio/`, componente nativo y fuentes reproducibles. Cierre de editor: señal real del bus maestro, silencio y pausa verificados. Escucha/calibración con auriculares finales pendiente.
- [x] **FEED-01 Consecuencias educativas.** Puntos 0–100, avisos correctivos y reconocimientos, resumen de pausa y solicitud lógica háptica cancelable. Rojo, ALTO de salida, contacto significativo, velocidad, distancia y sentido contrario junto a rutas. Quince comprobaciones correctas mediante física real de PIE. No cierra vibración física, evaluación clínica ni un sistema completo de normas.
- [ ] **HMD-01 Identificar y probar visor.** Depende del préstamo y datos del modelo; comprobar OpenXR, cámara sentada y recentrado. Cierre: imagen y tracking comprobados dentro del visor, con recorrido funcional.
- [ ] **G920-01 Integrar volante/pedales.** Depende de recibirlo. Diagnóstico, calibración y mapeo de ejes/botones, pruebas de desconexión. Cierre: mismo comportamiento funcional que teclado con dispositivo real, incluidos pedales en reposo.
- [ ] **HAP-01 Respuesta háptica congruente.** Seleccionar backend real de fuerza G920 y/o vibración Arduino; implementar evento, intensidad y apagado. Cierre: usuario acciona controles, la simulación responde y genera una sensación física ligada a un evento reproducible; probar pausa y fallo de conexión. Un botón sin respuesta física no cubre este cierre.
- [ ] **PKG-01 Entrega ejecutable.** Ajustar mapa de juego solo tras verificarlo, cocinar/empaquetar y probar en el PC de demostración con visor y háptico. Cierre: iniciar desde ejecutable sin editor y completar la práctica; conservar logs y plan de recuperación.

Estado: SCN-01, VEH-01, URB-01/TRA-01, SES-01, AUD-01 y FEED-01 verificados en editor el 14 de noviembre. La ciudad tiene colisión ampliada, con límites de cobertura y pruebas documentados en ESTADO.md. Hardware pendiente.

## P1 — Evidencia y entregables académicos

- [ ] **DOC-01 Actualizar documento académico.** Reescribir metodología para G920 y complemento realmente construido; explicitar objetivos de entrenamiento, limitaciones, resultados y fuentes verificadas. Revisar extensión por sección y exportar PDF con revisión visual. No inventar evaluación de usuarios ni eficacia terapéutica.
- [ ] **MKT-01 Empresa y publicidad.** Reunir o crear logo, objetivos y material de presentación; preparar web y contenido de una red social. Cierre: URL activa y contenido revisado por el equipo cuando se autorice publicarlo. No crear cuentas ni subir datos personales por inferencia de la rúbrica.
- [ ] **VID-01 Mini documental.** Guion, captura del escenario, sesión real y explicación del ciclo háptico; describir qué construyó el equipo. Cierre: archivo reproducible con audio legible y créditos.
- [ ] **DEL-01 Carpeta final.** Preparar ejecutable/instrucciones, PDF, video, vínculos y créditos/licencias. Verificar accesibilidad en el destino autorizado de Google Drive; no marcar entregado solo por reunir archivos locales.

## P2 — Mejoras que no bloquean el mínimo

- [ ] Peatones autónomos, condiciones adicionales, espejos optimizados o más recorridos.
- [ ] Ampliar Arduino con funciones adicionales después de demostrar un háptico estable.
- [ ] Automatizar más operaciones Blueprint si ofrece ahorro comprobable.

## Secuencia sugerida

**Primero:** SCN-01 y VEH-01, mientras el equipo identifica visor, componentes y material académico existente. Pueden avanzar documentos y guion con evidencia ya disponible.

**Siguiente hito propuesto tras el menú:** preparar cámara sentada/recentrado e interfaz espacial para Quest 3 como visor previsto, manteniendo Windows PCVR y teclado para desarrollo. La confirmación de imagen, tracking, comodidad y rendimiento requiere el visor real.

**En cuanto llegue hardware:** diagnóstico de G920 y HMD-01 antes de pulir toda la ciudad. Resolver incompatibilidad de runtime o entrada temprano. Confirmar préstamo de Quest 3 y disponibilidad de conexión Link.

**Antes de la revisión:** integrar HAP-01, ejecutar PKG-01 y grabar la demostración real. Reservar la última sesión para pruebas y entrega, evitando migraciones de motor o dependencias nuevas innecesarias.

## Formato de nuevas tareas

Cada tarea debe incluir identificador, objetivo observable, archivos/assets, dependencias, implementación, validación y limitaciones. Marcarla terminada solo al cumplir su criterio de cierre. Si falta hardware, registrar «implementado, pendiente de validación física» sin cerrar la tarea.
