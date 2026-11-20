# Edición nativa administrada desde CUB3

La meta es que **CUB3 sea la única aplicación que el usuario abre**: su biblioteca mostrará la edición nativa cuando haya una build jugable, y CUB3 administrará la descarga, actualización, perfil y ejecución del runtime C++ dentro de esa instancia. El motor se integra como módulo de CUB3, no como launcher alternativo.

Esto no convierte el binario oficial de Mojang ni copia su código. Es una implementación clean-room del comportamiento observable, inspirada en el enfoque de Horizon, verificada por etapas frente a Minecraft Java 26.3 usando la copia del juego que el usuario ya posee cuando sea técnicamente y legalmente posible. Los archivos oficiales no se subirán al repositorio ni se redistribuirán.

## Estado real

- **Implementado en esta etapa:** modelo y parser NBT big-endian, con límites de bytes, elementos y profundidad; pruebas unitarias sintéticas integradas al test target de CUB3.
- **No implementado:** lectura comprimida de regiones Anvil, cargar un mundo, chunks/renderizado, jugador/controles, simulación, autenticación/protocolo, o build jugable descargable.
- **Por ello CUB3 todavía no ofrece una versión C++ instalable:** no existe un runtime que pueda descargarse y ejecutarse. No se añade una versión falsa ni se reutiliza el JAR Java bajo la etiqueta C++.

## Siguiente secuencia

1. Completar regiones Anvil y lectura NBT comprimida con fixtures creados por tests y comprobaciones locales.
2. Abrir mundos de Minecraft 26.3 del usuario en modo de solo lectura y comparar campos/resultados observables sin copiar sus datos al repo.
3. Implementar y verificar por partes el modelo de mundo, reglas de bloques, generación y simulación; cada parte requiere pruebas propias y diferenciales.
4. Crear una build nativa ejecutable solamente cuando las etapas de mundo/render/juego la sostengan.
5. Entonces agregar a CUB3 un tipo de runtime/instalación `CUB3 C++ Edition` que descargue assets permitidos o los obtenga de la instalación autorizada, verifique hashes/versiones y ejecute el binario nativo desde Perfiles.

Cada etapa debe distinguir pruebas unitarias, comparaciones diferenciales y juego integrado. No se declarará compatible ni jugable antes de pruebas reales.
