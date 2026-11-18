# Contribuir a CUB3 Launcher

Gracias por ayudar a mejorar CUB3. Los cambios de código, correcciones,
documentación, recursos y sitio web deben integrarse en la rama **`dev`**. Está
protegida: los commits directos están bloqueados y cada cambio se entrega por
un pull request hacia `dev`.

## Preparar el entorno

```sh
git clone https://github.com/CRISTOP-bot/cub3-launcher.git
cd cub3-launcher
git switch dev
git pull --ff-only origin dev
git switch -c tipo/descripcion-corta
```

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

Publica tu rama de trabajo y abre una solicitud de integración con
**`dev` como rama destino**. Explica el cambio, cómo se verificó y cualquier
limitación conocida. Resuelve los comentarios antes de integrar. `main` se
mantiene estable y solo recibe una promoción de versión aprobada por el
mantenedor.

## Reportar errores

Incluye los pasos para reproducir el problema, el resultado esperado y el
obtenido, distribución y versión de CUB3. Elimina nombres de usuario, tokens,
rutas privadas y otra información personal de capturas y registros.
