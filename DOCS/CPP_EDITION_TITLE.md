# Leyenda “C++ Edition” en Minecraft 26.3

CUB3 puede cambiar la leyenda visual **Java Edition** del menú principal por **C++ Edition** mediante el sistema normal de paquetes de recursos de Minecraft. El ajuste está activado por defecto en **Ajustes → Título del juego**. Al iniciar Minecraft 26.3, CUB3 copia un paquete pequeño y original a la carpeta de recursos de esa instancia y lo agrega a la lista seleccionada de `options.txt`. Desactiva la casilla y guarda para volver al texto normal.

El pack solo reemplaza la textura `assets/minecraft/textures/gui/title/edition.png`; no modifica el JAR, no cambia la implementación del juego ni convierte Java en C++. Minecraft sigue siendo Java Edition por dentro. La marca se aplica a la leyenda del logo en el menú de título (no al título de la ventana del sistema) y está preparada para la versión 26.3, cuyo formato de paquete es 97.1. En otras versiones, CUB3 no selecciona el pack para evitar avisos de incompatibilidad.

El contenido del paquete se crea originalmente para CUB3; no incluye texturas del juego. Minecraft y el launcher oficial siguen descargando sus propios recursos.

Para regenerar el archivo ZIP desde el arte fuente incluido, ejecuta `python3 scripts/build_cpp_edition_pack.py` desde la raíz del repositorio.
