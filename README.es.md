# MundoCraft para PS4

[English](README.md) · **Español**

![MundoCraft para PS4: ilustración del mundo y la consola](assets/banner.svg)

![Alpha experimental](assets/alpha-badge.svg) ![PS4 homebrew](assets/platform-badge.svg) ![Luanti 5.17.0](assets/engine-badge.svg) ![Licencias por componente](assets/license-badge.svg)

**v0.3.19-alpha1** · **APP_VER 01.19**

MundoCraft es un cliente de Luanti adaptado para PlayStation 4. Esta versión es experimental. El PKG de esta distribución ya se compiló y validó localmente; todavía requiere su propia prueba en PS4. La experiencia previa con el motor 01.19 no sustituye esa prueba del paquete final.

Está basado en el motor libre [Luanti](https://www.luanti.org/), bajo LGPL-2.1-o-posterior,
desarrollado por su comunidad. Este port es modificado y no oficial; las fuentes
correspondientes y los avisos de terceros acompañan cada versión.

El cliente se conecta a servidores Luanti. Cada servidor tiene su propia identidad y progreso. Para jugar en un mundo local, el servidor debe estar encendido. El juego y sus recursos se obtienen del servidor al que te conectas; Mineclonia no viene incluido en el PKG.

MundoCraft no es Minecraft, no es compatible con mundos o servidores de Minecraft Java o Bedrock y no usa cuentas de Minecraft. No está afiliado con Mojang ni Microsoft.

<p align="center"><img src="client/assets/mundocraft-icon0.png" width="120" alt="Icono ilustrado de MundoCraft: isla de bloques y árbol"></p>

## Primeros pasos

- [Instalar en PS4](docs/INSTALLING.md)
- [Controles y juego](docs/USAGE.md)
- [Compilar desde las fuentes](docs/BUILDING.md)
- [Qué se probó y qué falta probar](docs/TESTING.md)
- [Cómo está organizado el cliente](docs/ARCHITECTURE.md)
- [Licencias, créditos y procedencia](THIRD_PARTY_NOTICES.md)

Las descargas de esta versión están en [Releases](https://github.com/giruai/mundocraft-ps4/releases). Comprueba el SHA-256 del PKG con el archivo `SHA256SUMS` adjunto a esa misma versión.

## Estado de esta versión

Una compilación anterior del motor 01.19 se probó en privado con Mineclonia 0.124.1: registró una identidad en una partida LAN creativa, rechazó una contraseña incorrecta y permitió volver a entrar tras reiniciar el servidor. Se comprobó movimiento, cámara, inventario, romper y colocar bloques, pausa y salida normal al menú de PS4; la posición y el inventario persistieron.

Esa prueba no usó el PKG de esta distribución. El juego público de la lista de servidores no llegó a mostrarse; la lista pública y la consulta de geolocalización sí funcionaron en una prueba anterior. Las sesiones largas de supervivencia, fabricación, combate y sonido todavía no están validadas. El sonido está desactivado.

Consulta [TESTING](docs/TESTING.md) para distinguir esas pruebas del estado del artefacto de esta versión.

## Fuentes y privacidad

El archivo de fuentes de cada versión incluye el código correspondiente, las modificaciones del port, las dependencias y sus licencias. El SDK de PS4 no se incluye: la guía de compilación indica cómo obtener las entradas bloqueadas desde sus fuentes.

El proyecto no incluye instrucciones para modificar la seguridad de la consola. Para instalar el PKG necesitas una PS4 que ya pueda ejecutar paquetes homebrew y un instalador de PKG que ya tengas disponible.

## Créditos

MundoCraft usa y adapta Luanti y otras bibliotecas de terceros. Consulta [Licencias](LICENSE), [CREDITS.md](CREDITS.md) y [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) para las licencias, avisos y procedencia del código.

La marca PlayStation pertenece a Sony Interactive Entertainment. Luanti es un proyecto independiente. Las marcas de terceros se mencionan únicamente para describir compatibilidad y procedencia.
