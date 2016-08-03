# Reference

Elements with no id are not implemented yet.

### levels 

description of the level : each id is an index on the level description bytes

level_pos | property(0-7) | comment
---------|----------|---------
color | 0 | color of the terrain on the minimap
control |  | see control types
jumps |  | see jump types
physics | | (gravity, maxspeed ) as a 2D vector pixel

2D vector pixels are defined by a center +  a position in X and Y on the palette. 

### control types

Player control type

control | id | description / controls
-----|----|----------------------
LR   |  0  | standard  controls : can (optional) run on pressing B, optionally jump on pressing A, X fire
LR hit |  | can hit enemies 
LR runstomp | | can run, can stomp enemies forward by running (like hitting them)
runner | | always goes right at average speed, can jump, average acceleration
LR+aim | | can go left+right, aims by 45 degrees increments 
beatemup | | can jump on ladders, falls back where we are


### jump types

Player jump type.

jump | id | description
-----|----|-----------
none | 0 | cant jump
simple | 1 | simple jumping
simple+stomp | | can stomp enemies vetically by pressing down while jumping (hit them)
double |  | double jump (stomps)
triple |  | triple jump (stomps)
double+wall | | double & wall jump
double+wall+stomp | | can stomp objects 


### Object Types

Indices in object type definition bytes.

 property | position | comment 
--------- | --------- | -------
 color |0| color to prepresent object on tilemap. 230 (transparent) if this object type is undefined
 movement |1| type of movement (see object movements)
 collision |2| type of collision - sprite_collide (see object collisions table)
 spawn | 3 | type of sprite spawned when this object dies
 reserved | 4-7 | leave transparent.
 collision_up | | collision when player hits from bottom
 collision_dn | | collision when player hits from bottom
 collision_side | | collision when player hits from bottom
 hit | | collision type if player hits/punches/stomp it (split into up/down/side?)


### Terrains

Terrain types. ie how a terrain tile behaves.

terrain | terrain_id | description
------| -------| -----
empty | 87 | empty, does not interact with player
animated_empty | 86 | 4 frames animated but behaves like empty. TileID will be +1 % 4 each 32 frames
obstacle | 104 | blocks user fro left or right  
kill | 240 | kills when touch it 
ladder | 147 | can go up, down even with gravity
ice | 151 | cannot stop on X, but can jump 
platform | 136 | cannot fall but can go through up or sideways
start |   | start from here or restart / save point. Always go from left to right.
jump |    | make the player automatically jump 

> TODO : make terrains testable by bits ?? by %8 ? 

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
flybounce | 11 | flies but bounces on walls. state : current speed vector
walk | 3 | subject to gravity, walks and reverse direction if obstacle or holes. 2 frames
walkfall |  | subject to gravity, left and right if obstacle, falls if hole. 2 frames
leftrightjump |  | jumps from time to time. 2 frames 
sticky |  | walks on borders of blocking sprites, will go around edges cw. 2frames
vsine4 |  | vertical sine, 4 tiles height. 2frames alternating every 16frames
bulletL | 8 | flies, not stopped by blocks, no gravity, right to left. 1frame
bulletR | 9 | flies, not stopped by blocks, no gravity, left to right. 1frame
bulletD |  | flies, not stopped by blocks, no gravity, goes down. 2frames
bulletLv2 | 11 | flies, a bit faster than preceding
bulletRv2 |  | flies, a bit faster than preceding
generator |  | does not move, generates each ~2 seconds enemy with id just after this one. 2fr 
ladder |  | stays on ladders. right to left, go back to right if finds border. 1 frame alternating
player | 255 | (white) implied for first object (??)

### Collisions

Object collision types : what happens when an object of this type collides with the player.

col | color id | comment
------ | ------ | ------
none | 230 | no collision
kill | 240 | red (==terrain_kill) , kills player instantly
block | terrain_obstacle | blocks the player - can push it
coin | 249 | yellow, gives a coin - or Nb =next ? , 50 of them gives a life
life | 25 | green, gives a life and disappear with explosion animation
keyA |  137 | gives a key of type A
keyB |   | gives a key of type B
end | | ends level
block_1keyA | | ends level if you've 1 key  of type A
block_3keyA | | ends level if you've 3 keys of type A
block_3keyB | | ends level if you've 3 keys of type B
switch | | transform next object in list into its next object
stomped | | blocks if run into but destroyed if hit / stomped
killjump | | kills player if hit from bottom or side, but stomps if jump on it (or 
killhit | | kills the player when touched from any angle , can be killed if stomped or hit
restart | | the object dies once touched. the player will restart here once touched.

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
pipe1 | 16 | 
pipe2 | 17 |  vertical  pipe width 2
pipe1h | 3*16 | horizontal pipe (a platform)
obstacle_unique | 7+3*16 | 
water | 8+3*16 | 
kill_one | 11+2*16 | 
kill_over | 10+1*16 | 
kill_under | 10+2*16 | 
altbg | 11 | 
ladder |  11+16 | 




