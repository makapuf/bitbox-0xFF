# A Side project

Having a mini DIY game console to play games on is very cool, but I found that some players would want to create games on it but lack the know-how of setting up a build chain, learn C with no IDE, gdb, the bitbox SDK (while all very simple steps if you know the majority of them, having to go through all of them at once is a little daunting)... So I wanted to made a project to let people be creative and start making some simple games quickly, while : 

 - Requiring few if no programming to start doing something
 - Ability to customize and edit the game mostly graphically / visually
 - Being compatible with many tools (ie not needing special uncommon tooling).
 - Be simple to modify a game, transmit it and play it !

Sideproject is a project I created to cover this. The idea is to create a side-platformer/shooter/ game engine (hence the name) using a single 256x384 8bpp image as game data, which is able to run on a PC, a Bitbox console (loading from microSD) as well as a micro bitbox (embedding several games in the internal flash).

(ref to SIP)


Let's see how is a whole game coded in an image in sideproject.


Generic format and setting your image editor
=============================================


The image must be saved as a 256x384 pixels 8bpp - 256 color, bmp, non rle-compressed file. This simple format is used to simplify decoding on a microcontroller.

The palette used is fixed and must be the bitbox micro palette. If you use another 256 color palette or modify the palette another palette, it will be discarded on load and the programs will use the bitbox micro palette.


XXX insert micro palette


Even if any editor can be used, setting up your image editor for work is quite important.


The important steps to create a blank image are:

 - Create a blank 256x384 pixels image
 - Use indexed color mode. The image format uses a palette which by definition is an indexed color mode. You MUST use precise colors ids sometimes and not nuances, so this is important. On GIMP by example, use the Image/mode/indexed colors using a personalized palette. Select bitbox micro palette, Don't remove unused colors, and keep the colors in order (this is important).
 - Open the indexed palette window (which is NOT the color palette window on gimp)
 - Display a 16x16 grid as a guide
 - Use the pencil tool (type N under gimp) , not the brush to have sharp edges (you will not


(XXX screenshots on gimp)


I presented GIMP here but there are many graphical editors capable of working with indexed colors, including photoshop. 

Header
=======

XXX header /main image position


The header is the top zone made of a rectangle of 256x128 pixels. It will be loaded as a title for your game and you can put whatever you want - it will not be interpreted. It is used exclusively for the pre-game / loading stage and will be completely ignored once entering the game.

Main area
==========

From now on we will start in the game and will only be referring to the main part of the image-game.

This main area is a 256x256 pixels image, made of 16x16 squares of of 256 values pixels.


     XXX main area+16x16 tiles numbers


The 16x16 squares will be called tiles. There are 256 of them, 16 lines of 16 tiles. We will count them from 0 (top left) to 255 (bottom right).


The main area will be used to encode the sprites, and four levels, themselves made of a mosaic of tiles, level behaviour, and positions of different enemies and their behaviours.

The tiles are taken from a  tileset, arranged in a tilemap per level.

Music and object behaviours is also encoded with pixels.

Tileset
========

The tileset is simple and a little limited (we have to use 64k pixels for everything !).

4 lines of 16x16 tiles are used, which allows 64 tiles. Those tiles are separated between 48 fixed-function tiles (ie tiles which, given their positio, have a precise usage) and 4x4 generic tiles (of which you can choose their behaviour).



     XXX find tiles + structure



Fixed tiles encode several regions of the screen, by defining borders and main regions :

- sky regions, empty background; no behaviour

- decor regions, which are like sky regions but are use for decorative purposes 

- blocking regions (where the player or "normal" enemies cannot go)

- killing regions : static tiles which, if touched, kill the player (spikes, lava, ... )



The 16 versatile tiles can have sky, kill or blocking behaviors, as well as many other special ones as will be shown after (ice, trampoline, ...).


Levels tilemaps
===============

The Tilemaps for a given level can be anywhere on the main region. They are found using the minimap, a special 16x16 tile on the lower left corner (tile id 240). More on that later.

On level tilemaps, you will specify **terrains** and **tile ids**. Terrains are different from tiles. They only mark the behaviour and not the specific tile ID. By example, for a block terrain as a ground tile, you will have different tiles : the main dirt tile (with maybe a variation of it), the grass which transition to the sky, the corners, ...

A **mapper algorithm** will interpret the terrains and transform them to tile_ids. It's not different than specifying , it's just simpler. The mapper algorithm is chosen by the color of pixel representing the minimap .. in the minimap, namely the bottom-left pixel of your whole image. 

 - color TRANSPARENT (230, a flashy pink) means no interpretation is done (apart seeing the pixel types mod 64)
 - color BLACK paints tile_id as a platformer would
 - color WHITE paints ```XXXX```

Terrains can be of three types :

- standard terrains (the one we have already seen : sky, decor, block, kill)
- other types of terrains 
- object_types 0-31, which mark the initial position of future objects
(object types are matched by being defined in the object header tile)


Minimap
=======

The minimap is located at the bottom left tile. It defines what each tile of your image is made of. Since you have 16x16 tiles in your and that a tile is 16x16, that means each tile is represented by a single pixel.
For tiles, the color in it defines the terrain it's using.  

> **Note** that if you use a mapper algorithm the resulting terrain can be different of your level defined terrain since you'll make level terrains → mapper → level tile_ids → minimap → level terrains. 


Object Types
============

Object types are defined in the object map tile which is defined just at the right of the Minimap. You will find object definitions for your whole game here, by two columns of 8 pixels, for each line, which make it 16x2=32 objects.

In fact, the 4 first ones define the level-specific constants, and then the next ones defined object types 0-25 (28 = 32 objects - 4 levels)


### Level-lines : positions 0-3
- position 0 defines the color of the level in the minimap.
- position 1 defines the player sprite for this level
- position 2 defines the player speed (%16, horizontal pos on palette), and jump height (/16, Vspeed). 240 / bottom left of the palette is no movement. 241 is speed 1 , no jump, is speed 0 (no move), only jump



### Object-lines

- position 0 is the position of the first frame of the animation of that sprite
- position 1 is the movement type
- position 2 is the player hit type
- position 3 is the id of the killing spawn sprite 0-26 (ie when killed, spawns this) - note that disappearing does not count as being killed
- 


Note that the two last object lines define 
- (id ) an explosion sprite that is launched at tile when user kills an enemy
- (id ) an magic FX when user gets something
They are defined as sprites but are expected to be of simple movement modes, no collision.