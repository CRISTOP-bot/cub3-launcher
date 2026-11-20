# Integración GitLab

CUB3 puede conectar una cuenta GitLab mediante OAuth de solo lectura para explorar proyectos donde participa la cuenta, ramas, commits recientes, etiquetas y releases. No hace `git pull` ni `push`, no descarga ni instala ejecutables y no sincroniza perfiles ni preferencias.

## Requisitos

- GitLab.com o una instancia GitLab Self-Managed accesible por HTTPS compatible con el flujo de autorización de dispositivo (GitLab 17.1 o posterior).
- Una aplicación OAuth registrada en GitLab. En Preferencias → Aplicaciones (`user_settings/applications`), crea una app de escritorio compatible con autorización de dispositivo y selecciona los permisos de lectura `read_api` y `read_user`. No habilites `api` ni permisos de escritura.
- El ID de aplicación (client ID). CUB3 no necesita el secreto de cliente.

Los controles de registro y autorización dependen de la versión e instancia de GitLab; un administrador puede deshabilitar OAuth o la autorización de dispositivo.

## Conectar

1. Abre **Ajustes → GitLab · proyectos y releases**.
2. Escribe la URL HTTPS de la instancia (`https://gitlab.com` por defecto) y el ID público de la app OAuth.
3. Pulsa **Conectar GitLab**. CUB3 abrirá la página oficial de autorización y mostrará el código temporal.
4. Introduce ese código en GitLab y aprueba el acceso. CUB3 valida la identidad y almacena la sesión en el llavero seguro del sistema.
5. Pulsa **Actualizar proyectos** y selecciona un proyecto para consultar ramas, commits, releases y etiquetas.

El ID de aplicación se guarda como preferencia local de CUB3 y no es un secreto. Los tokens de acceso y renovación solo se guardan en el llavero del sistema, nunca en SQLite, archivos, logs o el repositorio. Las solicitudes usan HTTPS y CUB3 informa si GitLab impone un límite temporal de solicitudes.

## Desconectar y revocar

Pulsa **Desconectar** y confirma para eliminar la sesión guardada en este dispositivo. Esto no revoca automáticamente el permiso en GitLab. Para revocarlo, quita la aplicación en GitLab → Preferencias → Aplicaciones autorizadas.

## Variables de entorno

No se requieren variables de entorno. El ID de cliente se introduce en Ajustes y los tokens se administran en el llavero seguro. No pegues tokens personales ni secretos en la interfaz.

## Alcance y limitaciones

- La conexión de CUB3 es independiente del MCP que usa Zapia; autorizar Zapia no autoriza automáticamente a CUB3.
- La lista inicial muestra hasta 100 proyectos (sin paginación en esta versión).
- La consulta es de solo lectura: CUB3 no modifica proyectos, ramas, issues ni releases.
- CUB3 muestra releases desde la API, pero no descarga ni instala actualizaciones del launcher.
- Perfiles, rutas y preferencias de Minecraft siguen locales. No se sincronizan entre dispositivos: CUB3 no tiene un servicio de sincronización ni un mecanismo de conflictos; no publiques esos datos en GitLab.
- CUB3 no descarga ni ejecuta código remoto.
