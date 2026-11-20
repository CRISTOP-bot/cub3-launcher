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
git switch nombre-de-tu-rama-de-trabajo-existente
git rebase upstream/dev
```

Usa la rama de trabajo que ya tienes en tu fork y abre desde ella una PR con
`dev` como destino. No hace falta crear otra rama para la PR, y no crees ramas
de trabajo en el repositorio original. Si todavía no tienes una rama fuente, una
PR de GitHub sí requiere una rama distinta de `dev` en tu fork. No hagas push
directo a `dev` ni a `main`.

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

Todos los PR deben tener **`dev` como rama destino**; no se aceptan PR dirigidos
a otra rama. Abre el PR desde la rama de trabajo existente en tu fork: no crees
una rama adicional en el repositorio original ni una rama intermedia solo para
abrir el PR. Una PR de GitHub requiere una rama fuente distinta de `dev`, por lo
que la rama existente del fork es la fuente. No se aceptan pushes directos a
`dev` ni a `main`, aunque tengas permisos de escritura. Explica el cambio, cómo
se verificó y cualquier limitación conocida. `main` se mantiene para promociones
estables aprobadas por PR.

## Reportar errores

Incluye los pasos para reproducir el problema, el resultado esperado y el
obtenido, distribución y versión de CUB3. Elimina nombres de usuario, tokens,
rutas privadas y otra información personal de capturas y registros.
