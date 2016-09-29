// player related functions
#include <stdbool.h>
#include "bitbox.h"

#include "game.h"

#define NJUMPS 0; // 2 for triple jumps, 0 for single ones
int njumps; // current value for double/triple jumps


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



void player_reset()
{
	// real start of level in get_level_start
	sprite[0].x=0;
	sprite[0].y=0;
	sprite[0].frame=0;
	sprite[0].type =0;
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



void move_player(struct Sprite *spr)
{
	kbd_emulate_gamepad();

	static uint16_t gamepad_oldstate=0;
	const uint16_t gamepad_pressed = gamepad_buttons[0] & ~gamepad_oldstate;

	struct SpriteType *spt = &sprtype[spr->type];

	// FIXME : make terrains bit-testable ? 
	bool on_ground = \
	    terrain_at(spr->x+spt->hitx1,spr->y+spt->hity2+1) == terrain_obstacle || 
		terrain_at(spr->x+spt->hitx2,spr->y+spt->hity2+1) == terrain_obstacle ||
	    terrain_at(spr->x+spt->hitx1,spr->y+spt->hity2+1) == terrain_platform || 
		terrain_at(spr->x+spt->hitx2,spr->y+spt->hity2+1) == terrain_platform;

	bool on_ladder = \
	    terrain_at(spr->x+spt->hitx1,spr->y+spt->hity1+1) == terrain_ladder || 
		terrain_at(spr->x+spt->hitx2,spr->y+spt->hity1+1) == terrain_ladder ||
	    terrain_at(spr->x+spt->hitx1,spr->y+spt->hity2+1) == terrain_ladder || 
		terrain_at(spr->x+spt->hitx2,spr->y+spt->hity2+1) == terrain_ladder;


	// -- movement 
		
	if (vga_frame%4==0) { 
		// Horizontal movement
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

		// on a ladder ? Vertical movement
		if ( on_ladder ) 
		{
			if (GAMEPAD_PRESSED(0,up)) {
				if (spr->vy>-3)
					spr->vy--;
			} else if (GAMEPAD_PRESSED(0,down)) {
				if (spr->vy<3)
					spr->vy++;
			} else {
				// decelerate
				if (spr->vy>0) 
					spr->vy--;
				else if (spr->vy<0)
					spr->vy++;
			}
		} else {
			// Gravity
			spr->vy +=1; 
			if (spr->vy>4) 
				spr->vy=4;
		}
	}



	// Jump 
	if ( gamepad_pressed & gamepad_A ) {
		if (on_ground || on_ladder ) {
			play_sfx(sfx_jump); 
			spr->vy = -6;
			njumps=NJUMPS; // TODO add to leveldef
		} else if (njumps) {
			play_sfx(sfx_jump); 
			spr->vy = -6;
			njumps--;
		}
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
		// would touch down ? (also uses hity2) obstacle or platform or ladder
		// TODO player can hang within platform
		uint8_t t_left =terrain_at( spr->x+spt->hitx1, spr->y+spr->vy + spt->hity2 );
		uint8_t t_right=terrain_at( spr->x+spt->hitx2, spr->y+spr->vy + spt->hity2 );

		if ( 
			  t_left  != terrain_obstacle && 
			  t_right != terrain_obstacle && 

			  t_left  != terrain_platform && 
			  t_right != terrain_platform 
  	    ) {
			spr->y += spr->vy;
		} else if ( !on_ground && !on_ladder ) {
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
	} else if (on_ladder) {
		if (spr->vy) {
			spr->frame=5; // ladder
			if (vga_frame%8==0) {
				spr->hflip = spr->hflip ? 0 : 1;
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

	// -- display debug game info 
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

