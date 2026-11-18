# CUB3 Launcher

![Estado de GitHub Pages](https://github.com/CRISTOP-bot/cub3-launcher/actions/workflows/pages.yml/badge.svg?branch=dev)

Launcher de Minecraft para Linux, escrito en C++20 y Qt 6. CUB3 permite iniciar
sesión con Microsoft o Ely.by, instalar versiones vanilla, consultar novedades
oficiales y organizar instalaciones locales.

> **Pre lanzamiento:** la versión pública actual es `0.1.0-pre.1` para Debian y
> Ubuntu de 64 bits. Windows y macOS todavía no tienen instaladores.
>
> **Proyecto experimental:** CUB3 tiene apenas unos días de vida. No es
> estable; puede contener errores, cambiar sin aviso o perder datos. Haz copias
> de seguridad de tus mundos y configuraciones antes de probarlo.

## Descargar

- [Paquete Debian/Ubuntu x86_64](https://github.com/CRISTOP-bot/cub3-launcher/releases/download/v0.1.0-pre.1/cub3-launcher-0.1.0-x86_64.deb)
- [Notas y archivos de la versión](https://github.com/CRISTOP-bot/cub3-launcher/releases/tag/v0.1.0-pre.1)
- [Sitio del proyecto](https://cristop-bot.github.io/cub3-launcher/)

Instala el paquete desde una terminal:

```sh
sudo apt install ./cub3-launcher-0.1.0-x86_64.deb
```

Se requiere una versión de Java compatible con Minecraft. Para Minecraft 26.3,
el manifiesto solicita Java 25.

## Funciones

- Inicio de sesión con Microsoft y Ely.by.
- Instalación y ejecución de versiones vanilla.
- Feed de noticias oficial de Minecraft.
- Preferencias de Java, memoria, resolución, rutas y descargas.
- Paquete `.deb` y acceso desde el menú de aplicaciones.

La creación de perfiles con Fabric, Forge u otros cargadores todavía no está
implementada. Consulta [la auditoría de errores y limitaciones](DOCS/BUG_AUDIT.md)
para conocer el estado revisado.

## Compilar desde el código

### Requisitos

- CMake 3.21 o posterior y un compilador C++20.
- Qt 6 con Widgets, Network, Sql y Concurrent.
- SQLite y el controlador Qt SQLite.
- Java compatible con la versión de Minecraft que quieras ejecutar.
- `unzip` para extraer bibliotecas nativas en Linux.

En Debian o Ubuntu, instala las dependencias principales:

```sh
sudo apt install build-essential cmake qt6-base-dev libqt6sql6-sqlite
```

Configura, compila y ejecuta:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/cub3
```

Para generar un paquete Debian:

```sh
cpack --config build/CPackConfig.cmake -B build
```

El paquete se genera en `build/`.

## Flujo de desarrollo

**Todo cambio debe terminar en `dev`: código, correcciones, sitio web y
documentación.** `dev` es la rama predeterminada y está protegida: trabaja en
una rama corta y abre un PR hacia `dev`. No confirmes directamente en `dev` ni
en `main`; esta última queda reservada para versiones estables.

```sh
git clone https://github.com/CRISTOP-bot/cub3-launcher.git
cd cub3-launcher
git switch dev
git switch -c tipo/descripcion-corta
```

Lee [`CONTRIBUTING.md`](CONTRIBUTING.md) antes de enviar cambios. Los PR deben
tener como destino `dev`.

## Datos y privacidad

La base de datos y las instancias se guardan en el directorio de datos de
usuario de Qt (`cub3-launcher`). La caché de noticias e imágenes usa el
directorio de caché de usuario. Microsoft requiere un ID de cliente de una
aplicación pública configurado en `CUB3_MICROSOFT_CLIENT_ID` o en las
preferencias de CUB3. No incluyas credenciales, tokens ni secretos en commits,
capturas o reportes públicos.

## Estructura del repositorio

- `src/`: aplicación C++ y Qt.
- `resources/`: recursos de escritorio e iconos.
- `assets/`: imágenes e identidad visual compartidas.
- `site/`: sitio estático publicado en GitHub Pages.
- `DOCS/`: auditorías y notas de versiones.
- `tests/`: espacio reservado para pruebas automatizadas.

## Licencia

El código de este proyecto se distribuye bajo la licencia MIT. Consulta
[`LICENSE`](LICENSE) para ver sus términos. La MIT cubre el código original que
el proyecto puede licenciar; Minecraft, sus marcas y recursos de terceros no
quedan incluidos y conservan sus propias condiciones de uso.

## Marcas registradas

CUB3 Launcher es un proyecto independiente y no está afiliado, respaldado ni
aprobado por Mojang Studios o Microsoft. Minecraft es una marca de Mojang
Studios.
