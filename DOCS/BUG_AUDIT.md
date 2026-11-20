# Revisión de errores

Revisión manual del flujo de interfaz, preferencias, instalación e inicio de
Minecraft. Validación de compilación Release y revisión de formato con
`git diff --check`.

## Corregido

- **Ajustes se cortaba al final de la ventana:** la página ahora permite
  desplazarse y mantiene el botón de guardado visible.
- **No había confirmación al guardar:** se muestra el resultado y se rechazan
  rutas de Java no ejecutables o carpetas de instancias inexistentes/no
  escribibles.
- **La carpeta configurada no se usaba:** Minecraft ahora crea y usa el
  directorio elegido en Ajustes.
- **La resolución se ignoraba:** los argumentos de Minecraft reciben los
  valores guardados por el usuario.
- **Las opciones de descargas no se aplicaban al arrancar:** concurrencia y
  verificación se cargan al crear el motor y se actualizan al guardar.
- **Los lanzamientos no aparecían en Perfiles ni en el contador del inicio:**
  al iniciar Minecraft se registra o actualiza la instalación vanilla.
- **Minecraft 26.3 terminaba con código 1:** su manifiesto requiere Java 25 y
  el equipo solo tenía Java 21. El launcher ahora lee el requisito por versión,
  busca una JVM compatible, guarda la ruta elegida y conserva la salida de Java
  en `logs/cub3-launcher.log` dentro de la carpeta de esa instalación.
- **El botón de guardado prometía preferencias sin efecto:** se retiró el
  control de efectos reducidos, que no tenía una implementación asociada.

## Limitaciones conocidas

- **Sesiones de Microsoft y Ely.by:** los tokens se guardan exclusivamente en
  el llavero seguro del sistema. Lecturas y escrituras se verifican y notifican;
  no existe respaldo en texto plano. Si el servicio de credenciales no está
  disponible, se deniega el acceso o la escritura falla, la sesión no puede
  persistir y CUB3 informa el error. La restauración valida con el proveedor y
  renueva tokens vencidos antes de mostrar la cuenta.
- **Los tokens son secretos locales protegidos por el llavero del sistema. No se
  guardan contraseñas de Microsoft ni de Ely.by.** Una denegación del proveedor
  o una falla de red durante la validación no borra automáticamente los datos
  del llavero.
- **Escrituras y cierres de sesión:** las operaciones asíncronas del llavero se
  serializan y se descartan respuestas antiguas para evitar que una lectura o
  escritura pendiente restaure una cuenta después de un logout/login posterior.
  La escritura del login termina antes de anunciar el inicio como completado.
- **Pruebas de autenticación:** se añadieron pruebas de serialización y perfil
  de cuenta. Los fallos/renovaciones de proveedores y la coordinación con un
  llavero simulado aún requieren cobertura automatizada específica; no se
  consideran verificados por estas pruebas de modelo.
- Microsoft requiere configurar un ID de cliente de una aplicación pública.
- La biblioteca muestra instalaciones vanilla; crear instancias con Fabric,
  Forge u otros cargadores todavía no está implementado.
- `tests/AuthSessionTests.cpp` cubre la serialización de cuentas Microsoft y
  Ely.by, la conservación de UUID/nombre/skin y las condiciones de expiración.
  No sustituye las pruebas integradas de almacenamiento, red y reanudación entre
  procesos que aún faltan.
- Las distribuciones Linux deben tener instalada la versión Java requerida por
  el manifiesto de Minecraft. En Debian/Ubuntu, Java 25 se instala con
  `sudo apt install openjdk-25-jre-headless`.

## Funciones nuevas aún sin verificación manual

- **Discord Rich Presence:** el cliente opcional usa el IPC local de Discord y requiere un Application ID público configurado por el usuario. La presencia indica solo el launcher o la versión de Minecraft; no anuncia usuario, mundo ni servidor. Las pruebas automáticas validan el encuadre del protocolo y el formato del ID, no una sesión real con Discord.
- **Motor nativo C++:** `src/engine/` contiene generación determinista de un terreno voxel básico y acceso a bloques/chunks. Aún no hay renderizador, controles ni juego ejecutable; tampoco se afirma compatibilidad con Minecraft. Ver [`NATIVE_ENGINE.md`](NATIVE_ENGINE.md).

- **Título C++ Edition:** para Minecraft 26.3 se añade un paquete de recursos local que reemplaza únicamente `assets/minecraft/textures/gui/title/edition.png`. CUB3 lo activa en `options.txt` si el ajuste está encendido y lo deselecciona al apagarlo. No se parchea ni redistribuye el JAR de Minecraft. La validación automatizada no sustituye la comprobación visual en el juego real.

## Comprobaciones

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
git diff --check
```

El build debe completarse. `ctest` no ejecuta casos mientras `tests/` siga sin
fuentes de prueba.
