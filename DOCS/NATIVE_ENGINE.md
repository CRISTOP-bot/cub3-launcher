# Motor nativo C++ de CUB3 — estado

El código de motor se desarrolla **dentro de CUB3**, para que CUB3 sea el único programa que el usuario necesita abrir. La meta a largo plazo es ofrecer desde Perfiles una edición nativa del juego, descargable/actualizable/ejecutable desde el propio launcher, una vez que exista una build jugable real. Esto no es una segunda aplicación de launcher.

La implementación es clean-room: reproduce por etapas el comportamiento observable de Minecraft Java, inspirándose en el método de Horizon. No traduce ni copia código de Horizon o Mojang, no modifica el cliente oficial y no incluye archivos del juego.

## Prototipos y límites

`src/engine/World.*` es un prototipo pequeño con bloques básicos, chunks 16×128×16, una superficie determinista de prueba y acceso a bloques. `World` crea y mantiene chunks en memoria al acceder a coordenadas globales, permite consultarlos y descargarlos explícitamente; las ediciones de bloques actualizan la altura de superficie. La descarga elimina las modificaciones porque todavía no hay guardado persistente. No es un mundo Minecraft, no genera su terreno, no es jugable y no ofrece paridad.

El parser NBT de `src/engine/Nbt.*` es la primera pieza de lectura de formato del juego: decodifica NBT big-endian con límites configurables para tamaño, elementos y profundidad. Sus fixtures son creados por las pruebas, no tomados de mundos de usuario. Tampoco convierte por sí solo un mundo en algo jugable.

**Todavía no existe una versión C++ instalable ni descargable desde CUB3.** Faltan lectura comprimida de regiones Anvil, carga real de mundos, modelo de bloques, renderizador, controles, simulación, guardado y un runtime funcional. No se debe anunciar una build de Minecraft C++ hasta que exista y se pruebe.

La siguiente etapa debe avanzar en cambios pequeños compilables, con pruebas de formato y comparaciones diferenciales controladas contra Minecraft 26.3 cuando sea posible. Solo cuando haya un runtime nativo real se conectarán su descarga, verificación y lanzamiento a Perfiles de CUB3. La distribución de assets oficiales debe respetar las condiciones de Mojang y depender de la copia legítima del usuario.
