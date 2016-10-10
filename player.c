// player related functions
#include <stdbool.h>
#include "bitbox.h"

#include "game.h"

#define NJUMPS 1 // 2 for double jumps, 1 for single ones, 0 for none
#define SPEED_JUMP 2000 // negative speed in 1/256th pixels per frame

#define ACCEL_X 50
#define MAX_SPEED_X 500

#define ACCEL_Y 70
#define MAX_SPEED_Y 500

#define ACCEL_FALL 100
#define MAX_SPEED_FALL 1000


int njumps; // current value for double/triple jumps


// get the terrain type located at _pixel_ in level position x,y
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
	sprite[0].val=0;
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
	const int sx=spr->x/256;
	const int sy=spr->y/256;

	bool on_ground = \
	    terrain_at(sx+spt->hitx1,sy+spt->hity2+1) == terrain_obstacle || 
		terrain_at(sx+spt->hitx2,sy+spt->hity2+1) == terrain_obstacle ||
	    terrain_at(sx+spt->hitx1,sy+spt->hity2+1) == terrain_platform || 
		terrain_at(sx+spt->hitx2,sy+spt->hity2+1) == terrain_platform;

	bool on_ladder = \
	    terrain_at(sx+spt->hitx1,sy+spt->hity1+1) == terrain_ladder || 
		terrain_at(sx+spt->hitx2,sy+spt->hity1+1) == terrain_ladder ||
	    terrain_at(sx+spt->hitx1,sy+spt->hity2+1) == terrain_ladder || 
		terrain_at(sx+spt->hitx2,sy+spt->hity2+1) == terrain_ladder;


	// -- movement 
		
	// Horizontal movement
	if (GAMEPAD_PRESSED(0,right)) {
		spr->vx += ACCEL_X;
		if (spr->vx > MAX_SPEED_X) 
			spr->vx = MAX_SPEED_X;

	} else if (GAMEPAD_PRESSED(0,left)) {
		spr->vx-=ACCEL_X;
		if (spr->vx <-MAX_SPEED_X) spr->vx = -MAX_SPEED_X;
	} else {
		// decelerate 
		spr->vx-=ACCEL_X;
		if (spr->vx<0) spr->vx = 0;
	}

	// on a ladder ? Vertical movement
	if ( on_ladder ) 
	{
		if (GAMEPAD_PRESSED(0,up)) {
			spr->vy-=ACCEL_Y;
			if (spr->vy<-MAX_SPEED_Y) spr->vy=-MAX_SPEED_Y;
		} else if (GAMEPAD_PRESSED(0,down)) {
			spr->vy+=ACCEL_Y;
			if (spr->vy>MAX_SPEED_Y) spr->vy=MAX_SPEED_Y;
		} else {
			// decelerate
			spr->vy-=ACCEL_Y;
			if (spr->vy<0) spr->vy=0;
		}
	} else {
		// Gravity
		spr->vy += ACCEL_FALL; 
		if (spr->vy>MAX_SPEED_FALL) 
			spr->vy=MAX_SPEED_FALL;
	}


	// Jump 
	if ( gamepad_pressed & gamepad_A ) {
		if (NJUMPS && (on_ground || on_ladder) ) {
			play_sfx(sfx_jump); 
			spr->vy = -SPEED_JUMP;
			njumps=NJUMPS-1; // TODO add to leveldef
		} else if (njumps) {
			play_sfx(sfx_jump); 
			spr->vy = -SPEED_JUMP;
			njumps--;
		}
	}



	// can move vertically / horizontally ? if not, revert (independently)

	// TODO : bigger sprites than 2x2
	// TODO : not out of level

	// test moving horizontally 	
	if (terrain_at( // test top collision
			(spr->x + spr->vx)/256 + (spr->vx>0 ? spt->hitx2:spt->hitx1),
			spr->y/256 + spt->hity1
		) != terrain_obstacle  &&
		terrain_at( // test bottom collision
			(spr->x + spr->vx)/256 + (spr->vx>0 ? spt->hitx2:spt->hitx1),
			spr->y/256 +spt->hity2
			) != terrain_obstacle
		) {
		spr->x += spr->vx;
	} else {
		spr->vx /= 2; 
	}

	if (spr->vy >0 ) {
		// would touch down ? (also uses hity2) obstacle or platform or ladder
		// TODO player can hang within platform
		uint8_t t_left =terrain_at( spr->x/256+spt->hitx1, (spr->y+spr->vy)/256 + spt->hity2 );
		uint8_t t_right=terrain_at( spr->x/256+spt->hitx2, (spr->y+spr->vy)/256 + spt->hity2 );

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
		uint8_t t_left =terrain_at( spr->x/256+spt->hitx1, (spr->y+spr->vy)/256 + spt->hity1 );
		uint8_t t_right=terrain_at( spr->x/256+spt->hitx2, (spr->y+spr->vy)/256 + spt->hity1 );

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
	if (vga_frame%32==0 || button_state() ) {	
		message("player (%d,%d) spd (%d,%d) cam (%d,%d) tile %x",spr->x/256,spr->y/256,spr->vx, spr->vy,camera_x%16,camera_y, terrain);
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


	if (spr->x/256 > camera_x + 200 ) camera_x = spr->x/256-200;
	if (spr->x/256 < camera_x + 100 ) camera_x = spr->x/256>100 ? spr->x/256-100 : 0;

	if (spr->y/256 > camera_y + 150 ) camera_y = spr->y/256-150;
	if (spr->y/256 < camera_y + 50  ) camera_y = spr->y/256-50;

	// check level limits

	if (camera_x<level_x1*256) camera_x=level_x1*256;
	if (camera_y<level_y1*256) camera_y=level_y1*256;

	if (camera_x>level_x2*256-VGA_H_PIXELS+256) camera_x=level_x2*256-VGA_H_PIXELS+256;
	if (camera_y>level_y2*256-VGA_V_PIXELS+256) camera_y=level_y2*256-VGA_V_PIXELS+256;
}


void player_kill()
{
	frame_handler = frame_die;
	stop_song();
	play_sfx(sfx_kill); 

	sprite[0].vy = -SPEED_JUMP;
	vga_frame=0;
	lives--;
	sprite[0].frame = 4;
}
