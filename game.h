#pragma once
#include <stdint.h>

// Defines -
// -----------------------------------


#define IMAGE_WIDTH 256
#define TITLE_HEIGHT 128
#define LEVEL_HEIGHT 256

#define TILEMAP_START (4*16*256)

#define TRANSPARENT 230 
#define HITBOX_COLOR 25
#define MAX_SPRITES 32 // max onscreen, can be many more per level
#define NB_SPRITETYPES 16 // 2x16

// minimap is a special 16x16 tile on the lower left corner. 
/* each pixel the 4 first lines show each level properties.

first pixel is the type of map. 0 is side view, 255 (white) is top view.
blue (#7) square is first level

 */

/* the 4 tiles after represent objects properties after 
- first pixel is the color on the map
- 
 
  */
// the color representing the position of the minimap is the transparent color.
// the rest is a depiction of terrains. just color them with colors after 48 (ie after the 4th line in color palette)


// Types -
// -----------------------------------

/* interpret pixel colors as terrains - terrains are >64 */
enum TerrainColors {
	terrain_empty=87, // empty
	terrain_alt =181, // alt empty

	terrain_obstacle=104, 
	terrain_obstacle2=72, 

	terrain_decor=159, // clouds, bushes, flowers...
	terrain_decor2=95, 

	terrain_kill=240, // lava, spikes ...

	terrain_ladder = 147,
	terrain_ice = 151, 

	terrain_start=255, // only one, replaced by its above value
};

// special tiles 0-f -> 200->215 ? 

enum TileIDs {
	tile_empty=0,

	tile_cloud=1,
	tile_altcloud=2,
	tile_longcloud=3,
	
	tile_decor_one=9,
	tile_decor  =7+2*16,
	tile_decor_h=6,
	tile_decor_v=9+16,
	tile_decor_under=10,

	tile_ground=2*16+4,
	tile_altground=3*16+6,
	tile_pipe1=16,
	tile_pipe2=17, // vertical  pipe width 2
	tile_pipe1h=3*16, // horizontal pipe
	tile_obstacle_unique=7+3*16,

	tile_water=8+3*16,
	tile_kill_one=11+2*16,
	tile_kill_over=10+1*16,
	tile_kill_under=10+2*16,

	tile_altbg = 11,
	tile_ladder= 11+16,

 	// will be mapped as first tile after tiles.
};


struct SpriteType;
struct Sprite {
	int x,y; // pos 
	int vx,vy; // speed
	uint8_t hflip; // h flip. not always linked to vx

	uint8_t frame; // in animation
	uint8_t state; // zero init, spec depends on type 

	uint8_t type;  // TRANSPARENT meaning undefined sprite

	uint8_t tx,ty; // original position on level in tiles (absolute), 0,0 means none
};

enum spritetype_movement {
	// standard 
	mov_player = 255, // (white) implied for first object
	mov_nomove = TRANSPARENT , // static, by example gives a bonus once touched. 1 frame
	mov_alternate1 = 7,  // (bright blue) no move, just alternating 2 frames each 16 frames
	mov_alternate2 = 39, // (bright blue) no move, just alternating 2 frames each 32 frames
	mov_alternate3 = 71, // (bright blue) no move, just ping-ponging 3 frames (ABCBA ..) each 16 frames	
	mov_alternate4 = 103, // (purple) no move, just cycling 4 frames each 16 frames	

	mov_throbbing = 25,  // (bright green) static going up and down one pixel (to be better seen). 1 frame.
	mov_singleanim=107,  // (grey) 4 frames animation then destroy sprite

	mov_flybounce = 11, // flies but bounces on walls. state : current speed vector

	mov_walk = 3, // subject to gravity, walks and reverse direction if obstacle or holes. 2 frames
	mov_walkfall = 4, // subject to gravity, left and right if obstacle, falls if hole. 2 frames
	mov_leftrightjump = 5, // jumps from time to time. 2 frames 
	mov_sticky = 6,    // walks on borders of blocking sprites, will go around edges cw. 2frames
	mov_vsine4 = 7,    // vertical sine, 4 tiles height. 2frames.

	mov_bulletL = 8,  // flies, not stopped by blocks, no gravity, right to left. 1frame
	mov_bulletR = 9,  // flies, not stopped by blocks, no gravity, left to right. 1frame
	mov_bulletD = 10,  // flies, not stopped by blocks, no gravity, goes down. 2frames

	mov_bulletLv2 = 11,  // flies, a bit faster than preceding
	mov_bulletRv2 = 12,  // flies, a bit faster than preceding
	
	mov_generator = 224, // does not move, generates each ~2 seconds enemy with id just after this one. 2fr (just before)

	mov_ladder = 15,  // stays on ladders. right to left, go back to right if finds border. 1 frame alternating
};


// collision with player
enum sprite_collide {
	col_none = TRANSPARENT, // no collision
	col_kill = terrain_kill, // 240 - red, kills player instantly
	col_block = terrain_obstacle, // blocks the player - can throw it from edges...

	col_coin = 249, // yellow, gives a coin - or N=next , 50 of them gives a life
	col_life = 25,  // green, gives a life
	col_key  = 137, // gives a key
	

	col_spawn, // creates a new object (inplace) - type is next pixel
	col_span2, // spawns two identical elements
	
	// col_superX


};

struct SpriteType {
	// copy from data for speed/convenience
	uint8_t color; // TRANSPARENT if undefined
	uint8_t movement;
	uint8_t collision; // type of collision - sprite_collide
	uint8_t speed; // speed as byte-vector (zero is 119).
	
	uint8_t pad[5];

	// interpreted
	uint8_t x,y,w,h; 
	uint8_t hitx1,hity1,hitx2,hity2;
}; 



// bg tiles type for SIDE view
/*
--> plusieurs confs ensuite : rich BG, ...


 background :
   normal(1)
   alt(1) - terrain ?

 decors :
   cloud&co (decors non-touching ground, UP) : H:3, 1x1 A/B (alternate) : 5
   		inverser pour down ??

   bush (ie touching obstacle down):H(3)- for 2-6 H,V(2),1(1 or repeated >7), nxn (4) 10

 alt-bg : pas de gravité sur les alt bg !
   1xN vertical : ladder
   alt-bg : NxN

 obstacle :
   tubes V2xN(4) , V1xN(2), H1xN(3), ground MxM(3x3 + 1 alt center)
   unbreakable space
   ground (3: 1 pour contact avec air, )

 platforms :
   1x1 : 1
   horizontal : 3

 special blocks(1x1) : animated (all, 2 frames) -: 32 blocks
 - generating bullets 1/2 (2types)
 - interrupt (on+off)
 - switchable (by inter)
 - killing (2x2 frames each : au contact de l'air, 1 au contact terrain-up+dn)
 - auto jump up 2frames
 - door : -3keys open/closed
 - cassable si super + cassé
 - surprise / incassable (=surprise finie)
 - incassable (=? deja cassé=1x1 terrain)


 bonus : terrain diffrent mais même tile ? + tile alt/bg (pour secrets)
 - bonus super (off : given)
 - key+1 (+ taken)
 - life +1
 - +1 piece / off (suprise)

 - bloc start autowalk/glisse ? <-- non, level

mode autoscroll : peut casser des trucs qu'on peut pas sinon ! des blocs
   bump up / left / right
terrains : 12 types + 2 bg + 1 decor + 1 obstacles  : 16!

HUD :
 - lifes, super, 0-9, piece : 16 en 4x4 : 4 tiles

MODES :
 - platformer avec gravite (avec ou sans saut .. )
 - super=lance trucs, super=glisse & casse, etoile mario
 - autoscroll avec gravite run & gun
 - autoscroll sans gravite (shhmup)
 - lemmings-like (touche pas ennemies (ou si:) , enn. meurent tt seuls, sautent, casse qq briques & c tt, aller ds porte ... )
 - jumper simple / avec tir
 - alterego ! avec scroll ?

WORLD :
    chains N blocs de 16x16
	alterner : monde1-1, 2-1,3-1,4-1..N-1 puis 1-2 2-2 3-2 ...

*/

enum GameState {
	FS_Nothing, // nothing read, file closed
	FS_Header, // header only
	FS_Title, // title data
	FS_Level, // level raw data
	FS_Interpreted, // interpreted level data
	FS_Die, // dying animation
};


// variables -
// -----------------------------------

extern enum GameState game_state; // what is currently in data memory ?

extern uint8_t tile2terrain[256]; // fast lookup of tile_id to terrain type id.

extern uint8_t data[256*256];
extern uint8_t level_color; // color of pixels in minimap

extern int player_x, player_y; // position inside whole map (data seen as a 256x256 tilemap. 
extern int camera_x, camera_y; // vertical position of the title/scroll
extern struct SpriteType sprtype[NB_SPRITETYPES]; 
extern struct Sprite sprite[MAX_SPRITES];

extern int lives, coins, level, keys;

// functions -
// -----------------------------------

void load_bmp(const char *filename); // --> init + load_next (cycle tt seul)
void load_title(uint8_t *data);
void load_level(uint8_t *data);
int sine(uint8_t phi);

void interpret_terrains();
void interpret_spritetypes();

uint8_t get_terrain(const uint8_t tile_id);
void player_kill();


/*
	step1 : modify graphics

	open in grahical editor (ex : gimp, photoshop)
	set grid size to 16, work in indexed palette (set, uncheck remove .. ) - never change palette
	a tile will be 16x16 so 16x8 for the title (fixed) and 16x16 for the game

	structure of the grid :
	- main title 8x16 tiles
	- sprites
	- BG tiles
	- 4 levels
	- sounds


    step1 : modify graphics
	step2 : modify levels
	step3 : modify gameplay
	step5 : modify music
	step4 : modify sheet structure
	step5 : multilevels

*/