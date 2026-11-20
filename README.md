# CUB3 Launcher

![Estado de GitHub Pages](https://github.com/CRISTOP-bot/cub3-launcher/actions/workflows/pages.yml/badge.svg?branch=dev)

Launcher de Minecraft escrito en C++20 y Qt 6 para Linux y Windows x64. CUB3 permite iniciar sesión con Microsoft o Ely.by, instalar versiones vanilla, consultar novedades oficiales y organizar instalaciones locales.

> **Pre lanzamiento:** la versión pública `0.1.0-pre.1` incluye el paquete Debian/Ubuntu x86_64. El build portable de Windows x64 se genera en CI como artefacto de desarrollo (retención de 30 días); no es todavía una publicación estable ni un instalador. macOS sigue sin port.
>
> **Proyecto experimental:** CUB3 tiene apenas unos días de vida. No es
> estable; puede contener errores, cambiar sin aviso o perder datos. Haz copias
> de seguridad de tus mundos y configuraciones antes de probarlo.

## Descargar

- [Paquete Debian/Ubuntu x86_64](https://github.com/CRISTOP-bot/cub3-launcher/releases/download/v0.1.0-pre.1/cub3-launcher-0.1.0-x86_64.deb)
- [Notas y archivos de la versión](https://github.com/CRISTOP-bot/cub3-launcher/releases/tag/v0.1.0-pre.1)
- [Build portable de Windows x64 en GitHub Actions](https://github.com/CRISTOP-bot/cub3-launcher/actions/workflows/build.yml) (artifact de desarrollo, retención 30 días)

Para obtener el ZIP de Windows, abre el workflow, elige una ejecución exitosa y descarga el artefacto **cub3-launcher-windows-x64**. Comprueba el `.sha256` incluido antes de usarlo. No hay instalador ni Release estable de Windows todavía.
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

También puedes abrir la **Guía rápida** desde el botón de ayuda de la barra lateral dentro de la app. El tutorial muestra el paso actual y permite avanzar o retroceder con las flechas izquierda/derecha. En la ventana principal, `Ctrl+1` a `Ctrl+5` llevan a Inicio, Perfiles, Descargas, Mods y Ajustes.

Durante el inicio de sesión, CUB3 mantiene el formulario abierto, muestra el progreso y presenta los errores ahí mismo. Si cancelas Microsoft, se detiene el sondeo del código de dispositivo. Si falla Ely.by, se borra la contraseña del campo y se conserva el usuario para corregirlo. Los controles principales tienen nombres accesibles; Ajustes avisa cuando hay cambios sin guardar y Descargas explica su estado vacío.

Haz copias de seguridad de tus mundos antes de probar builds de desarrollo.

## Funciones

- Inicio de sesión con Microsoft y Ely.by.
- Restauración automática de sesión desde el llavero seguro del sistema.
- Instalación y ejecución de versiones vanilla.
- Feed de noticias oficial de Minecraft.
- Preferencias de Java, memoria, resolución, rutas y descargas.
- Paquete `.deb` para Linux y build portable ZIP x64 para Windows (artefacto de desarrollo de CI).
- Búsqueda de mods en Modrinth por versión/cargador, descarga del `.jar` seleccionado a la carpeta `mods` de la instalación y acceso al proyecto. CurseForge abre su búsqueda web; CUB3 todavía no instala cargadores.

Antes de descargar un mod, revisa la versión compatible, licencia y requisitos en la página del proyecto. Los `.jar` son código de terceros y no los verifica ni respalda CUB3.

La creación de perfiles con Fabric, Forge u otros cargadores todavía no está
implementada. Consulta [la auditoría de errores y limitaciones](DOCS/BUG_AUDIT.md)
para conocer el estado revisado.

## Compilar desde el código

### Requisitos

- CMake 3.21 o posterior y un compilador C++20.
- Qt 6 con Widgets, Network, Sql y Concurrent.
- QtKeychain para guardar la sesión de forma segura en el sistema.
- SQLite y el plugin Qt SQLite.
- `unzip` para extraer bibliotecas nativas en Linux; en Windows se usa `tar.exe` incluido en Windows 10 1809 o posterior y Windows 11.
- Java compatible con la versión de Minecraft que quieras ejecutar (recomendado; no es una versión fija).

En Windows se requiere Visual Studio 2022 con C++ y CMake, además de Git. El manifiesto `vcpkg.json` fija el baseline e instala Qt 6, Qt SVG, el driver SQLite y QtKeychain. Desde PowerShell, configura y ejecuta las pruebas así:

```powershell
git clone https://github.com/microsoft/vcpkg.git vcpkg
git -C vcpkg checkout e1ff419d28c7f0a94abaf0a848fe0aeeb69b2de3
.\vcpkg\bootstrap-vcpkg.bat -disableMetrics
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

La CI compila y ejecuta CTest en Windows, prepara un ZIP portable con las DLL/plugins de Qt y el runtime de MSVC, e incluye el checksum SHA-256. El ZIP se sube como artefacto de desarrollo temporal de Actions (30 días); aún no se genera un instalador ni una Release estable de Windows.

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

## Contribuir

**Todas las contribuciones se envían mediante pull request (PR) a `dev`. No se aceptan pushes ni commits directos a las ramas del proyecto.** Haz fork, trabaja en una rama propia basada en `upstream/dev`, súbela a tu fork y abre la PR con destino `dev`. Esto aplica al código, pruebas, documentación, recursos y sitio. `main` se reserva para promociones estables mediante PR.

Lee [`CONTRIBUTING.md`](CONTRIBUTING.md) para conocer el flujo completo, las comprobaciones y cómo reportar errores.

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
