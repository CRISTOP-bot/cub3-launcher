# Instrucciones para agentes y colaboradores

- **Todo cambio regular debe terminar en `dev`**: código, documentación,
  recursos, configuración y sitio web. `dev` es la rama activa y predeterminada.
- `dev` no está protegida. Se permiten commits directos y PR con destino `dev`;
  para cambios amplios, usa una rama corta y revisa el diff antes de integrar.
- `main` está protegida: no hagas commits ni pushes directos. Se reserva para
  versiones estables y solo recibe una promoción aprobada por el mantenedor a
  través de un PR.
- Antes de editar, verifica la rama y la base de tus cambios. Si hay cambios
  locales sin confirmar, consérvalos; nunca los descartes para cambiar de rama.
- Consulta `CONTRIBUTING.md` para compilar, validar cambios y preparar el PR.
- No afirmes que se ejecutaron pruebas si no se ejecutaron. Documenta con
  claridad limitaciones y funciones todavía no implementadas.
- No incluyas tokens, credenciales, archivos `.env` ni datos personales.
