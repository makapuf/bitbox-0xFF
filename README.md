# CAUTION : **WORK IN PROGRESS**

# 0xFF : use paint to make games.

Having a mini DIY game console to play games on is very cool and makinf games on it with nothing but your C compiler is a great feeling. However, I found that some players would like to create games on it but lack the know-how (or the patience) of setting up a build chain, learn C, gdb, the bitbox SDK and game dev (while all very simple steps if you know the majority of them, having to go through all of them at once is a little daunting)... So I wanted to made a project to let people be creative and start making some simple games quickly, while : 

 - Requiring few if no programming to start doing something
 - Ability to customize and edit the game mostly graphically / visually
 - Being compatible with many tools (ie not needing special uncommon tooling).
 - Be simple to modify a game, transmit it and play it !

0xFF is a project I created to cover this. The idea is to create a side-platformer/shooter/.. game engine using a single 256x256 8bpp image as game data (hence the name, since 0xFF is 255 in hexadecimal and all data in the game can be expressed as a number between 0 and 255), which is able to run on a PC, a Bitbox console (loading from microSD) as well as a micro bitbox (embedding several games in the internal flash).

The kind of games we're talking about here are [tile-based games](https://en.wikipedia.org/wiki/Tile-based_video_game).

This idea of a game encoded on an image is not mine, it originates from the [SIP](http://siegfriedcroes.com/sip/) (Single Image Platformer) idea by Siegfried Croes.


Let's see how a whole game can be coded in an image in 0xFF.


![level](0xff_level.png)

![demo](0xff_demo.gif)



Generic format and setting your image editor
=============================================


The image must be saved as a 256x256 pixels 8bpp - 256 color, bmp, non rle-compressed file. This simple format is used to simplify decoding on a microcontroller. Larger images with an header of 256x128 (with a total of 256x384) can also be used.

The palette used is fixed and must be the bitbox micro palette. If you use another 256 color palette or modify the palette, it will be discarded on load and the engine will use the bitbox micro palette.


***XXX insert micro palette image XXX ***


Even if almost any image editor can be used, setting up your image editor for work is quite important.


The important steps to create a blank image are:

 - Create a blank 256x256 pixels image
 - Use indexed color mode. The image format uses a palette which by definition is an indexed color mode. You MUST use precise colors ids sometimes and not nuances, so this is important. On GIMP by example, use the Image/mode/indexed colors using a personalized palette. Select bitbox micro palette, Don't remove unused colors, and keep the colors in order (this is important).
 - Open the indexed palette window (which is NOT the color palette window on gimp)
 - Display a 16x16 grid as a guide
 - Use the pencil tool (press key "N" under gimp), not the brush, to have sharp edges (also set your eraser to have sharp edges). Also set your brush size to 1 pixel wide, it's pixel art we're talking about.


***(XXX screenshots on gimp)***


I presented GIMP here but there are many graphical editors capable of working with indexed colors, including photoshop. 

> **NOTE : The transparent color** is the color 230 on the bitbox micro palette (#FF00DA). This is the color referred to define "empty" as there is no alpha channel, we'll be using this horrid color that you will rarely use on your sprites. If you *really* want to use this ugly pink, you can find nearby colors almost as flashy as this one on the palette.

Main area
==========

This main area (as opposed to the optional title zone defined afterwards) is a 256x256 pixels image, made of 16x16 squares of of 256 values pixels.



The 16x16 squares will be called tiles. There are 256 of them, 16 lines of 16 tiles. We will count them from 0 (top left) to 255 or 0xff in hex (bottom right).

     XXX main area+16x16 tiles numbers

The image will be used to encode four levels, themselves made of a mosaic of tiles, the sprites, level behaviour, and positions of different enemies and their behaviours.

Music and object behaviours will also be encoded with pixels on tiles.

Levels
======

Levels are made of tiles, with tiles coming from a *tileset* and positioned on your different levels with a *tilemap* for each level. They also represent where we will find the "sprites", ie the animated objects, such as the players, the enemies, ... 

The first thing the engine will need to interpret your image is where to find the tilemap of the levels on the image. So we will provide a map of the image. The map is called the *minimap*.

> **Minimap**
> The minimap is located at the bottom left tile (tile number 0f). It defines what each tile of your image is made of. Since you have 16x16 tiles in your image and that a tile is 16x16, that means each tile is represented by a single pixel on the minimap. Use the position of a pixel to get the position of a tile in the image.

There are 4 levels in a game. The color used on the minimap to represent each level is defined in the tile just on the right of the minimap (F1) and are the first pixels of the four first lines of this tile ```f1```. We will detail the rest of the tile f1 afterwards.

To define a level, first paint with the color to represent it (pick up one) on the minimap. You will then use those tiles you designated on the minimap to paint the level tilemaps. 

So if you take color 0 (black) for the first level (set the first pixel of the tile on the right of the minimap to zero), and you painted the first row of the minimap black (color 0), you can draw the first level of your game on the first 16 tiles (00 to 0F) of your image.

Tilemaps
========

The tiles on the level  tilemaps are specified by their tile number. To set a tile reference to a tile on your image, take its tile number on the image (00 to FF), take the color corresponding to this number and paint your number with it. 

By example, if you paint a rock on the tile on the first tile of the fourth line (numbered 0x40 or 64 in decimal), just put on your tilemap the pixel of the color 64 of the palette (which is a light green). (BTW, you can see that this color 0x40 it's also the first color on the fourth line of your palette, i.e. the position in the palette matches the position of the tile on your image). 

You can use that to specify every tile of your levels. but how can we specify how the tilemap will react ?


Tilesets and Terrains
=====================

As we saw, the tileset will be defined by drawing tiles on your image and referencing those by their color Id on the level maps. But what about their behavior ? What defines that a ground can be stood on, lava kills, you can fall through the air and that the mountain in in the background ?

Well, each on your image used as a tile for your game will be referenced by the minimap, and we will encode its behaviour with different colors. 

Back to our example, say that our tile 0x40 which we represented as a rock is *solid*, we will paint its corresponding pixel on the minimap (ie the 1st pixel of the 4th line) with the color for solid objects : 104, which is a kind of brown (think "ground"). Empty tiles (or background ones), which do not stop the player are marked with 87, which is blue (think "sky"). This is a convention and will always be the case. A reference is given hereafter.

Those behaviours (solid, sky, ..) are called **terrains** .

> This means that you cannot use those predefined *terrains colors* for your *levels colors* on your minimap. Life is though.


Now you can draw your first tiles and then compose them into tilemaps for your levels. 

But we don't have any enemies and no way to tell how your level *plays*. Well, that'll be the next step with *object types* 


Object Types
============

Object types are defined in the object map tile which is defined just at the right of the Minimap (where we defined our level color for the minimap, remember ?). You will find object types definitions for your whole game here, an object type being defined as 8 pixels horizontally. The tile defines two columns of 8 pixels, for each line, which make it 16x2=32 objects.

In fact, the 4 first "objects" define the level-specific constants, and then the next ones define object types 0-25 (28 = 32 objects - 4 levels)


### Level-lines : positions 0-3
- position 0 defines the color of the level in the minimap.
- position 1 defines the player sprite for this level
- position 2 defines the player speed (%16, horizontal pos on palette), and jump height (/16, Vspeed). 240 (bottom left of the palette) is no movement. 241 is speed 1 , no jump, is speed 0 (no move), only jump (which CAN be useful) ...

### Object-lines

- pixel 0 encodes the position of the first frame of the animation of that sprite graphics on the image. 
- position 1 is the movement type (it also defines the number of frames used for the movement animations)
- position 2 is the player hit type (kills the player, gives a life ..)
- position 3 is the objecttype id of the spawned sprite 0-25 (ie when this object is killed, spawn an object of this kind in place).

# HUD
The HUD is used to present to the user the current status : level of lives, score and keys. Letters are encoded as 8x8 minitiles (4 in a tile). They are specified as 5 tiles (20 minitiles) on the bottom of the image, see the examples.

Extras 
=====
## Bitmap Header

The title space is an optional 256x128 header on the top of your image, which will not be used at all during the game but can be used to present your game with a simple bitmap. you resulting game image will thus be 256x384 pixels.

## Title level
The title level is a tile just above the minimap, which will be used to define a simple 16x16 mini-level (using the same object types) which will be sued as a title if your game image is 256x256. Note that this level will not be playable, will have no scrolling and will just serve as illustration of your game.

##Mappers

Instead of having to remember and set every tile id individually, which can grow tiresome, you can use *mappers*. Mappers are a way to draw your tilemaps not using tile_ids, but terrains.

On level tilemaps, you will specify **terrains** (and also often individual **tile ids**). Terrains are different from tiles. They only mark the behaviour and not the specific tile ID. A **mapper** will then transform those at loading stage to tile_ids.

By example, with sky and ground terrains, you will have a ground tile and a sky tile, but also different transitions : the grass which transition to the sky, the corners, alternate tiles for the ground ...

The **mapper algorithm** will interpret the terrains and transform them to tile_ids. The mapper algorithm is chosen by the color of pixel representing the minimap .. in the minimap, namely the bottom-left pixel of your whole image. 

This pixel can be : 
 - color TRANSPARENT (230, a flashy pink) means that no interpretation is done - this is the no-op mapper.
 - color BLACK paints tile_id as a platformer would. **TODO** explain mapper algorithm.
 - color WHITE **XXX**

> **Note** that if you use a mapper the resulting terrain of the generated tiles can be different of your level-defining terrain since you'll make level terrains → (mapper) → level tile_ids → (minimap) → level terrains. 

### The Transparent mapper

Transparent mapper is a no-op mapper. Use this if you define directly the tile ids on your tilemaps with their respective color.

### The Black mapper

The Black mapper can be used for side platformers. With this mapper, a few conventions must be followed : 

**The tileset** is fixed : 4 lines of 16x16 tiles are used, which allows for 64 tiles. Those tiles are separated between 48 fixed-function tiles (ie tiles which, given their position, have a precise usage) and 4x4 generic tiles (of which you can choose their behaviour).


***XXX explain tiles + structure***

Fixed tiles encode several regions of the screen, by defining borders and main regions :

- sky regions, empty background; no behaviour

- decor regions, which are like sky regions but are use for decorative purposes 

- blocking regions (where the player or "normal" enemies cannot go)

- killing regions : static tiles which, if touched, kill the player (spikes, lava, ... )


The 16 versatile tiles can have sky, kill or blocking behaviors, as well as many other special ones as will be shown after (ice, trampoline, ...).

# Music and SFX

Music will be encoded also as tiles. We will be using the chiptune engine of the bitbox but encoded as colors. The chiptune engine  is based on LFT as ported by @pulkomandy and mostly works with macros : instruments are macros to trigger voices oscillators and then patterns encode notes.

###Instruments
Your game will encode instruments in a tile, with 16 instruments of 16 macro steps. They will also serve as SFX definitions.
### Patterns 
A pattern is 16 steps like a mini piano roll
### Songs 
Whole songs can be specified by level. A song references a list of patterns by its pixel to tile mapping. A tile on the right of the pixel can be used to define a 

# Reference
### positions
### terrains
### object movements
### object collisions
### black mapper terrains


