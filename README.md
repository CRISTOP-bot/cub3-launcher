# CUB3 Launcher

Launcher ligero de Minecraft escrito en C++20 y Qt 6. Incluye inicio de sesión
con Microsoft y Ely.by, instalación de versiones vanilla, noticias oficiales,
perfiles locales y un instalador Debian.

## Requisitos

- CMake 3.21 o posterior
- Compilador compatible con C++20
- Qt 6: Widgets, Network, Sql y Concurrent
- SQLite y el controlador Qt SQLite
- Java compatible con la versión de Minecraft que quieras ejecutar
- `unzip` para extraer bibliotecas nativas en Linux

En Debian o Ubuntu, las dependencias de compilación suelen instalarse con:

```sh
sudo apt install build-essential cmake qt6-base-dev libqt6sql6-sqlite
```

## Compilar y ejecutar

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/cub3
```

Para crear el paquete `.deb` en Linux:

```sh
cpack --config build/CPackConfig.cmake -B build
```

El instalador aparece como `build/cub3-launcher-<versión>-<arquitectura>.deb`.
Al instalarlo, CUB3 queda disponible en el menú de aplicaciones.

## Datos y configuración

La base de datos y las instancias se guardan en el directorio de datos de
usuario de Qt (`cub3-launcher`). La caché de noticias e imágenes usa el
directorio de caché de usuario. Las preferencias de Java, memoria, carpeta de
instancias, resolución y descargas se editan en **Ajustes**.

Las noticias y notas de versión se obtienen de los feeds oficiales de Mojang.
Microsoft requiere el ID de cliente de una aplicación pública configurado en
`CUB3_MICROSOFT_CLIENT_ID` o en las preferencias de CUB3. No publiques ese ID
junto con secretos de cliente.

## Estado del proyecto

El flujo de instalación e inicio implementado actualmente es vanilla. La
creación de perfiles con cargadores de mods aún no está implementada. Revisa
[`docs/BUG_AUDIT.md`](docs/BUG_AUDIT.md) para los hallazgos revisados, cambios
realizados y limitaciones conocidas. El sitio estático se publica desde
[`site/`](site/index.html) mediante GitHub Pages: https://cristop-bot.github.io/cub3-launcher/.
