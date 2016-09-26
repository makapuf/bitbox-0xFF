#pragma once
#include <stdint.h>

#include "defs.h" // generated from REFERENCE.md 
// Defines
// -----------------------------------

#define IMAGE_WIDTH 256
#define TITLE_HEIGHT 128

// position of the title "level"
#define TILE_TITLE_X 2 
#define TILE_TITLE_Y 15

#define LEVEL_HEIGHT 256

#define TRANSPARENT 230 
#define HITBOX_COLOR 25
#define MAX_SPRITES 32 // max onscreen, can be many more per level
#define NB_SPRITETYPES 16 // 2x16 for whole game

#define START_LIVES 5

#define SPRITE_INACTIVE 200 // still loaded but not active, of type 200+old_type
#define SPRITE_FREE 255 // not loaded, place is free


// Types
// -----------------------------------

struct Sprite {
	int32_t  x,y; // pos 
	int     vx,vy; // speed
	uint8_t hflip; // h flip. not always linked to vx

	uint8_t frame; // in animation
	uint8_t state; // zero init, spec depends on type 

	uint8_t type;  // TRANSPARENT meaning undefined sprite, else type id 0-31

	uint8_t tx,ty; // original position on level in tiles (absolute), 0,0 means none
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


// Variables
// -----------------------------------

extern void (*frame_handler)( void ); // pointer to frame handler.

extern uint8_t tile2terrain[256]; // fast lookup of tile_id to terrain type id.

extern uint8_t data[256*256]; 
extern uint8_t level_color; // color of pixels in minimap

extern int player_x, player_y; // position inside whole map (data seen as a 256x256 tilemap. 
extern int camera_x, camera_y; // vertical position of the title/scroll
extern uint8_t level_x1,level_y1,level_x2,level_y2;

extern struct SpriteType sprtype[NB_SPRITETYPES]; 
extern struct Sprite sprite[MAX_SPRITES];

extern int lives, coins, level, keys;
extern char hud[20]; // 0123456789:lkgtWLEVS (l=life,k=key,g=gold, t=time) -> 0123456789ABCDEFGHIJ and ' ' is space

// Functions
// -----------------------------------

void frame_die  (void);
void frame_play (void);
void frame_leveltitle(void);
void frame_title(void);
void frame_logo (void);
void frame_error(void);

void enter_title(void);
void enter_logo (void);
void enter_level(void);

// loader
int loader_init (); // init loader 
int load_bmp (const char *filename); // --> init + load_next (cycle)
int load_game_data (uint8_t *data);
int sine (uint8_t phi);

// sprites
void interpret_spritetypes();
void sprites_reset();
void sprite_kill(struct Sprite *spr);
void sprite_move(struct Sprite *spr);
void all_sprite_move() ;
uint8_t collision_tile(const struct Sprite *spr);
struct Sprite *spawn_sprite(uint8_t type, int x, int y);


// player
void move_camera(void);
void move_player(struct Sprite *spr);
void player_reset(void);
void player_kill();

// sounds
void play_sfx( int sfx_id ); 
void play_song();
void stop_song();

// player
void manage_sprites();
uint8_t terrain_at(int x, int y);

void black_mapper(void);


// Pure inlines 
// ----------------------------------


// read pixel at x,y in tile number X
inline uint8_t read_tile(const uint8_t tile_id, const int x, const int y)
{
	return data[(tile_id/16*16+y)*256+(tile_id%16)*16 + x];
}

// get property id (0-7) from object id (to sprite_type)
// objects 0-4 are levels ! 
inline uint8_t get_property(const int object_id, const int property) 
{
	return read_tile(0xF1,property+(object_id>=16)*8,object_id%16);
}

// tile id to terrain (ie. read minimap)
inline uint8_t get_terrain (const uint8_t tile_id)
{
	return read_tile(240,tile_id%16,tile_id/16); 
}


inline int is_walkable(uint8_t terrain) {
	return terrain == terrain_obstacle || 
		   terrain == terrain_ice || 
		   terrain == terrain_ladder || 
		   terrain == terrain_platform;
}