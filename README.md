# Elevator DS - Port of my Godot game "Elevator" to the Nintendo DS

Written in C using devkitpro + libnds + maxmod.

Youtube devlog: [https://youtu.be/tmKcgSL6wPA?si=uZiMQEB-dOOpAfwF](url)

![gameplay](https://github.com/jpcerrone/elevator-nds/blob/master/ReadmeDemo.gif)

## This port serves as an example for doing some cool things in libnds like:
* Sprite Palette swapping.
* Utilizing all background layers in both tile mode and bitmap mode.
* Saving and loading from a flashcard.
* Scene transition effects.
* Virtual touchscreen buttons.
* Using GRIT to unify the palettes of every background.
* Using different Makefile configurations for backgrounds and sprites.
* Drawing numbers without using fonts.
* Using maxmod with sfx and .it tracker files.
* Using timers.

## Compiling and Running instructions
* Install devkitpro (Tested with 3.0.3), open up its development console on this repository's folder, and run the 'make' command.
* Copy the elevator.nds file into a flashcart or open it up with an emulator. Tested with No$GBA.

## Credits
m6x11Font by Daniel Linssen https://managore.itch.io/m6x11

Game and port by Juan Cerrone

If you like my work you can follow me here:
* https://www.youtube.com/user/jpcerrone (Music/Programming)
* https://www.instagram.com/juan.cerrone.pixel.art/ (Pixel Art)
* https://open.spotify.com/artist/6H2QtCjF5ANcG7PhiRWNCq?si=3t06lhQ2RVm1r3B7c1vHDA (Spotify)
