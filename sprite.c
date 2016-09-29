// all sprite related functions
// see game.h for definitions
#include <bitbox.h>

#include "game.h"

// --Sprites 
struct Sprite sprite[MAX_SPRITES];
struct SpriteType sprtype[NB_SPRITETYPES]; 

void sprites_reset()
{
	for (int i=0;i<MAX_SPRITES;i++) {
		sprite[i].type=SPRITE_FREE; 
	}
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


void interpret_spritetypes()
{	
	for (int id=0;id<NB_SPRITETYPES;id++) { // TODO 16 x 2columns
		struct SpriteType *spt = &sprtype[id];	

		// type of player is read from level , not objects
		if (id==0) {
			spt->color  = get_property(level,1); 
		} else {
			spt->color     = get_property(4+id,0); 
			spt->movement  = get_property(4+id,1);
			spt->collision = get_property(4+id,2);
			spt->spawn     = get_property(4+id,3); 
		}

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


struct Sprite *spawn_sprite(uint8_t type, int x, int y)
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
	sprite[pos].val = 0;
	sprite[pos].x  = x;  sprite[pos].y  = y;
	sprite[pos].vx = 0;  sprite[pos].vy = 0;
	sprite[pos].tx = 255; sprite[pos].ty=255;

	sprite[pos].hflip=0; 

	message ("starting sprite %d type %d @ %d,%d onscreen\n",pos,type,sprite[pos].x-camera_x,sprite[pos].y-camera_y);
	return &sprite[pos];
}


void sprite_kill(struct Sprite *spr)
{
	// needs to spawn something ?	
	const uint8_t spawn = sprtype[spr->type].spawn;
	if (spawn != TRANSPARENT)
		spawn_sprite(spawn,spr->x,spr->y);			
	spr->type+=SPRITE_INACTIVE; // remove : set inactive (type) but not unloaded
}

static inline int is_walkable(uint8_t terrain) {
	return terrain == terrain_obstacle || 
		   terrain == terrain_ice || 
		   terrain == terrain_ladder || 
		   terrain == terrain_platform;
}

static inline int is_blocking(uint8_t terrain) {
	return terrain == terrain_obstacle || 
		   terrain == terrain_ice ;
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

		// a few frames and disappear, going up
		case mov_singleup8 : 
			spr->val++; // value=frame
			spr->y-=2;
			if (spr->val>30)
				sprite_kill(spr);
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
				is_blocking ( terrain_at(
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
			play_sfx(sfx_coin); 

		
			break;

		case col_kill : 
			player_kill();
			break;

		case col_end : 
			level += 1;
			enter_level();
			play_sfx(sfx_level);
			// TODO small animation (like kill but happy) ?
			break; 

		default : 
			message("unhandled collision type %d\n", spt->collision);
			break;
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

void all_sprite_move()
{
	for (int i=1;i<MAX_SPRITES;i++)
		if (sprite[i].type < SPRITE_INACTIVE ) {
			sprite_move(&sprite[i]);
			if (sprite_collide(&sprite[0], &sprite[i])) {
				sprite_collide_player(&sprite[i]);
			}
		}
}