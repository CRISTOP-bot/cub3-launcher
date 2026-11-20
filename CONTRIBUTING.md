# Contribuir a CUB3 Launcher

Gracias por ayudar a mejorar CUB3. **Las personas con permiso de escritura en
el repositorio pueden hacer commits en sus ramas de trabajo. Quienes no tienen ese acceso deben contribuir desde un fork y enviar un pull request (PR).** Los
cambios se integran mediante PR dirigido a `dev`; no se aceptan pushes directos
a `dev` ni a `main`. Esto aplica al código, correcciones, documentación,
recursos y sitio web. `dev` es la rama activa y predeterminada; `main` se reserva
para promociones estables aprobadas por PR. El permiso de escritura permite
hacer commits en el repositorio, pero no omitir la revisión y las validaciones
requeridas para integrar cambios.

## Preparar el entorno

Crea una rama de trabajo en el repositorio original si tienes permiso de
escritura. Si no tienes ese acceso, haz un fork y trabaja en una rama de tu
fork:

```sh
# Si no tienes acceso de escritura, haz fork en GitHub y clona tu fork.
# Si tienes acceso de escritura, clona el repositorio original en su lugar.
git clone https://github.com/TU-USUARIO/cub3-launcher.git
cd cub3-launcher
git remote add upstream https://github.com/CRISTOP-bot/cub3-launcher.git
git fetch upstream
git switch -c nombre-de-tu-rama-de-trabajo upstream/dev
```

Para el clon con permiso de escritura, usa la URL del repositorio original en
`git clone` y crea tu rama de trabajo desde `origin/dev`. Para un fork, conserva
`upstream` como se muestra y crea la rama desde `upstream/dev`. En ambos casos,
abre un PR hacia `dev` desde tu rama; no hagas push directo a `dev` ni a `main`.
No es necesario crear una rama intermedia solo para abrir el PR.

Quienes tienen permiso de escritura pueden crear commits en ramas del
repositorio original. Quienes no tienen ese acceso deben crear sus commits en
una rama de su propio fork y solicitar la integración mediante una PR. Para
ambos casos, el PR debe pasar la revisión y las validaciones requeridas antes de
integrarse; el permiso de escritura no las omite.

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

Los cambios destinados a `dev` se integran mediante un PR con **`dev` como
rama destino**; no se aceptan PR dirigidos a otra rama. Quien tenga permiso de
escritura puede hacer commits en una rama del repositorio original; quien no lo
tenga debe abrir el PR desde una rama de su fork. En ambos casos, no hagas push
directo a `dev` ni a `main`; el PR debe pasar la revisión y las validaciones
requeridas antes de integrarse. No crees una rama intermedia solo para abrir el
PR. Explica el cambio, cómo se verificó y cualquier limitación conocida. `main`
se mantiene para promociones estables aprobadas mediante PR.

## Reportar errores

Incluye los pasos para reproducir el problema, el resultado esperado y el
obtenido, distribución y versión de CUB3. Elimina nombres de usuario, tokens,
rutas privadas y otra información personal de capturas y registros.
