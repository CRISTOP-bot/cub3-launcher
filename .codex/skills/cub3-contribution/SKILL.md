---
name: cub3-contribution
description: Follow CUB3's experimental project rules when changing code, docs, assets, or the website and preparing a pull request.
---

# Contribuir a CUB3

1. Lee `AGENTS.md` y las partes pertinentes de `CONTRIBUTING.md`.
2. Comprueba el estado del árbol y la rama actual. Conserva cualquier trabajo
   local existente.
3. Trabaja en `dev` o basa una rama corta en `origin/dev`. Todo cambio regular
   de código, documentación, recursos y web termina en `dev`.
4. No hagas commits ni pushes directamente a `main`: está protegida y se
   reserva para promociones estables mediante PR.
5. Describe la validación que realmente se ejecutó. No afirmes que una
   comprobación pasó si no se ejecutó; no hay que inventar pruebas.
6. Indica riesgos y limitaciones. CUB3 tiene apenas unos días de desarrollo,
   es experimental y no es estable. Evita prometer fiabilidad o seguridad de
   datos.
7. Revisa `git diff --check` y `git status --short` antes de publicar la rama.
