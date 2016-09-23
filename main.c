#include <stdint.h>
#include <stdbool.h>

#include <string.h>

#include "bitbox.h"

#include "game.h"
#include "defs.h"

int lives;
int coins;
int level; // 0-3
int njumps; // current value for double/triple jumps

uint8_t level_color; // color of pixels in minimap for current level
uint8_t level_x1,level_y1,level_x2,level_y2;
uint16_t start_x, start_y; // start position on world

uint8_t data[256*256];


int camera_x, camera_y; // vertical position of the title/scroll

// sprites on screen

struct Sprite sprite[MAX_SPRITES];
struct SpriteType sprtype[NB_SPRITETYPES]; 

void (*frame_handler)( void ); 

void player_reset()
{
	// TODO real start of level ! 
	sprite[0].x=0;
	sprite[0].y=150+4*256;
	sprite[0].frame=0;
	sprite[0].type =0;
}

static const uint8_t sines[64] = {
	  0,   6,  12,  18,  25,  31,  37,  43,  49,  56,  62,  68,  74,  80,  86,  92,
	 97, 103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176, 
	181, 185, 189, 193, 197, 201, 205, 209, 212, 216, 219, 222, 225, 228, 231, 234,
	236, 238, 241, 243, 244, 246, 248, 249, 251, 252, 253, 254, 254, 255, 255, 255 
};

int sine(uint8_t phi)
{
	// phi [0,255], out : [-255,255]
	int ofs;
	switch(phi/64) {
		case 0 : ofs=sines[phi]; break;
		case 1 : ofs=sines[127-phi]; break;
		case 2 : ofs=-sines[phi-128]; break;
		case 3 : ofs=-sines[255-phi]; break;
		default:
			ofs=0;break;
	}
	return ofs;
}

// get the terrain type located at pixel in level position x,y
inline uint8_t terrain_at(int x, int y)
{
	//message("%d,%d %d==%d\n",x/256,x,get_terrain(y/256*16+x/256),level_color);
	if (y<0 || x<0 || x>256*16 || y>256*16 || get_terrain(y/256*16+x/256)!=level_color) {
		return terrain_obstacle; // off limits or is the tested tile not defined as level ? then blocks
	}
	const uint8_t tile_id = data[y/16*256+x/16]; // read tile_id from level
	return get_terrain(tile_id);
}


inline int can_move_hori(uint8_t terrain) {
	return !(terrain == terrain_ice || terrain == terrain_obstacle);
}

inline int can_move_up(uint8_t terrain) {
	return !(terrain == terrain_ladder || terrain == terrain_ice || terrain == terrain_obstacle);
}

inline int can_move_down(uint8_t terrain) {
	return !(terrain == terrain_ladder || terrain == terrain_ice || terrain == terrain_obstacle);
}

inline int is_walkable(uint8_t terrain) {
	return terrain == terrain_obstacle || 
		   terrain == terrain_ice || 
		   terrain == terrain_ladder || 
		   terrain == terrain_platform;
}




static inline int sprite_left(const struct Sprite *spr)
{
	struct SpriteType *spt = &sprtype[spr->type];
	return spr->x+spt->hitx1;
}

static inline int sprite_right(const struct Sprite *spr)
{
	struct SpriteType *spt = &sprtype[spr->type];
	return spr->x+spt->hitx2;
}

static inline int sprite_top(const struct Sprite *spr)
{
	struct SpriteType *spt = &sprtype[spr->type];
	return spr->y+spt->hity1;
}

static inline int sprite_bottom(const struct Sprite *spr)
{
	struct SpriteType *spt = &sprtype[spr->type];
	return spr->y+spt->hity2;
}



/* checks which terrain this tile collides with returns one terrain for 
the sprite even if several collide.
Returns : terrain type
 */
uint8_t collision_tile(const struct Sprite *spr)
{
	// TODO in the level only !
	// TODO check for larger sprites (while +16 until hitx2 ... )
	// checks collision between tile and terrain. returns most interesting tile type
	if (spr->y>=4096) return terrain_empty;
	uint8_t t = terrain_at(sprite_left(spr),sprite_top(spr));

	if (t==terrain_empty) t = terrain_at(sprite_right(spr),sprite_top(spr));
	if (t==terrain_empty) t = terrain_at(sprite_left(spr),sprite_bottom(spr));
	if (t==terrain_empty) t = terrain_at(sprite_right(spr),sprite_bottom(spr));
	return t;
}


void move_player(struct Sprite *spr)
{
	kbd_emulate_gamepad();

	static uint16_t gamepad_oldstate=0;
	const uint16_t gamepad_pressed = gamepad_buttons[0] & ~gamepad_oldstate;

	struct SpriteType *spt = &sprtype[spr->type];

	// FIXME : make terrains bit-testable ? 
	int on_ground = \
	    terrain_at(spr->x+spt->hitx1,spr->y+spt->hity2+1) == terrain_obstacle || 
		terrain_at(spr->x+spt->hitx2,spr->y+spt->hity2+1) == terrain_obstacle ||
	    terrain_at(spr->x+spt->hitx1,spr->y+spt->hity2+1) == terrain_platform || 
		terrain_at(spr->x+spt->hitx2,spr->y+spt->hity2+1) == terrain_platform;

	// -- movement 
		
	// Left / Right
	if (vga_frame%4==0) { 
		if (GAMEPAD_PRESSED(0,right)) {
			if (spr->vx < 3) 
				spr->vx++;
		} else if (GAMEPAD_PRESSED(0,left)) {
			if (spr->vx >-3) 
				spr->vx--;
		} else {
			// decelerate 
			if (spr->vx>0) 
				spr->vx--;
			else if (spr->vx<0) 
				spr->vx++;
		}
	}

	// Jump - TODO double / triple / wall jump 
	if (gamepad_pressed & gamepad_A ) {
		if (on_ground) {
			play_sfx(0); // play ! // TODO add in ref
			spr->vy = -6;
		}
	}

	// Gravity
	if (vga_frame%4==0) {	
		spr->vy +=1; 
		if (spr->vy>4) 
			spr->vy=4;
	}

	// can move vertically / horizontally ? if not, revert (independently)

	// TODO : bigger sprites than 2x2
	// TODO : not out of level

	// test moving horizontally 	
	if (terrain_at( // test top collision
			spr->x + spr->vx + (spr->vx>0 ? spt->hitx2:spt->hitx1),
			spr->y + spt->hity1
		) != terrain_obstacle  &&
		terrain_at( // test bottom collision
			spr->x + spr->vx + (spr->vx>0 ? spt->hitx2:spt->hitx1),
			spr->y+spt->hity2
			) != terrain_obstacle
		) {
		spr->x += spr->vx;
	} else {
		spr->vx /= 2;
	}

	if (spr->vy >0 ) {
		// would touch down ? obstacle or platform (also uses hity2) 
		// TODO player can hang within platform
		uint8_t t_left =terrain_at( spr->x+spt->hitx1, spr->y+spr->vy + spt->hity2 );
		uint8_t t_right=terrain_at( spr->x+spt->hitx2, spr->y+spr->vy + spt->hity2 );

		if (  t_left!=terrain_obstacle && 
			  t_right != terrain_obstacle && 

			  t_left!=terrain_platform && 
			  t_right != terrain_platform 
			  ) 
		{
			spr->y += spr->vy;
		} else {
			spr->vy /= 2;
		}
	} else {
		// would touch up ? - obstacle only , uses hity1
		uint8_t t_left =terrain_at( spr->x+spt->hitx1, spr->y+spr->vy + spt->hity1 );
		uint8_t t_right=terrain_at( spr->x+spt->hitx2, spr->y+spr->vy + spt->hity1 );

		if (t_left!=terrain_obstacle && t_right != terrain_obstacle) {
			spr->y += spr->vy;
		} else {
			spr->vy /= 2;
		}
	}


	// -- animation
	if (on_ground) {		
		// animate LR walking frame even if cannot move
		if ((gamepad_buttons[0] & (gamepad_left|gamepad_right))) {
			if (vga_frame%4==0) {
				if (spr->frame>=2) 
					spr->frame=0;
				else 
					spr->frame++;
			}
		} else {
			spr->frame=0;
		}
	} else {
		if (spr->vy<0)
			spr->frame = 2; // jump
		else 
			spr->frame = 3; // fall
	}
	
	if (spr->vx>0) 	
		spr->hflip=0;
	else if (spr->vx<0) 
		spr->hflip=1;

	uint8_t terrain = collision_tile(spr);

	if (terrain==terrain_kill) 
		player_kill();

	// -- display game info 
	if (vga_frame%32==0) {	
		message("player (%d,%d) cam (%d,%d) tile %x",spr->x,spr->y,camera_x%16,camera_y, terrain);
		switch (terrain)
		{
			case terrain_empty  : message(" empty"); break;
			case terrain_obstacle : message(" obstacle"); break;
			case terrain_kill   : message(" kill"); break;
			case terrain_ladder : message(" ladder"); break;
			case terrain_ice : message(" ice"); break;
			default : message(" other:%d",terrain); break;
		}
		message ("\n");
	}

	gamepad_oldstate = gamepad_buttons[0];
}


void move_camera(void)
{
	const struct Sprite *spr = &sprite[0];


	if (spr->x > camera_x + 200 ) camera_x = spr->x-200;
	if (spr->x < camera_x + 100 ) camera_x = spr->x>100 ? spr->x-100 : 0;

	if (spr->y > camera_y + 150 ) camera_y = spr->y-150;
	if (spr->y < camera_y + 50  ) camera_y = spr->y-50;

	// check level limits

	if (camera_x<level_x1*256) camera_x=level_x1*256;
	if (camera_y<level_y1*256) camera_y=level_y1*256;

	if (camera_x>level_x2*256-VGA_H_PIXELS+256) camera_x=level_x2*256-VGA_H_PIXELS+256;
	if (camera_y>level_y2*256-VGA_V_PIXELS+256) camera_y=level_y2*256-VGA_V_PIXELS+256;
}



// --Sprites 

void sprites_reset()
{
	for (int i=0;i<MAX_SPRITES;i++) {
		sprite[i].type=SPRITE_FREE; 
	}
}

void interpret_spritetypes()
{	
	for (int id=0;id<NB_SPRITETYPES;id++) { // TODO 16 x 2columns
		struct SpriteType *spt = &sprtype[id];
		spt->color     = get_property(4+id,0);
		
		spt->movement  = get_property(4+id,1);
		spt->collision = get_property(4+id,2);
		spt->spawn     = get_property(4+id,3); 

		if (spt->color == TRANSPARENT) {			
			message ("    spritetype %d undefined\n",id);
			continue;
		}

		// compute first frame position
		spt->x = (spt->color%16)*16;
		spt->y = (spt->color/16)*16;

		// compute size / hitbox if not already known before - else copy
		// scan first tile horizontally. first hitbox pixel must be in tile
		int found=0;
		for (spt->hitx1=0;spt->hitx1<16;spt->hitx1++) {
			if (data[spt->y*256+spt->x+spt->hitx1]==HITBOX_COLOR) {
				data[spt->y*256+spt->x+spt->hitx1] = TRANSPARENT; // TODO copy symmetric Y pixel
				found=1;
				break; 
			}
		}

		for (spt->hity1=1;spt->hity1<16;spt->hity1++) {
			if (data[(spt->y+spt->hity1)*256+spt->x]==HITBOX_COLOR) {
				data[(spt->y+spt->hity1)*256+spt->x] = TRANSPARENT;
				break; 
			}	
		}
		// if found, hitbox y1 is necessarily >0
		if (found) {
			// now find x2
			for (spt->hitx2=spt->hitx1+1;spt->hitx2<=255;spt->hitx2++) {
				if (data[spt->y*256+spt->x+spt->hitx2] == HITBOX_COLOR) {
					data[spt->y*256+spt->x+spt->hitx2] = TRANSPARENT;
					break;
				}
			}
			// now find y2
			for (spt->hity2=spt->hity1+1;spt->hity2<=255;spt->hity2++) {
				if (data[(spt->y+spt->hity2)*256+spt->x] == HITBOX_COLOR) {
					data[(spt->y+spt->hity2)*256+spt->x] = TRANSPARENT;
					break;
				}
			}
			
			// infer w and h of sprite (as integral number of tiles)
			spt->w = 16*((15+spt->hitx2)/16);
			spt->h = 16*((15+spt->hity2)/16);

		} else {			
			// force to 16x16 full
			spt->hitx1=0;
			spt->hity1=0;
			spt->hitx2=15;
			spt->hity2=15;
			spt->w = 16;
			spt->h = 16;
		}

		message("sprtype %d - color %d move %d collision %d spawns %d ",id,spt->color, spt->movement, spt->collision, spt->spawn);
		message("x:%d y:%d w:%d h:%d ", spt->x, spt->y, spt->w, spt->h);
		message("hitbox : (%d,%d)-(%d,%d)\n", spt->hitx1,spt->hity1,spt->hitx2,spt->hity2);
		/*
		for (int y=0;y<spt->h;y++) {
			for (int x=0;x<spt->w;x++)
				message("%d ",data[(spt->y+y)*256+spt->x+x]);
			message("\n");
		}
		*/
	}
}

// http://kishimotostudios.com/articles/aabb_collision/
static inline int sprite_collide(const struct Sprite *a,const struct Sprite *b) {
	if (sprite_left(a) > sprite_right(b)) return 0; // A isToTheRightOf B
	if (sprite_right(a) < sprite_left(b)) return 0; // A isToTheLeftOf B
 	if (sprite_bottom(a) < sprite_top(b)) return 0; // A is above B
 	if (sprite_top(a) > sprite_bottom(b)) return 0; // AisBelowB
  	return 1;
}


inline struct Sprite *spawn_sprite(uint8_t type, int x, int y)
{
	int pos=0;

	// find a proper empty place to pos
	for (pos=0;pos<MAX_SPRITES && sprite[pos].type!=SPRITE_FREE;pos++);

	if (pos==MAX_SPRITES) {
		message("cannot insert new sprite : not enough, will overdraw (now %d)",pos);
		pos=MAX_SPRITES-1;
	}


	sprite[pos].type = type;
	sprite[pos].frame = 0;
	sprite[pos].x  = x;  sprite[pos].y  = y;
	sprite[pos].vx = 0;  sprite[pos].vy = 0;
	sprite[pos].tx = 255; sprite[pos].ty=255;

	sprite[pos].hflip=0; 

	message ("starting sprite %d type %d @ %d,%d onscreen\n",pos,type,sprite[pos].x-camera_x,sprite[pos].y-camera_y);
	return &sprite[pos];
}


inline void sprite_kill(struct Sprite *spr)
{
	// needs to spawn something ?	
	const uint8_t spawn = sprtype[spr->type].spawn;
	if (spawn != TRANSPARENT)
		spawn_sprite(spawn,spr->x,spr->y);			
	spr->type+=SPRITE_INACTIVE; // remove : set inactive (type) but not unloaded
}


void sprite_move(struct Sprite *spr)
{
	struct SpriteType *spt = &sprtype[spr->type];

	switch(spt->movement) {

		// simple animations --------------------
		
		// fast alternate, no move
		case mov_alternate1 : 
			spr->frame = (vga_frame/16)%2;
			break;

		// slow alternate, no move
		case mov_alternate2 : 
			spr->frame = (vga_frame/32)%2;
			break;

		// fast pingpong 3 frames, no move
		case mov_alternate3 : 
			spr->frame = (uint8_t[]){0,1,2,1}[(vga_frame/8)%4];
			break;

		// cycling 4 frames, no move
		case mov_alternate4 : 
			spr->frame = (vga_frame/8)%4;
			break;

		// 4 frames and disappear (fast)
		case mov_singleanim4 : 
			if (vga_frame%8==0) {
				spr->frame++;
				if (spr->frame == 4) {
					sprite_kill(spr);
				}
			}
			break;

		// 2 frames and disappear (fast)
		case mov_singleanim2 : 
			if (vga_frame%8==0) {
				spr->frame++;
				if (spr->frame == 2) {
					sprite_kill(spr);
				}
			}
			break;

		// 1 frame, going up and down
		case mov_throbbing : 
			if ((vga_frame%32)==0)
				spr->y +=1;
			else if (vga_frame%32==16)
				spr->y -=1;
			break;

		case mov_bulletL : // 1 frame, l to r
			if (vga_frame%2==0) spr->x-=3;
			break;

		case mov_bulletLv2 : 
			spr->x-=2;
			break;

		case mov_bulletR : // 1 frame, l to r
			if (vga_frame%2==0) spr->x+=3;
			spr->hflip=1;
			break;

		// gravity based ---------------
		case mov_walk : 
			spr->frame = (vga_frame/16)%2; // alternate 2 frames
			if (spr->vx==0) { // start -> TODO move to spawn (or even make it oop)
				// drop to floor
				while (terrain_at(spr->x+spt->hitx1, spr->y+spt->hity2+1)==terrain_empty) 
					spr->y++; 
				spr->vx=1; // start going right
			} else if ( // reverse if obstacle or over a hole
				is_walkable ( terrain_at(
					spr->x+spr->vx+(spr->vx>0?spt->hitx2:spt->hitx1),
					spr->y+spt->hity2
					)) || 
				!is_walkable( terrain_at(
					spr->x+spr->vx+(spr->vx>0?spt->hitx2:spt->hitx1), 
					spr->y+spt->hity2+1)
				) 
			) {
				spr->vx = -spr->vx;
			}

			spr->x += spr->vx;
			spr->hflip = spr->vx<0; // left means reverse
			break;
	} 

}

// -- tilemap-related functions
void get_level_boundingbox(void)
{
	// scan level enclosing rectangle in tiles and fill level_x1,y1,x2,y2
	for (int x=0;x<16;x++) {
		for (int y=0;y<16;y++)
			// any one of the row is the level color ? ok this is the start
			if (get_terrain(y*16+x)==level_color) {
				level_x1=x;
				goto getx2;
			}
	}

	getx2:
	for (int x=15;x>=level_x1;x--) {
		for (int y=0;y<16;y++)
			// any one of the row is the level color ? ok this is the end
			if (get_terrain(y*16+x)==level_color) {
				level_x2=x;
				goto gety1;
			}
	}

	gety1:
	for (int y=0;y<16;y++) {
		for (int x=0;x<16;x++)
			// any one of the row is the level color ? ok this is the start
			if (get_terrain(y*16+x)==level_color) {
				level_y1=y;
				goto gety2;
			}
	}
	
	gety2: 
	for (int y=15;y>=level_y1;y--) {
		for (int x=0;x<16;x++)
			// any one of the row is the level color ? ok this is the end
			if (get_terrain(y*16+x)==level_color) {
				level_y2=y;
				goto finished;
			}
	}

	finished: 
	message("Level found to be from %dx%d-%dx%d\n",level_x1,level_y1,level_x2,level_y2);
}

// Set level start position as color white, replace with tile 0.
void get_level_start()
{
	for (int j=level_y1*16;j<level_y2*16;j++)
		for (int i=level_x1*16;i<level_x2*16;i++) {
			if (data[j*256+i]==get_property(level,1)) {
				// move player
				sprite[0].x=i*16;
				sprite[0].y=j*16;

				data[j*256+i]=0; // TODO replace with nearest empty terrain 
				message("starting position : (%d,%d)\n",sprite[0].x,sprite[0].y);
				return;
			}
		}
	// Not found, set default position on top left of level
	message("using default starting position : (%d,%d)\n",sprite[0].x,sprite[0].y);
	sprite[0].x=level_x1*16;
	sprite[0].y=level_y1*16;
}


// load sprites from tilemap if onscreen, or unload them to tilemap if offscreen
void manage_sprites( void )
{	
	int modified=0;

	// first unload offscreen sprites ( put them back on tilemap)
	for (int i=1;i<MAX_SPRITES;i++) { // 0 is the player :)
		if (sprite[i].type == SPRITE_FREE ) continue;
		else if (sprite[i].x - camera_x + sprtype[sprite[i].type].w < -32 || sprite[i].x-camera_x > VGA_H_PIXELS+32) {
			message("Hiding sprite %d (%d,%d) of type %d outside of screen\n",i,sprite[i].x, sprite[i].y, sprite[i].type);


			if (data[sprite[i].tx!=255 || sprite[i].ty!=255]) {
				uint8_t typ = sprite[i].type;
				bool respawn=true;

				// active : respawn, else re-spawn if typ has specific values
				if (typ>=SPRITE_INACTIVE) {
					typ-=SPRITE_INACTIVE;
					respawn = (typ == col_none || typ==col_kill);
				}
				
				// get type color &put back on tilemap if respawn
				if (respawn) 
					data[sprite[i].ty*256+sprite[i].tx] = get_property(typ+4,property_color); 
			}
			sprite[i].type=SPRITE_FREE;
			modified=1;
		}
	}

	// load sprites
	// TODO : scan only borders after initial scan of visible screen since center is normally done

	for (int j=-2;j<(240/16+2);j++)
		for (int i=-2;i<(320/16+2);i++)
		{
			uint8_t *c = &data[(camera_y/16+j)*256+camera_x/16+i]; // tile id
			for (int spt=0;spt<NB_SPRITETYPES;spt++)
				if (*c==sprtype[spt].color && *c != TRANSPARENT) {
					struct Sprite *spr = spawn_sprite(spt, (camera_x/16)*16 + i*16, (camera_y/16)*16 + j*16);

					// put where we found it in 
					spr->tx = camera_x/16+i;
					spr->ty = camera_y/16+j;
					modified=1;	// will display sth on console
					
					*c=0; 
					// TODO replace with nearest empty
					// TODO : remember init position sur pos vide & type in array of elts 32x(u8 x,y,type), when pos not visible, re-place in tilemap - pas nec tt d'un coup : scanne 1/4 à la fois ? 

					break;
				}
		}

	if (modified) {	
		message("now sprites :");
		for (int i=0;i<MAX_SPRITES;i++) {
		    if (sprite[i].type==SPRITE_FREE)
		    	message ("   ");
			else if (sprite[i].type>=SPRITE_INACTIVE)
	    		message("-%02x",sprite[i].type- SPRITE_INACTIVE );
	    	else 
				message("%02x ",sprite[i].type);
		}
		message("\n");
	}
}


void animate_tilemap(void) {
	// process 1/16th of screen vertically each frame (or maybe just 1/16th of tiles if bigger screen)
	uint8_t tile_line = vga_frame%16; // tile line to process == 0-15 (assumes VGA_V_PIXELS <= 256 )

	// all tiles horizontally : 320/16 = 20 tiles
	for (int i=0;i<VGA_H_PIXELS/16;i++) {	
		uint8_t *c = &data[(camera_y/16+tile_line)*256+camera_x/16+i];
		if (get_terrain(*c)==terrain_animated_empty) {
			if (*c%4!=3) {
				*c +=1 ;
			} else {
				*c -= 3;
			}
		}
	}
}

void sprite_collide_player(struct Sprite *spr)
{	

	struct SpriteType *spt = &sprtype[spr->type];

	switch(spt->collision) { // TODO collide differently top / bottom / sides
		case col_none : 
			break;
		
		message("handling collision of type %d with player !\n", spt->collision);
		case col_coin : 
			// adds coin, small sfx + animations, remove
			coins += 1;
			sprite_kill(spr);
			play_sfx(2); // TODO defs.h

		
			break;

		case col_kill : 
			player_kill();
			break;

		case col_end : 
			level += 1;
			enter_level();
			// TODO small animation (like kill but happy) ?
			break; 

		default : 
			message("unhandled collision type %d\n", spt->collision);
			break;
	}


}

void update_hud()
{
	hud[12]='0'+vga_frame/6000;
	hud[13]='0'+(vga_frame/600)%10;
	hud[14]='0'+(vga_frame/60) %10;

	// TODO keys ?

	hud[17]='0'+coins/10;
	hud[18]='0'+coins%10;
}

// ---------------------------------------------------------------------------------------

void game_init(void)
{
	loader_init();
	if (load_bmp("level0.bmp")) {
		frame_handler = frame_error;
		return;
	}
		
	enter_title(); // title in fact
}


void reset_level_data() 
{
	// reload all data into RAM
	if (load_game_data(data)) {
		frame_handler = frame_error;
		return;
	}
	
	sprites_reset();
	player_reset();
	interpret_spritetypes();

	// apply mapper just once
	uint8_t mapper = data[255*256];
	switch (mapper) {
		case 0 : 
			black_mapper(); 
			break;

		case TRANSPARENT : 
			// null mapper
			break;

		default: 
			message("Unknown mapper ! ");
			frame_handler = frame_error;
			return;
	}

	// interpret level after mapper
	level_color=get_property(level,0);
	// scan level to find bounds and location of starting point.
	get_level_boundingbox();
	get_level_start();

	memcpy(hud,"B0 F0G0 C0 E000 D00  ",sizeof(hud));
	
	hud[1]='0'+lives;
	hud[6]='0'+level+1; 
	

	move_camera(); // avoid being negative
	
	play_song(); // start playing song

	vga_frame=0;
	coins=0;
}

void player_kill()
{
	frame_handler = frame_die;
	stop_song();
	play_sfx(1); // TODO : defs.h

	sprite[0].vy = -6;
	vga_frame=0;
	lives--;
	sprite[0].frame = 4;

}

void frame_die()
{
	// player movement
	if (sprite[0].vy<6 && vga_frame%4==0)
		sprite[0].vy++;
	sprite[0].y += sprite[0].vy;


	// TODO SFX

	if (vga_frame>=60*3) {

		if (lives>0)
			enter_level();
		else 
			enter_title(); 
	}

}

void frame_play()
{
	manage_sprites(); 
	move_player(&sprite[0]);
	move_camera();
	animate_tilemap();

	for (int i=1;i<MAX_SPRITES;i++)
		if (sprite[i].type < SPRITE_INACTIVE ) {
			sprite_move(&sprite[i]);
			if (sprite_collide(&sprite[0], &sprite[i])) {
				sprite_collide_player(&sprite[i]);
			}
		}
	update_hud();
}


void enter_title(void)
{
	if (load_game_data(data)) { 
		frame_handler=frame_error;
		return;
	}

	frame_handler = frame_title;

	// manage_sprites(); // TODO add sprites for title srceen ?

}

void frame_title()
{
	static uint16_t gamepad_oldstate = gamepad_start;

	// move sprites from level 0
	if (gamepad_buttons[0] & ~gamepad_oldstate & gamepad_start) { 

		// reset game
		lives = START_LIVES;
		level = 0;
		//
		enter_level();

	}

	// kinda like animate_tilemap only simpler
	uint8_t tile_line = vga_frame%8; // tile line to process == 0-7
	for (int i=0;i<16;i++) { // process one line each frame.
		uint8_t *c = &data[(TILE_TITLE_Y*16+tile_line)*256+TILE_TITLE_X*16+i];
		if (get_terrain(*c)==terrain_animated_empty) {
			*c += (*c%4!=3) ? 1 : -3;
		}
	}

	gamepad_oldstate = gamepad_buttons[0];
}

void enter_level()
{
	vga_frame=0;
	frame_handler=frame_leveltitle;

	// title hud
	memcpy(hud," B0 GHIHG 1A0       ",20);
	hud[2]='0'+lives;
	hud[12]='0'+level+1;

	camera_y = -100;
}

void frame_leveltitle()
{
	// title animation. TODO cubic / elastic easing ? 
	if (vga_frame<250) {
		float t= (float)vga_frame/250.f;

		camera_y = (int)(-100 + 400*(4*t*t*t + -6*t*t + 3*t));
	} else {
		reset_level_data();
		frame_handler = frame_play;
	}
}

void frame_error()
{
}

void game_frame()
{
	kbd_emulate_gamepad();

	frame_handler(); 

}
