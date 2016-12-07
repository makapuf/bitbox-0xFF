// player related functions
#include <stdbool.h>
#include "bitbox.h"

#include "game.h"


// values cached for this level from level data
int njumps; // current value for jumps
int std_accel_x, alt_accel_x, std_maxspeed_x, alt_maxspeed_x;
int std_accel_y, alt_accel_y, std_maxspeed_y, alt_maxspeed_y;
int control;

// get the terrain type located at _pixel_ in level position x,y
inline uint8_t terrain_at(int x, int y)
{
	if (y<0 || x<0 || x>256*16 || y>256*16 || get_terrain(y/256*16+x/256)!=current_level_color) {
		message("out of screen\n");
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

	sprite[0].vx=0;
	sprite[0].vy=0;

	// avoid recomputing each frame
	control = get_property(level,level_control);

	std_accel_x = get_property(level,level_accel)%16*16;
	std_accel_y = get_property(level,level_accel)/16*16;
	alt_accel_x = get_property(level,level_altaccel)%16*16;
	alt_accel_y = get_property(level,level_altaccel)/16*16;

	std_maxspeed_x = get_property(level,level_maxspeed)%16*256;
	std_maxspeed_y = get_property(level,level_maxspeed)/16*256;
	alt_maxspeed_x = get_property(level,level_altmaxspeed)%16*256;
	alt_maxspeed_y = get_property(level,level_altmaxspeed)/16*256;

	message("STD X:acc=%d max=%d Y:acc=%d max=%d\n", std_accel_x, std_maxspeed_x, std_accel_y, std_maxspeed_y);
	message("ALT X:acc=%d max=%d Y:acc=%d max=%d\n", alt_accel_x, alt_maxspeed_x, alt_accel_y, alt_maxspeed_y);
}

void move_player()
{
	struct Sprite *spr = &sprite[0];
	struct SpriteType *spt = &sprtype[0];


	// handle player input 
	static uint16_t gamepad_oldstate=0;
	const uint16_t gamepad_pressed = gamepad_buttons[0] & ~gamepad_oldstate;

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

	// walking or running (shortcuts) 
	const bool running = control != control_side && GAMEPAD_PRESSED(0,B);
	const int accel_x    = running ? alt_accel_x : std_accel_x; 
	const int maxspeed_x = running ? alt_maxspeed_x : std_maxspeed_x; 

	if (GAMEPAD_PRESSED(0,right)) {
		spr->vx += accel_x;
		if (spr->vx > maxspeed_x) spr->vx = maxspeed_x;
	} else if (GAMEPAD_PRESSED(0,left)) {
		spr->vx-=accel_x;
		if (spr->vx <-maxspeed_x) spr->vx = -maxspeed_x;
	} else if (spr->vx>0) { // decelerate going left
		spr->vx-=accel_x;
		if (spr->vx<0) spr->vx = 0;
	} else {
		spr->vx+=accel_x;
		if (spr->vx>0) spr->vx = 0;
	}

	// if applicable, vertical movement
	if ( control == control_side || on_ladder )  // jumping on ladders ? 
	{
		if (GAMEPAD_PRESSED(0,up)) {
			spr->vy-=std_accel_y;
			if (spr->vy<-std_maxspeed_y) spr->vy=-std_maxspeed_y;
		} else if (GAMEPAD_PRESSED(0,down)) {
			spr->vy+=std_accel_y;
			if (spr->vy>std_maxspeed_y) spr->vy=std_maxspeed_y;
		} else { // decelerate XXX depends on sign
			spr->vy-=std_accel_y;
			if (spr->vy<0) spr->vy=0;
		}
	} else if (!on_ground) {		
		// Falling / Gravity

		// special : if keep jump button going up, gravity is lower
		if (spr->vy<0 && GAMEPAD_PRESSED(0,A)) {
			spr->vy += std_accel_y;
		} else {
			spr->vy += alt_accel_y; 
		}

		if (spr->vy>alt_maxspeed_y) 
			spr->vy=alt_maxspeed_y;
	}

	// Jump : single if classic, none if side
	if ( gamepad_pressed & gamepad_A && control != control_side ) {
		if ( on_ground || on_ladder ) { // first jump TODO Wall jump ? Cf. collision
			play_sfx(sfx_jump); 
			spr->vy = -alt_maxspeed_y;
			// N more jumps
			switch (control) {
				case control_classic : njumps=0; break;
				case control_modern  : njumps=1; break;
				case control_infjump : njumps=9999; break;
			}
		} else if (njumps) {
			play_sfx(sfx_jump); 
			spr->vy = -alt_maxspeed_y;
			njumps--;
		}
	}



	// can move vertically / horizontally ? if not, revert (independently)

	// TODO : bigger sprites than 2x2
	// TODO : not out of level
	// TODO : unified collisions : with sprites, : move in non-collision side until free.

	// test moving horizontally 
	uint8_t t_top = terrain_at( // test top collision
			(spr->x + spr->vx)/256 + (spr->vx>0 ? spt->hitx2:spt->hitx1),
			spr->y/256 + spt->hity1
		);
	uint8_t t_bottom = terrain_at( // test bottom collision
			(spr->x + spr->vx)/256 + (spr->vx>0 ? spt->hitx2:spt->hitx1),
			spr->y/256 + spt->hity2
		);

	if (t_bottom != terrain_obstacle  && t_top!= terrain_obstacle) {
		spr->x += spr->vx;
	} else {
		//message("Horizontal collision\n");
		spr->vx = 0; 
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
		} else { 
			//message("On ground\n");
			spr->vy =0;
		}
	} else {
		// would touch up ? - obstacle only , uses hity1
		uint8_t t_left =terrain_at( spr->x/256+spt->hitx1, (spr->y+spr->vy)/256 + spt->hity1 );
		uint8_t t_right=terrain_at( spr->x/256+spt->hitx2, (spr->y+spr->vy)/256 + spt->hity1 );

		if (t_left!=terrain_obstacle && t_right != terrain_obstacle) {
			spr->y += spr->vy;
		} else {
			// message("Vertical collision up\n");
			spr->vy =0;
		}
	}


	// -- animation
	if (control==control_side || on_ground) {		
		// animate LR walking frame even if cannot move
		if ((gamepad_buttons[0] & (gamepad_left|gamepad_right))) {
			// if speed is contrary to where we're going, wr're sliding
			if (control!=control_side && ((GAMEPAD_PRESSED(0,left) && spr->vx>0) || (GAMEPAD_PRESSED(0,right) && spr->vx<0))) {
				spr->frame=6;
			} else {			
				if (vga_frame%4==0) {
					if (spr->frame>=2) 
						spr->frame=0;
					else 
						spr->frame++;
				}
			}
		} else {
			if ( // going up / dn ? alternate jump frame
				control==control_side && 
				(gamepad_buttons[0] & (gamepad_up|gamepad_down)) && 
				(vga_frame/4)%2
				)
				spr->frame=3;
			else 
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
	
	if (control==control_side || spr->vx>0) 	
		spr->hflip=0;
	else if (spr->vx<0) 
		spr->hflip=1;

	// collisions player with terrain
	uint8_t terrain = collision_tile(spr);
	if (terrain==terrain_kill || terrain==terrain_anim_kill) 
		player_kill();

	// -- display debug game info 
	if (vga_frame%32==0 || button_state() ) {	
		message("player (%d,%d) spd (%d,%d) frame %d cam (%d,%d) tile %x",spr->x/256,spr->y/256,spr->vx, spr->vy,spr->frame,camera_x%16,camera_y, terrain);
		switch (terrain)
		{
			case terrain_empty  : message(" empty"); break;
			case terrain_obstacle : message(" obstacle"); break;
			case terrain_kill   : message(" kill"); break;
			case terrain_anim_kill   : message(" anim_kill"); break;
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

	sprite[0].vy = -2000;
	vga_frame=0;
	lives--;
	sprite[0].frame = 4;
}
