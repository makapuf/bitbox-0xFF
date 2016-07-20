# CAUTION : **WORK IN PROGRESS**

# 0xFF : use paint to make games.

Having a mini DIY game console to play games on is very cool and making games on it with nothing but your C compiler is a great feeling. However, I found that some players would like to create games on it but lack the know-how (or the patience) of setting up a build chain, learn C, gdb, the bitbox SDK and game dev (while all very simple steps if you know the majority of them, having to go through all of them at once is a little daunting)... So I wanted to made a project to let people be creative and start making some simple games quickly, while : 

 - Requiring few if no programming to start doing something
 - Ability to customize and edit the game mostly graphically / visually
 - Being compatible with many tools (ie not needing special uncommon tooling).
 - Be simple to modify a game, transmit it and play it !

0xFF is a project I created to cover this. The idea is to create a side-platformer/shooter/.. game engine using a single 256x256 8bpp image as game data (hence the name, since 0xFF is 255 in hexadecimal and all data in those games can be expressed as a number between 0 and 255 : colors, positions x and y, tiles, ...), which is able to run on a PC, a Bitbox console (loading from microSD) as well as a micro bitbox (embedding several games in the internal flash).

The kind of games we're talking about here are [tile-based games](https://en.wikipedia.org/wiki/Tile-based_video_game).

This idea of a game encoded on an image is not mine, it originates from the [SIP](http://siegfriedcroes.com/sip/) (Single Image Platformer) idea by Siegfried Croes.

## Further reading

Let's see how a whole game can be coded in an image in 0xFF in the [Tutorial](TUTORIAL.md) , or check the [Reference](REFERENCE.md)


![level](0xff_level.png)

![demo](0xff_demo.gif)

