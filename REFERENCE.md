
# Reference

Elements with no id are not implemented yet

### levels 

position | property | comment
---------|----------|---------
0      | level color | color of the terrain on the minimap
1-7    | reserved 
      | control | see control types
      | jumps   | see jump types
      | physics | (gravity, maxspeed ) as a 2D vector pixel

2D vector pixels are defined by a center +  a position in X and Y on the palette. 

### control types

name | id | description | buttons 
-----|----|----------- | -----------
LR   |    | standard   | can (optional) run on pressing B, optionally jump on pressing A, X fire
LR hit |  | can hit enemies 
LR runstomp | | can run, can stomp enemies forward by running (like hitting them)
runner | | always goes right at average speed, can jump, average acceleration
LR+aim | | can go left+right, aims by 45 degrees increments 
beatemup | | can jump on ladders, falls back where we are


### jump types

name | id | description
-----|----|-----------
none | | cant jump
simple |  | simple jumping
simple+stomp | | can stomp enemies vetically by pressing down while jumping (hit them)
double |  | double jump (stomps)
triple |  | triple jump (stomps)
double+wall | | double & wall jump
double+wall+stomp | | can stomp objects 


### object types

position | property | comment 
--------- | --------- | -------
0 | color | color to prepresent object on tilemap. TRANSPARENT if this object type is undefined
1 | movement | type of movement (see object movements)
2 | collision | type of collision - sprite_collide (see object collisions table)
3 | spawn | type of sprite spawned when this object dies
4-7 | reserved | leave transparent.
  | collision_up | collision when player hits from bottom
  | collision_dn | collision when player hits from bottom
  | collision_side | collision when player hits from bottom
  | hit | collision type if player hits/punches/stomp it (split into up/down/side?)


### terrains

name | terrain_id | description
------| -------| -----
terrain_empty | 87 | empty, does not interact with player
terrain_animated_empty | 86 | 4 frames animated but behaves like empty. TileID will be +1 % 4 each 32 frames
terrain_obstacle | 104 | blocks user fro left or right  
terrain_kill | 240 | kills when touch it 
terrain_ladder | 147 | can go up, down even with gravity
terrain_ice | 151 | cannot stop on X, but can jump 
terrain_platform | 136 | cannot fall but can go through up or sideways
terrain_start |   | start from here or restart / save point. Always go from left to right.
terrain_jump |    | make the player automatically jump 

### object movements

title | color id | comment 
----------- | --------- | ----------------------
mov_nomove | TRANSPARENT | static, by example gives a bonus once touched. 1 frame
mov_alternate1 | 7 | (bright blue) no move, just alternating 2 frames each 16 frames
mov_alternate2 | 39 | (bright blue) no move, just alternating 2 frames each 32 frames
mov_alternate3 | 71 | (bright blue) no move, just ping-ponging 3 frames (ABCBA ..) each 16 frames	
mov_alternate4 | 103 | (purple) no move, just cycling 4 frames each 16 frames	
mov_throbbing | 25 | (bright green) static going up and down one pixel (to be better seen). 1 frame.
mov_singleanim4 | 7 | (grey) 4 frames animation then destroy sprite
mov_singleanim2 | 75 | (blueish grey) 2 frames animation then destroy sprite
mov_flybounce | 11 | flies but bounces on walls. state : current speed vector
mov_walk | 3 | subject to gravity, walks and reverse direction if obstacle or holes. 2 frames
mov_walkfall | 4 | subject to gravity, left and right if obstacle, falls if hole. 2 frames
mov_leftrightjump | 5 | jumps from time to time. 2 frames 
mov_sticky | 6 | walks on borders of blocking sprites, will go around edges cw. 2frames
mov_vsine4 | 7 | vertical sine, 4 tiles height. 2frames alternating every 16frames
mov_bulletL | 8 | flies, not stopped by blocks, no gravity, right to left. 1frame
mov_bulletR | 9 | flies, not stopped by blocks, no gravity, left to right. 1frame
mov_bulletD | 10 | flies, not stopped by blocks, no gravity, goes down. 2frames
mov_bulletLv2 | 11 | flies, a bit faster than preceding
mov_bulletRv2 | 12 | flies, a bit faster than preceding
mov_generator | 224 | does not move, generates each ~2 seconds enemy with id just after this one. 2fr 
mov_ladder | 15 | stays on ladders. right to left, go back to right if finds border. 1 frame alternating
mov_player | 255 | (white) implied for first object (??)

### object collisions


collision type | color id | comment
------ | ------ | ------
col_none | TRANSPARENT | no collision
col_kill | terrain_kill == 240 | red, kills player instantly
col_block | terrain_obstacle | blocks the player - can push it
col_coin | 249 | yellow, gives a coin - or Nb =next ? , 50 of them gives a life
col_life | 25 | green, gives a life and disappear with explosion animation
col_keyA |  137 | gives a key of type A
col_keyB |   | gives a key of type B
col_end | | ends level
col_block_1keyA | | ends level if you've 1 key  of type A
col_block_3keyA | | ends level if you've 3 keys of type A
col_block_3keyB | | ends level if you've 3 keys of type B
col_switch | | transform next object in list into its next object
col_stomped | | blocks if run into but destroyed if hit / stomped
col_killjump | | kills player if hit from bottom or side, but stomps if jump on it (or 
col_killhit | | kills the player when touched from any angle , can be killed if stomped or hit

### Black mapper terrains / tiles

#### Terrain types 

New terrain types defined : standard terrains, plus definitions of ALTs Id (allowing two neighbour blocks to be defined independently with the same terrain type) ; also decors.

terrain_name | color_id | comment
----|-----|-------
terrain_alt  | 181 |  alt empty
terrain_decor | 159 | clouds, bushes, flowers...
terrain_decor2 | 95 | alt decor
terrain_obstacle2 | 72 | alt obstacle terrain. allows defining zones better

#### Tiles definition

those define the tile_id defined with black mapper (as well as how they behave)

tile name | tile id | comment
----|-----|------
tile_empty | 0 | sky
----|-----|------
tile_cloud | 1 | cloud 1x1
tile_altcloud | 2 | alt cloud 1x1 
tile_longcloud | 3 | cloud Nx1
----|-----|------
tile_decor_one | 9 | decor 1x1
tile_decor |  =7+2*16 | 
tile_decor_h | 6 | 
tile_decor_v | 9+16 | 
tile_decor_under | 10 | 
----|-----|------
tile_ground | 2*16+4 | 
tile_altground | 3*16+6 | 
----|-----|------
tile_pipe1 | 16 | 
tile_pipe2 | 17 |  // vertical  pipe width 2
tile_pipe1h | 3*16 |  // horizontal pipe 
tile_obstacle_unique | 7+3*16 | 
----|-----|------
tile_water | 8+3*16 | 
tile_kill_one | 11+2*16 | 
tile_kill_over | 10+1*16 | 
tile_kill_under | 10+2*16 | 
tile_altbg | = 11 | 
tile_ladder |  11+16 | 




