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

## Tutorial rápido

1. Abre **Inicio** y pulsa el botón de cuenta → **Conectar cuenta**.
2. Elige **Microsoft** para autorizar en el navegador. En esta compilación de desarrollo puede solicitar el ID GUID de una aplicación pública de Microsoft; no uses tu correo ni tu contraseña como ID.
3. Elige **Ely.by** para escribir el correo o usuario y la contraseña de Ely.by en la pestaña correspondiente. La contraseña no se guarda.
4. Selecciona una versión en Inicio. En **Ajustes** configura Java si hace falta, además de memoria, resolución y carpeta de juego.
5. Pulsa **Jugar**. Usa **Perfiles** para ver instalaciones locales y **Descargas** para seguir el progreso.

También puedes abrir la **Guía rápida** desde el botón de ayuda de la barra lateral dentro de la app. Haz copias de seguridad de tus mundos antes de probar builds de desarrollo.

## Funciones

- Inicio de sesión con Microsoft y Ely.by.
- Restauración automática de sesión desde el llavero seguro del sistema.
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
- QtKeychain para guardar la sesión de forma segura en el sistema.
- SQLite y el plugin Qt SQLite.
- `unzip` para extraer bibliotecas nativas en Linux.
- Java compatible con la versión de Minecraft que quieras ejecutar (recomendado; no es una versión fija).

El paquete `.deb` declara las bibliotecas compartidas detectadas durante el
empaquetado, además de `libqt6sql6-sqlite` y `unzip`; APT instala los paquetes
que falten al instalarlo. `default-jre-headless` se declara como recomendación
para que APT lo instale por defecto, pero puedes usar una JVM compatible ya
instalada. La versión de Java depende de Minecraft; por ejemplo, Minecraft
26.3 requiere Java 25.

Instálalo desde un terminal para que APT resuelva las dependencias:

```sh
sudo apt install ./cub3-launcher-dev-amd64.deb
```

El build de desarrollo del sitio se empaqueta en Debian 12 (Bookworm), no en
Ubuntu, y valida que el paquete no dependa de bibliotecas Qt `t64` que Bookworm
no ofrece. Está pensado para Debian 12 o posterior en amd64; otras
distribuciones pueden no ser compatibles. Cada build lleva una revisión
creciente para que APT lo reconozca como actualización y no como degradación.

En Debian o Ubuntu, instala las dependencias principales:

```sh
sudo apt install build-essential cmake qt6-base-dev libqt6sql6-sqlite qtkeychain-qt6-dev
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

**Todo cambio regular debe terminar en `dev`: código, correcciones, sitio web y
documentación.** `dev` es la rama activa y predeterminada. `main` está protegida:
no hagas commits directos allí; se reserva para promociones estables mediante
PR.

```sh
git clone https://github.com/CRISTOP-bot/cub3-launcher.git
cd cub3-launcher
git switch dev
```

Puedes trabajar directamente en `dev` o abrir un PR con destino `dev`. Lee
[`CONTRIBUTING.md`](CONTRIBUTING.md) antes de enviar cambios.

## Datos y privacidad

Consulta la [Política de uso y privacidad](https://cristop-bot.github.io/cub3-launcher/politica.html), publicada junto con el sitio del proyecto.

La base de datos y las instancias se guardan en el directorio de datos de
usuario de Qt (`cub3-launcher`). La caché de noticias e imágenes usa el
directorio de caché de usuario. Los tokens de sesión se guardan únicamente en el llavero
seguro del sistema; las contraseñas no se guardan. Si el llavero no está
instalado, bloqueado o rechaza una lectura/escritura, CUB3 avisa y no presenta
la sesión como guardada; no usa SQLite ni archivos en texto plano como respaldo.
Al iniciar, el launcher comprueba la cuenta con el proveedor y renueva tokens
cuando corresponde antes de mostrarla como autenticada. Microsoft requiere un
ID de cliente de una aplicación pública configurado en
`CUB3_MICROSOFT_CLIENT_ID` o en las preferencias de CUB3. No incluyas
credenciales, tokens ni secretos en commits, capturas o reportes públicos.

## Estructura del repositorio

- `src/`: aplicación C++ y Qt.
- `resources/`: recursos de escritorio e iconos.
- `assets/`: imágenes e identidad visual compartidas.
- `site/`: sitio estático publicado en GitHub Pages.
- `DOCS/`: auditorías y notas de versiones.
- `tests/`: pruebas automatizadas de componentes.

## Licencia

El código de este proyecto se distribuye bajo la licencia MIT. Consulta
[`LICENSE`](LICENSE) para ver sus términos. La MIT cubre el código original que
el proyecto puede licenciar; Minecraft, sus marcas y recursos de terceros no
quedan incluidos y conservan sus propias condiciones de uso.

## Marcas registradas

CUB3 Launcher es un proyecto independiente y no está afiliado, respaldado ni
aprobado por Mojang Studios o Microsoft. Minecraft es una marca de Mojang
Studios.
