# Motor nativo C++ — estado inicial

Este es un núcleo de mundo voxel independiente escrito en C++20, separado de la capa Qt del launcher. La primera pieza (`src/engine/World.*`) define bloques `Air`, `Grass`, `Dirt` y `Stone`, chunks de 16×128×16, terreno determinista por semilla/coordenada y conversión robusta de coordenadas de mundo negativas a coordenadas locales. Las pruebas verifican generación reproducible, acceso y edición con límites y división de coordenadas negativas.

## Límites actuales

Esta base **no es un juego jugable**: aún faltan renderizado, cámara/controles, colisiones y física, ciclo de actualización, gestión/streaming de chunks, inventario, guardado, sonido y una interfaz de juego. No carga, distribuye ni imita los assets, código ni formato de mundos de Minecraft; no declara compatibilidad ni paridad con Minecraft u Horizon. El generador actual es una superficie de prueba sencilla, no un generador de terreno de Minecraft.

Las siguientes etapas deben avanzar como cambios pequeños compilables con pruebas propias; validar cada comportamiento contra una referencia solo cuando exista una implementación real y una especificación verificable. Recursos y archivos del juego siguen siendo responsabilidad del usuario y deben respetar sus licencias y términos.
