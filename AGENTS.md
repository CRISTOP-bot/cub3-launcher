# Instrucciones para agentes y colaboradores

- **Todo cambio del proyecto debe terminar en `dev`**: código, documentación,
  recursos, configuración y sitio web.
- `dev` está protegida. Crea una rama corta desde `dev`, trabaja allí y abre un
  pull request con destino `dev`. Nunca confirmes ni publiques commits
  directamente en `dev`.
- No integres cambios en `main`. Esa rama se reserva para versiones estables;
  cualquier actualización de `main` debe ser una instrucción explícita del
  mantenedor.
- Antes de editar, verifica la rama y la base de tus cambios. Si hay cambios
  locales sin confirmar, consérvalos; nunca los descartes para cambiar de rama.
- Consulta `CONTRIBUTING.md` para compilar, validar cambios y preparar el PR.
- No afirmes que se ejecutaron pruebas si no se ejecutaron. Documenta con
  claridad limitaciones y funciones todavía no implementadas.
- No incluyas tokens, credenciales, archivos `.env` ni datos personales.
