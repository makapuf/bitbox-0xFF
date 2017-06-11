#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "bitbox.h"

#include "defs.h"
#include "game.h"

int lives;
int coins;
int level; // 0-3
uint8_t current_level_color;
uint8_t level_x1,level_y1,level_x2,level_y2; // bounding box of level in tiles
uint16_t start_x, start_y; // start position on world

uint8_t data[256*256];


int camera_x, camera_y; // vertical position of the title/scroll

void (*frame_handler)( void ); 

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


// -- tilemap-related functions
const uint8_t level_colors[4] = {
	terrain_level1,
	terrain_level2,
	terrain_level3,
	terrain_level4
};

void get_level_boundingbox(void)
{
	current_level_color=level_colors[level];

	// scan level enclosing rectangle in tiles and fill level_x1,y1,x2,y2
	for (int x=0;x<16;x++) {
		for (int y=0;y<16;y++)
			// any one of the row is the level color ? ok this is the start
			if (get_terrain(y*16+x)==current_level_color) {
				level_x1=x;
				goto getx2;
			}
	}

	getx2:
	for (int x=15;x>=level_x1;x--) {
		for (int y=0;y<16;y++)
			// any one of the row is the level color ? ok this is the end
			if (get_terrain(y*16+x)==current_level_color) {
				level_x2=x;
				goto gety1;
			}
	}

	gety1:
	for (int y=0;y<16;y++) {
		for (int x=0;x<16;x++)
			// any one of the row is the level color ? ok this is the start
			if (get_terrain(y*16+x)==current_level_color) {
				level_y1=y;
				goto gety2;
			}
	}
	
	gety2: 
	for (int y=15;y>=level_y1;y--) {
		for (int x=0;x<16;x++)
			// any one of the row is the level color ? ok this is the end
			if (get_terrain(y*16+x)==current_level_color) {
				level_y2=y;
				goto finished;
			}
	}

	finished: 
	message("Level found to be from %dx%d-%dx%d\n",level_x1,level_y1,level_x2,level_y2);
}


// load sprites from tilemap if onscreen, or unload them to tilemap if offscreen
void manage_sprites( void )
{	
	int modified=0;

	// first unload offscreen sprites ( put them back on tilemap)
	for (int i=1;i<MAX_SPRITES;i++) { // 0 is the player :)
		if (sprite[i].type == SPRITE_FREE ) continue;
		else if (
			sprite[i].x/256 - camera_x + sprtype[sprite[i].type].w < -32 || 
			sprite[i].x/256 - camera_x > VGA_H_PIXELS+32 || 
			sprite[i].y/256 - camera_y + sprtype[sprite[i].type].h < -32 || 
			sprite[i].y/256 - camera_y > VGA_V_PIXELS+32 
			) {
			message("Hiding sprite %d (%d,%d) of type %d outside of screen\n",i,sprite[i].x/256, sprite[i].y/256, sprite[i].type);


			if (data[sprite[i].tx!=255 || sprite[i].ty!=255]) {
				uint8_t typ = sprite[i].type;
				bool respawn=true;

				// active : respawn, else re-spawn if typ has specific values
				if (typ>=SPRITE_INACTIVE) {
					typ-=SPRITE_INACTIVE;
					respawn = (typ == col_none || typ==col_kill);
				}
				
				// get type color & put back on tilemap if respawn
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
					struct Sprite *spr = spawn_sprite(spt, 
						(camera_x/16+i)*16*256, 
						(camera_y/16+j)*16*256
						);

					// put where we found it in 
					spr->tx = camera_x/16+i;
					spr->ty = camera_y/16+j;
					modified=1;	// will display sth on console
					
					// modify tilemap with nearest empty (assumes at least one nearby)
					for (int dx=-1;dx<1;dx++)
						for (int dy=-1;dy<1;dy++) {
							uint8_t d=*(c+dx+dy*256);
							if (get_terrain(d)==terrain_empty) { // near one is an empty ? use it
								*c=d;
								goto done;
							}
						}
					*c=*(c-256); // in doubt, set to upper one

					done: 
					break; // found sprite color in index, done
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
		uint8_t t = get_terrain(*c);
		if (t==terrain_animated_empty || t==terrain_anim_kill) {
			if (*c%4!=3) {
				*c +=1 ;
			} else {
				*c -= 3;
			}
		}
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

void next_level()
{
	level += 1;
	enter_leveltitle();
	play_sfx(sfx_level);
	// TODO small animation (like kill but happy), THEN next leveltitle ?
}

void frame_die()
{
	// player movement
	if (sprite[0].vy<2000)
		sprite[0].vy+=80;
	sprite[0].y += sprite[0].vy;

	// TODO SFX

	if (vga_frame>=60*3) {
		if (lives>0)
			enter_leveltitle();
		else 
			enter_title(); 
	}

}


void frame_play()
{
	manage_sprites(); 
	move_player();
	move_camera();
	animate_tilemap();

	all_sprite_move(); // sprite movement and collisions


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
	const uint16_t gamepad_pressed = gamepad_buttons[0] & ~gamepad_oldstate;

	// move sprites from level 0
	if ( gamepad_pressed & gamepad_start) { 

		// reset game
		lives = START_LIVES;
		level = START_LEVEL;
		
		enter_leveltitle();
		
	} else if (gamepad_pressed & gamepad_select) {
		// XXX SFX
		load_next();
		enter_title(); // re-enter title
	} else if ((gamepad_buttons[0] & gamepad_L && gamepad_buttons[0] & gamepad_R) || mouse_buttons ) {
		enter_edit(0);
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

void enter_leveltitle()
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

void game_init(void)
{
	loader_init();
	if (load_next()) {
		frame_handler = frame_error;
		return;
	}
		
	enter_title(); // title in fact
}

void game_frame()
{
	frame_handler(); 
}
