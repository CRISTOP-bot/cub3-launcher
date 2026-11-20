# Contribuir a CUB3 Launcher

Gracias por ayudar a mejorar CUB3. **Toda contribución debe enviarse mediante
una pull request (PR) dirigida a `dev`; no se aceptan contribuciones mediante
push o commits directos a las ramas del proyecto.** Esto aplica al código,
correcciones, documentación, recursos y sitio web. `dev` es la rama activa y
predeterminada; `main` se reserva para promociones estables aprobadas por PR.

## Preparar el entorno

```sh
# Haz fork del repositorio en GitHub y clona tu fork
git clone https://github.com/TU-USUARIO/cub3-launcher.git
cd cub3-launcher
git remote add upstream https://github.com/CRISTOP-bot/cub3-launcher.git
git fetch upstream
git switch -c feat/descripcion-del-cambio upstream/dev
```

Trabaja siempre en una rama propia basada en `upstream/dev`, súbela a tu fork
y abre una PR con destino `dev`. No hagas push directo a `dev` ni a `main`.

Consulta los requisitos y pasos de compilación en el [README](README.md).

## Cambios y validación

- Mantén los cambios enfocados y describe el problema que resuelven.
- Para la aplicación, compila con CMake en modo Release.
- Para el sitio, conserva rutas relativas compatibles con GitHub Pages bajo
  `/cub3-launcher/` y actualiza el flujo de Pages si agregas nuevos assets.
- Ejecuta las comprobaciones pertinentes a los archivos que cambiaste y
  reporta exactamente qué se ejecutó y su resultado.
- No agregues dependencias, pruebas o funciones ajenas al cambio sin motivo.
- No incluyas secretos, tokens, archivos de compilación ni datos personales.

Antes de confirmar, revisa el cambio:

```sh
git diff --check
git status --short
```

Usa mensajes de commit concisos, en imperativo, por ejemplo:

```text
Fix settings path validation
Document Linux installation
Improve mobile download layout
```

## Integración

Todas las contribuciones deben integrarse mediante una pull request a `dev`.
No se aceptan pushes directos, aunque tengas permisos de escritura. Configura
**`dev` como rama destino** y explica el cambio, cómo se verificó y cualquier
limitación conocida. `main` también requiere PR y se mantiene para promociones
estables aprobadas por el mantenedor.

## Reportar errores

Incluye los pasos para reproducir el problema, el resultado esperado y el
obtenido, distribución y versión de CUB3. Elimina nombres de usuario, tokens,
rutas privadas y otra información personal de capturas y registros.
