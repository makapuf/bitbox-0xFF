# Reference

Elements with no id are not implemented yet.

In a given position, the pixels can represent :

 - a color (transparent being color 240)
 - a number (0-255 or 0xFF, the number of the color in the palette, 0 being black and 255 white)
 - a reference to a tile : the number of the color is the number of the tile from left to right, top to bottom. note that the place of the color in the palette arranged in a 16x16 grid is the same as the position of the tile in the main area.
 - a 2D field. The pixel color is placed in X,Y position on the palette. By example, black is (0,0) and white (15,15). color 81 = 5*16+1 -> x=1,y=5

### levels 

description of the level : each id is an index on the level description bytes

level | property(0-7) | comment
----------|----------|---------
color     | 0 | color of the terrain on the minimap
player_color | 1 | color of the player on the level map
control   | 2 | see control types
accel   |  3 | X/Y (acceleration/gravity) as a 2D vector pixel. depends on controls
maxspeed   | 4  | X/Y max speed as a 2D vector pixel. depends on controls
altaccel | 5 | X/Y alt acceleration / gravity (see controls)
altmaxspeed   | 6 | running / jumping X/Y max speed as a 2D vector pixel. depends on controls
maxtime | 7 | must finish level before Nx2 seconds. If set to zero, just records it


### Control types

Player control type

control | id | description / controls / speeds & accels
-----|----|----------------------
classic   |  0  | standard controls : can (optional) run on pressing B, jump on pressing A, X fires (if available), L/R changes projectile type if several gained. AccelX=Walk, AccelY=LadderAccel, MaxSpeedX=Walk max speed, MaxSpeedY=LadderVSpeed. AltAccel_X=Run Accel, AltMaxSpeed_X=Run speed.  AltAccel_Y = gravity, AltMaxSpeed Y=fall/jump speed.
side | 100 | player can control X and Y (no gravity for player), always faces the same direction. No jumping/running. Autoscroll is given by alt speed.
modern | 249 | can double/wall jump, stomp vertically on enemies (like hitting them)
runstomp | | can charge by keeping B pressed while not moving then when ready runs, can stomp enemies forward by running (like hitting them) 
runner | | always goes right at average speed, can jump, sometimes fire
aim | | can go left+right, aims by 45 degrees increments 
hit |  | left/right ; B: run ; Y : hits enemies (horizontally or stomping) : 2 more sprites frame for hitting H/V
beatemup | | can jump on ladders, falls back where we are (default)
nojump | | cannot jump (nor run)
infjump | 255 | player has infinite jumps


### Terrains

Terrain types. ie how a terrain tile behaves.

terrain | terrain_id | description
------| -------| -----
empty | 87 | empty, does not interact with player
animated_empty | 86 | 4 frames animated but behaves like empty. TileID will be +1 % 4 each 32 frames
obstacle | 104 | blocks user from left or right  
kill | 240 | kills when touch it 
anim_kill | 168 | idem but animated
ladder | 147 | can go up, down even with gravity
ice | 151 | cannot stop on X, but can jump. 
platform | 136 | cannot fall but can go through up or sideways
jump |    | makes the player automatically jump 
animated_touch |  | animated once, transforms when user touches it

> You can also use any color defined as an enemy color.
> Remember that on tilemap, reference 255 is the level start (defaulting to top left if not found).

> TODO : make terrains testable by bits ?? by %8 ? 

### Object Types

Indices in object type definition bytes. 8 colors defined max.

 property | position | comment 
--------- | --------- | -------
 color |0| color to prepresent object on tilemap, as well as id of the top left of the first tile for the sprite frames. 230 (transparent) if this object type is undefined
 movement |1| type of movement (see object movements)
 collision |2| type of collision - sprite_collide (see object collisions table)
 spawn | 3 | type of sprite spawned when this object dies (0-15) TODO COLOR ! 
 alt_collision | | alt collision type (does not influence respawning)
 alt_collision_when | | condition where alt collision is triggered See alt_collisions


### Movements

Object types movements.

mov | color id | comment 
----------- | --------- | ----------------------
nomove | 230 | (transparent) static, by example gives a bonus once touched. 1 frame
alternate1 | 7 | (bright blue) no move, just alternating 2 frames each 16 frames
alternate2 | 39 | (bright blue) no move, just alternating 2 frames each 32 frames
alternate3 | 71 | (bright blue) no move, just ping-ponging 3 frames (ABCBA ..) each 16 frames	
alternate4 | 103 | (purple) no move, just cycling 4 frames each 16 frames	
throbbing | 25 | (bright green) static going up and down one pixel (to be better seen). 1 frame.
singleanim4 | 107 | (grey) 4 frames animation then destroy sprite
singleanim2 | 75 | (blueish grey) 2 frames animation then destroy sprite
singleup8 | 128 | goes up slowly for a moment then disappear, 1 frame
flybounce | 11 | flies 45° (starts up/left) but bounces on walls. state : current speed vector
walk | 3 | walks and reverse direction if obstacle or holes. 2 frames
bulletL | 8 | flies, not stopped by blocks, no gravity, right to left. 1frame
bulletR | 9 | flies, not stopped by blocks, no gravity, left to right. 1frame
walkfall |  | subject to gravity, left and right if obstacle, falls in holes. 2 frames
walkjump |  | walks but jumps every 2s. 2 frames 
sticky |  | walks on borders of blocking sprites, will go around edges cw. 2frames
sticky_r |  | walks on borders of blocking sprites, will go around edges ccw. 2frames
vsine4 |  | vertical sine, 4 tiles height. 2frames alternating every 16frames
bouncing | | bounces with gravity on the ground, 4 tiles high
bulletD |  | flies, not stopped by blocks, no gravity, goes down. 2frames
bulletLv2 | 11 | flies, a bit faster than preceding
bulletRv2 |  | flies, a bit faster than preceding
generator |  | does not move, generates each ~2 seconds enemy with id just after this one. 2fr 
ladder |  | stays on ladders. right to left, go back to right if finds border. 1 frame alternating
player | 255 | (white) implied for first object (??)
walkdouble | | walk but is 2x the size of its pixels (actually rendered differently)
minibulletL | | 4 mini tiles per tile. fly horizontally, die on walls 
minibulletLD | | 4 mini tiles per tile. fly oblique, die on walls 



### Collisions

Object collision types : what happens when an object of this type collides with the player.

col | color id | comment
------ | ------ | ------
none | 230 | no collision - respawns
kill | 240 | red (==terrain_kill) kills player instantly - respawns
killnorespawn | 237 | kills player instantly
block | 104| (=terrain obstacle) blocks the player - can push it
coin | 249 | yellow, gives a coin - or Nb =next ? , 50 of them gives a life
life | 25 | green, gives a life 
key |  137 | gives a key
end | 255| ends level 
switch | | transform next object in list into its next object (ex. if object in pos 3, objects 4 get transformed in objects 5 and reciprocally)
three_keys | | switch (next types...) only if you've got 3 keys - removes them
fire | | allows firing of projectile of type defined just after
restart | | the object dies once touched. the player will respawn here once touched.

### Alt Collisions

When is the alt collision triggered (and the standard collision by exclusion)

altcol | color_id | comment
-------|----------|-------------
none   |   230 | always standard collision
top   |   | when touches top
bottom |    | when touches bottom
side |   | when touch side
punched |  | when punched or run into
projectile |  | when a projectile touches it
punched_up |    |  when punched from bottom to top


### Projectile Type 

Projectile types are similar to object types (and are defined in the same space) but collision handling is different. 

 proj_property | position | comment 
--------- | --------- | -------
 color |0| id of the 4 frame-8x8 mini sprites tile
 movement |1| type of movement (see object movements)
 collision | | type of collision : projectiles collide only with objects. take object value
 spawn | 3 | type of sprite spawned when this object dies - or TRANSPARENT

## Sound

### Instruments properties

instr_prop | id | description
---------- | --- | -----------
square12   | 0 | square wave, 12% ratio
square25   | 1 | square wave, 25% ratio
square50   | 2 | square wave, 50% ratio
sawtooth   | 3 | sawtooth wave
triangle | 4 | triangle wave
noise | 5 | noise wave


### SFX definitions

sfx | id | description
--- | ---- | -----
kill | 0 | player killed
jump | 1 | player jumps
fire| 2 | player fires projectile
level | 3 | end of level
coin | 4 | +1 coin
key |5 | +1 key
life | 6 | +1 life
switch | 7 | switch activated
destroy1 | 8 | object destroyed 1
destroy2 | 9 | object destroyed 2
hit | 10 | player hits enemy 
unlock | 11 | when 3keys gets unlocked

## Black mapper 

### Black Mapper Terrain types 

Standard terrains + New terrain types defined : 
 - definitions of ALTs Id (allowing two neighbour blocks to be defined independently with the same terrain type) 
 - also decors (which will be tranformed as background tiles ), alt BG, ...

terrains must be > 64 !

blk_terrain | color_id | comment
----|-----|-------
empty | terrain_empty | see terrain
obstacle | terrain_obstacle | 
kill | terrain_kill | 
ladder | terrain_ladder |
alt  | 181 |  alt empty
decor | 159 | clouds, bushes, flowers...
decor2 | 95 | alt decor
obstacle2 | 72 | alt obstacle terrain. allows defining zones better

###  Black mapper tiles 

Black Mapper Tiles positions 

those define the tile_id defined with black mapper (as well as how they behave)

blk_tile | tile id | comment
----|-----|------
empty | 0 | sky
cloud | 1 | cloud 1x1
altcloud | 2 | alt cloud 1x1 
longcloud | 3 | cloud Nx1
decor_one | 9 | decor 1x1
decor |  7+2*16 | 
decor_h | 6 | 
decor_v | 9+16 | 
decor_under | 10 | 
ground | 2*16+4 | 
altground | 3*16+6 | 
pipe1 | 16 | vertical pipe width 1
pipe2 | 17 |  vertical  pipe width 2
pipe1h | 3*16 | horizontal pipe (acts as a platform)
obstacle_unique | 7+3*16 | 
water | 8+3*16 | 
kill_one | 11+2*16 | 
kill_over | 10+1*16 | 
kill_under | 10+2*16 | 
altbg | 11 | 
ladder |  11+16 | 




