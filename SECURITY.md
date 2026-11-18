# Seguridad

No publiques contraseñas, tokens OAuth, credenciales de Microsoft, archivos de
configuración privados ni registros que contengan datos personales.

Las peticiones remotas de la aplicación deben usar HTTPS; se rechazan URLs con
credenciales incorporadas y los registros de red omiten consultas y fragmentos.
Las respuestas de API se procesan en streaming con un límite de 32 MiB; las
respuestas de mayor tamaño se cancelan. Las descargas de archivos usan un búfer
acotado y, si el manifiesto indica el tamaño esperado, se detienen antes de
aceptar datos que lo excedan.

Para reportar una vulnerabilidad, usa la opción **Report a vulnerability** de
la pestaña Security de GitHub si está habilitada para este repositorio. Si no
está disponible, contacta al mantenedor en privado antes de publicar detalles
técnicos. Incluye pasos de reproducción y el impacto; no adjuntes secretos
reales.
