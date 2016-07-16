#pragma once
#include <stdint.h>

// Defines -
// -----------------------------------


#define IMAGE_WIDTH 256
#define TITLE_HEIGHT 128
#define LEVEL_HEIGHT 256

#define TRANSPARENT 230 
#define HITBOX_COLOR 25
#define MAX_SPRITES 32 // max onscreen, can be many more per level
#define NB_SPRITETYPES 16 // 2x16 for whole game

#define START_LIVES 5

// Types
// -----------------------------------

/* interpret pixel colors as terrains - terrains are >64 */
// TODO : make terrains testable by bits ?? by %8 ? 
enum TerrainColors {
	terrain_empty=87, // empty
	terrain_animated_empty=86, // 4 frames animated but behaves like empty. TileID will be +1 % 4 each 32 frames
	terrain_obstacle=104, 
	terrain_kill=240, // lava, spikes ...
	terrain_ladder = 147,
	terrain_ice = 151, 
	terrain_platform = 136, // cannot fall but can go through up or sideways

	terrain_start=255, // only one, replaced by its above value
	
	// used with mappers only
	terrain_alt =181, // alt empty
	terrain_decor=159, // clouds, bushes, flowers...
	terrain_decor2=95, 
	terrain_obstacle2=72, 
};

// black mapper tile_id
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
	tile_pipe1h=3*16, // horizontal pipe TODO  : make it a platform
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
	int32_t  x,y; // pos 
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
	mov_nomove = TRANSPARENT , // static, no animation. 1 frame.
	mov_alternate1 = 7,  // (bright blue) no move, just alternating 2 frames each 16 frames
	mov_alternate2 = 39, // (bright blue) no move, just alternating 2 frames each 32 frames
	mov_alternate3 = 71, // (bright blue) no move, just ping-ponging 3 frames (ABCBA ..) each 16 frames	
	mov_alternate4 = 103, // (purple) no move, just cycling 4 frames each 16 frames	

	mov_throbbing = 25,  // (bright green) static going up and down one pixel (to be better seen). 1 frame.
	mov_singleanim4=107,  // (grey) 4 frames animation then destroy sprite
	mov_singleanim2=75,  // (blueish grey) 2 frames animation then destroy sprite

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
	col_block = terrain_obstacle, // blocks the player - can push him from edges...

	col_coin = 249, // yellow, gives a coin - or N=next , 50 of them gives a life
	col_life = 25,  // green, gives a life and disappear with explosion animation
	col_key  = 137, // gives a key


	// col_bulletX // activates bullet X


};

struct SpriteType {
	// copy from data for speed/convenience
	uint8_t color; // TRANSPARENT if undefined
	uint8_t movement;
	uint8_t collision; // type of collision - sprite_collide
	uint8_t spawn; // type of sprite spawned
	//uint8_t speed; // speed as byte-vector (zero is 119).

	uint8_t pad[4];

	// interpreted
	uint8_t x,y,w,h; 
	uint8_t hitx1,hity1,hitx2,hity2;
}; 


// variables
// -----------------------------------

extern void (*frame_handler)( void ); // pointer to frame handler.

extern uint8_t tile2terrain[256]; // fast lookup of tile_id to terrain type id.

extern uint8_t data[256*256]; 
extern uint8_t level_color; // color of pixels in minimap

extern int player_x, player_y; // position inside whole map (data seen as a 256x256 tilemap. 
extern int camera_x, camera_y; // vertical position of the title/scroll
extern struct SpriteType sprtype[NB_SPRITETYPES]; 
extern struct Sprite sprite[MAX_SPRITES];

extern int lives, coins, level, keys;

// functions
// -----------------------------------

int load_bmp(const char *filename); // --> init + load_next (cycle tt seul)
int load_title(uint8_t *data);
int load_level(uint8_t *data);
int sine(uint8_t phi);

void interpret_spritetypes();

uint8_t get_terrain(const uint8_t tile_id);
void player_kill();
uint8_t collision_tile(const struct Sprite *spr);
void sprite_move(struct Sprite *spr);
void manage_sprites();
uint8_t terrain_at(int x, int y);

void black_mapper(void);

void enter_title(void);

// inlines 
// ----------------------------------

// get property id (0-7) from object id (to sprite_type)
// objects 0-4 are levels ! 
inline uint8_t get_property(const int object_id, const int property) 
{
	return data[(15*16+object_id%16)*256+property+16 + (object_id>=16)*8];
}
