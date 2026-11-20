# Discord Rich Presence

CUB3 puede anunciarse en el perfil de Discord cuando el usuario habilita la función. La integración usa el IPC local de la aplicación de escritorio Discord (socket local en Linux/macOS o named pipe en Windows); no añade SDK ni contacta una API remota. Si Discord no está abierto, CUB3 reintenta cada 15 segundos.

## Configuración

1. Crea una aplicación en el [Discord Developer Portal](https://discord.com/developers/applications).
2. Copia el **Application ID** público de la aplicación. No uses un bot token, client secret ni token de usuario.
3. En CUB3 abre **Ajustes → Discord · presencia enriquecida**, pega el ID y activa la presencia. Guarda los cambios.
4. Para apagarla, desmarca la opción y guarda. La actividad se borra de Discord al desconectar.

La presencia informa **“En el launcher”** mientras CUB3 espera y **“Jugando Minecraft <versión>”** cuando el proceso de Minecraft iniciado por CUB3 está activo. No comparte el nombre de tu cuenta, nombre/ruta de mundo, dirección de servidor ni mensajes. La función está apagada por defecto y guarda solo el ID público y la preferencia local.

## Estado y validación

Las pruebas cubren el encabezado binario little-endian del IPC y la validación del ID de aplicación. No sustituyen una prueba manual con una aplicación de Discord registrada y el cliente de escritorio abierto. La presencia no funciona con Discord abierto solo en un navegador ni con un cliente móvil.
