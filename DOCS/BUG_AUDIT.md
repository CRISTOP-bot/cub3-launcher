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

- Las sesiones de Microsoft y Ely.by se conservan en el llavero seguro del
  sistema. Si no hay un servicio de credenciales disponible o el usuario
  deniega el acceso, la sesión dura solo hasta cerrar el launcher.
- Los tokens son secretos locales protegidos por el llavero del sistema. No se
  guardan contraseñas de Microsoft ni de Ely.by.
- Microsoft requiere configurar un ID de cliente de una aplicación pública.
- La biblioteca muestra instalaciones vanilla; crear instancias con Fabric,
  Forge u otros cargadores todavía no está implementado.
- `tests/` no contiene casos automatizados en esta revisión. CMake informa que
  no hay fuentes de prueba.
- Las distribuciones Linux deben tener instalada la versión Java requerida por
  el manifiesto de Minecraft. En Debian/Ubuntu, Java 25 se instala con
  `sudo apt install openjdk-25-jre-headless`.

## Comprobaciones

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
git diff --check
```

El build debe completarse. `ctest` no ejecuta casos mientras `tests/` siga sin
fuentes de prueba.
