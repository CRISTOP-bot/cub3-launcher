# Iconos de CUB3

Los iconos de navegación y acciones usan SVG vectoriales de 24 × 24, con un
trazo de 1.8 y `currentColor`. La barra lateral los rasteriza y aplica los
colores de texto del tema; no necesitan una variante clara u oscura separada.
Los alias de `resources/cub3.qrc` forman parte de su integración con la app.

- `dashboard`: inicio; `library`: perfiles; `downloads`: descargas.
- `mods`: mods; `settings`: ajustes; `play`: jugar; `refresh`: actualizar.
- `cub3.svg` es el icono de identidad existente y mantiene su diseño y colores.

Los iconos son archivos SVG del proyecto. Comprueba su sintaxis y referencias
con `python3 tests/AssetTests.py`; revisa además su apariencia en la aplicación
Qt al cambiar las formas, ya que esa prueba no sustituye una revisión visual.
