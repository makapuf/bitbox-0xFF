#pragma once
#include <stdint.h>

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


// Types
// -----------------------------------

struct Sprite {
	int32_t  x,y; // pos 
	int vx,vy; // speed
	uint8_t hflip; // h flip. not always linked to vx

	uint8_t frame; // in animation
	uint8_t state; // zero init, spec depends on type 

	uint8_t type;  // TRANSPARENT meaning undefined sprite

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
extern struct SpriteType sprtype[NB_SPRITETYPES]; 
extern struct Sprite sprite[MAX_SPRITES];

extern int lives, coins, level, keys;

// Functions
// -----------------------------------

void frame_die  (void);
void frame_play (void);
void frame_title(void);
void frame_logo (void);
void frame_error(void);

void enter_title(void);
void enter_logo (void);

int load_bmp(const char *filename); // --> init + load_next (cycle tt seul)
int load_title(uint8_t *data);
int load_level(uint8_t *data);
int sine(uint8_t phi);

void interpret_spritetypes();

void player_kill();
uint8_t collision_tile(const struct Sprite *spr);
void sprite_move(struct Sprite *spr);
void manage_sprites();
uint8_t terrain_at(int x, int y);

void black_mapper(void);


// Inlines 
// ----------------------------------

// get property id (0-7) from object id (to sprite_type)
// objects 0-4 are levels ! 
inline uint8_t get_property(const int object_id, const int property) 
{
	return data[(15*16+object_id%16)*256+property+16 + (object_id>=16)*8];
}

inline uint8_t get_terrain (const uint8_t tile_id)
{
	return data[(15*16+(tile_id/16))*256+tile_id%16]; // minimap values
}
