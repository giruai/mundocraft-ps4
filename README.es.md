# MundoCraft para PS4

[English](README.md) · **Español**

![MundoCraft para PS4: ilustración del mundo y la consola](assets/banner.svg)

![Versión beta](assets/beta-badge.svg) ![PS4 homebrew](assets/platform-badge.svg) ![Luanti 5.17.0](assets/engine-badge.svg) ![Licencias por componente](assets/license-badge.svg)

**Beta v0.3.20-beta1** · **APP_VER 01.20**

MundoCraft es un cliente de Luanti adaptado para PlayStation 4. Esta versión es experimental. Esta versión añade sonido, confirmación de búsqueda y división de pilas con el mando. **La beta distribuye el mismo PKG exacto 0.3.20 probado en PS4 por LAN y en un servidor público de Mineclonia. El mantenedor también confirmó que sus pruebas de estabilidad concluyeron con éxito.** La promoción a beta no cambia el binario: conserva sus etiquetas internas alpha. Ver el [registro de compilación](docs/BUILD_REPORT_0320.md).

Está basado en el motor libre [Luanti](https://www.luanti.org/), bajo LGPL-2.1-o-posterior,
desarrollado por su comunidad. Este port es modificado y no oficial; las fuentes
correspondientes y los avisos de terceros acompañan cada versión.

El cliente se conecta a servidores Luanti. Cada servidor tiene su propia identidad y progreso. En la modalidad LAN probada, el servidor debe estar encendido. El juego y sus recursos se obtienen del servidor al que te conectas; Mineclonia no viene incluido en el PKG.

MundoCraft no es Minecraft, no es compatible con mundos o servidores de Minecraft Java o Bedrock y no usa cuentas de Minecraft. No está afiliado con Mojang ni Microsoft.

<p align="center"><img src="client/assets/mundocraft-icon0.png" width="120" alt="Icono ilustrado de MundoCraft: isla de bloques y árbol"></p>

## Primeros pasos

- [Descargar v0.3.20-beta1](https://github.com/giruai/mundocraft-ps4/releases/tag/v0.3.20-beta1)
- [Instalar en PS4](docs/INSTALLING.md)
- [Controles y juego](docs/USAGE.md)
- [Compilar desde las fuentes](docs/BUILDING.md)
- [Qué se probó y qué falta probar](docs/TESTING.md)
- [Cómo está organizado el cliente](docs/ARCHITECTURE.md)
- [Licencias, créditos y procedencia](THIRD_PARTY_NOTICES.md)

Las descargas de esta versión están en [Releases](https://github.com/giruai/mundocraft-ps4/releases). Comprueba el SHA-256 del PKG con el archivo `SHA256SUMS` adjunto a esa misma versión.

## Estado de esta versión

El PKG publicado exacto de 0.3.19-alpha1 se probó con Mineclonia 0.124.1 en LAN creativa el 24/10/2026: 60 minutos y 18 segundos de observación conectada sin cierres ni desconexiones detectados. Funcionaron mover pilas completas, equipar armadura, colocar y romper bloques y una receta sencilla. La posición, orientación e inventarios coincidieron después de reiniciar el servidor y entrar con la misma cuenta y contraseña. El contador interno suavizado indicó 47–58 FPS, habitualmente 58, con picos de variación; no demuestra 60 FPS estables.

La beta 0.3.20 habilita la cadena de audio, hace que «Done» confirme los campos de texto y añade Cuadrado como clic secundario: tomar media pila o colocar una unidad. Pasó pruebas locales y una prueba del PKG exacto en PS4: sonido audible, silenciar/reactivar, búsqueda, pilas, receta de dos casilleros, persistencia tras reiniciar el servidor y salida normal a Home. Fueron unos 17 minutos de ejecución de la aplicación; no fue una prueba prolongada ni una medición de FPS. Una sesión funcional posterior también cargó un mundo público de Mineclonia y comprobó inventario, fabricación y reingreso con contraseña conservando visualmente los objetos; los números exactos de las pilas públicas no fueron legibles de forma independiente en las capturas de Remote Play. El mantenedor confirmó después el éxito de sus pruebas de estabilidad, sin aportar duración ni mediciones detalladas. La progresión completa de supervivencia, combate y arranque de mundos offline no quedan cubiertos por las pruebas funcionales registradas. El formulario separado de doble contraseña sigue pendiente; ver [uso y cuentas](docs/USAGE.md). El motor conserva código de servidor integrado, pero el PKG no incluye un juego. El formulario separado de registro con doble contraseña aún necesita revisión para el teclado en pantalla; la prueba pública usó temporalmente el modo unificado de ingreso/registro.

Consulta [TESTING](docs/TESTING.md) para el alcance y los límites de cada prueba.

## Fuentes y privacidad

El archivo de fuentes de cada versión incluye el código correspondiente, las modificaciones del port, las dependencias y sus licencias. El SDK de PS4 no se incluye: la guía de compilación indica cómo obtener las entradas bloqueadas desde sus fuentes.

El proyecto no incluye instrucciones para modificar la seguridad de la consola. Para instalar el PKG necesitas una PS4 que ya pueda ejecutar paquetes homebrew y un instalador de PKG que ya tengas disponible.

## Créditos

MundoCraft usa y adapta Luanti y otras bibliotecas de terceros. Consulta [Licencias](LICENSE), [CREDITS.md](CREDITS.md) y [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) para las licencias, avisos y procedencia del código.

La marca PlayStation pertenece a Sony Interactive Entertainment. Luanti es un proyecto independiente. Las marcas de terceros se mencionan únicamente para describir compatibilidad y procedencia.
