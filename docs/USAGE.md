# Controls and gameplay

Controls are listed here using PlayStation button names. Assignments may vary by screen; follow the prompts shown in the menu.

| Button | Action |
| --- | --- |
| Left stick | Move |
| Right stick | Look around |
| Cross | Jump |
| Circle | Sneak in game; go back in menus |
| Options | Pause or resume |
| Triangle | Open inventory; when a text field is focused, open the on-screen keyboard |
| L1 / R1 | Change the selected hotbar slot |
| R2 | Break or mine the pointed-at block |
| L2 | Place the selected block |
| Square | Auxiliary action in the world; secondary click in inventory |

Focus a text field before opening the on-screen keyboard. Use the keyboard to enter a player name, password, or server address. **Done** confirms the field with the same Enter event as a physical keyboard; a form may submit or close immediately. **Cancel** restores the text from before the keyboard opened. In Mineclonia's inventory search, Done applies the entered filter.

In an inventory, **Cross** picks up or moves the whole stack. With an empty cursor, **Square** takes the larger half of a stack; while carrying a stack, Square places one item into the pointed-at slot. These are the engine's normal secondary-click operations. Recipe and inventory rules still belong to the server/game. The exact 0.3.20 build passed PS4 checks for search, splitting 64 into 32/32, placing one item and arranging a two-cell recipe.

## Sound

The 0.3.20 build enables OpenAL Soft, Ogg/Vorbis decoding and SDL2's native PS4 audio output. Fresh settings enable sound at 60% volume. Existing user settings are preserved: check Settings → Sound if previously muted. Sound was heard in the PS4 test, mute/unmute changed the captured audio signal, and audio returned after reconnecting. Long-session timing and every output-device setup remain unmeasured.

## Servers and progress

Each server maintains its own accounts and worlds. Use the same name and password to return to the same account on a server. Progress is not automatically shared between servers.

The 0.3.20 public test used Luanti's existing unified login/registration mode (`enable_split_login_register = false`) temporarily. The default separate double-password registration form has a known code-review concern with this on-screen keyboard: Done sends Enter before both password fields may be filled. That form still needs a correction; existing-account login and the unified flow were exercised. The original user settings were restored after testing.

Pausing locally does not stop the world clock on an online server. If you need the world to stop, disconnect or ask the server administrator.

To play the tested LAN mode, start the compatible server first, then connect from MundoCraft. The server provides the game resources, including textures and Mineclonia content. The client retains Luanti's embedded local-server code, but this PKG bundles no game and offline world startup has not been validated on PS4.

## Exit

Use Circle to go back through menus and the client's Exit to OS option to return to Home normally. Both the published 0.3.19-alpha1 PKG and the exact 0.3.20 beta binary passed that check.
