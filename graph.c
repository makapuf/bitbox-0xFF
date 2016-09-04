#include <string.h>

/* All screen blitting */

#include "bitbox.h"
#include "game.h"

#if 0
void logo_line8(void)
{
	// display title in the center of the screen (no clipping!)
	char *draw8 = (char*)draw_buffer;

	if ((vga_line>=camera_y) && (vga_line<camera_y+TITLE_HEIGHT)) {
		memcpy(draw8+camera_x,data+(vga_line-camera_y)*256,256);
		for (int i=camera_x+256;i<VGA_H_PIXELS;i++)
			draw8[i]=0;

	} else if (vga_line>=camera_y+TITLE_HEIGHT && vga_line<=camera_y+3*TITLE_HEIGHT/2) {

		int phi = (2*vga_frame+8*vga_line)%256;
		int ofs = sine(phi);
		ofs = ofs*((int)vga_line-camera_y-TITLE_HEIGHT)/(16*64); // scale

		memcpy(
			draw8+camera_x+ofs,
			data+(128-(vga_line-camera_y-TITLE_HEIGHT)*2)*256,
			256
			);

		// right border
		for (int i=camera_x+ofs+256;i<VGA_H_PIXELS;i++)
			draw8[i]=0;

		// a bit darker
		for (int i=0;i<VGA_H_PIXELS;i++)
			draw8[i]&=0b01101011;

	} else {
		memset(draw_buffer,0,VGA_H_PIXELS);
	}
}
#endif 

static inline void blit8(char *draw8, const int chr, const int pos) 
{
	for (int i=0;i<8;i++) {
		uint8_t c=data[15*16*256+64+i+chr*8+(vga_line-8)*256];
		if (c!=TRANSPARENT)
			draw8[pos+i]=c;
	}
}

void screen_line8(void)
{
	// XXX handle x/y offset negative ?
	char *draw8 = (char*)draw_buffer;

	// background draw
	uint8_t tile_id;
	int abs_y = (int)vga_line+camera_y;

	if (abs_y<0) return;

	// first tile (clipped)	

	tile_id = data[(abs_y/16)*IMAGE_WIDTH + camera_x/16];
	memcpy(draw8,
		&data[(tile_id%16)*16 +  // tile column
			  (tile_id/16)*16*IMAGE_WIDTH +  // tile line
			  abs_y%16*IMAGE_WIDTH +  // line offset
			  (camera_x)%16 // offset
			   ],
		(-camera_x)&15 // pos modulo
		);


	// TODO : check this tile of tilemap is inside level 
	// TODO : skip up to end of super tile (16 tiles) and draw once
	// TODO : special cases ?
	for (int tile=-camera_x%16?1:0;tile<VGA_H_PIXELS/16+1;tile++) {
		// read tilemap
		tile_id = data[ (abs_y/16)*IMAGE_WIDTH + tile + camera_x/16];
		memcpy(draw8+tile*16 - camera_x%16 ,
		      &data[((tile_id/16)*16 + abs_y%16)*IMAGE_WIDTH +  // tile line + line ofs			 
		      (tile_id%16)*16]  // tile column
			  ,16);
	}

	// draw all sprites 
	for (int spr_id=MAX_SPRITES-1;spr_id>=0;spr_id--) { // reverse order for draw priority
		struct Sprite *spr=&sprite[spr_id];
		if (spr->type==TRANSPARENT) 
			continue;

		struct SpriteType *spt=&sprtype[spr->type];

		if ( abs_y >= spr->y  && \
			 abs_y <  spr->y + spt->h)
		{
			int start = spr->x-camera_x>=0 ? 0:camera_x-spr->x;
			int end = spr->x+spt->w-camera_x<320 ? spt->w : 320+camera_x-spr->x;
			// TODO start / end are frame-based, avoid recompute ?
			for (int i=start;i<end;i++) { // TODO w=16,32,48. use it!
				uint8_t c=data[
					(spt->y+vga_line-spr->y+camera_y)*IMAGE_WIDTH+\
					spt->x + (spr->hflip?spt->w-1-i:i) + spr->frame*spt->w
					];
				if (c != TRANSPARENT) {
					draw8[spr->x-camera_x+i]=c;
				}
			}
		}
	}

	// TODO return sprites to position if out / killed

	// HUD
	if ((vga_line-8)<8) {
		// lives
		blit8(draw8, 256,8*8);
		blit8(draw8, lives,9*8);

		// level / world
		blit8(draw8, 256+3,11*8);
		blit8(draw8, 256+8,12*8);
		blit8(draw8, 256+3,13*8);
		blit8(draw8, level+1,14*8);

		// time
		blit8(draw8, 256+4,16*8);
		blit8(draw8, vga_frame/6000,17*8);
		blit8(draw8, (vga_frame/600)%10,18*8);
		blit8(draw8, (vga_frame/60)%10,19*8);

		// keys
		
		// coins
		blit8(draw8, 256+6,22*8);
		blit8(draw8, coins/10,23*8);
		blit8(draw8, coins%10,24*8);
	}
}

#define TITLE_OFS_Y 40
#define TITLE_OFS_X 32


static inline void _draw_tiles_title(const int ofs, const int abs_y, const int darken) {
	uint8_t * restrict draw8 = (uint8_t*)draw_buffer;

	for (int tile=0;tile<16;tile++) {
		// read tilemap
		uint8_t tile_id = data[ (abs_y/16+TILE_TITLE_Y*16)*IMAGE_WIDTH + tile + TILE_TITLE_X*16];
		memcpy(ofs + draw8 +tile*16 ,
		      &data[((tile_id/16)*16 + abs_y%16)*IMAGE_WIDTH +  // tile line + line ofs			 
		      (tile_id%16)*16]  // tile column
			  ,16);
	}

	// "a bit" darker
	if (darken==1)
		for (int i=ofs;i<ofs+16*16;i++)
			draw8[i]&=0b01101011;

	// l/r borders
	for (int i=0;i<ofs;i++)
		draw8[i]=0;
	for (int i=ofs+16*16;i<VGA_H_PIXELS;i++)
		draw8[i]=0;
}


void title_line8(void)
{
	int ofs, darken, abs_y;

	// background draw only, with effects
	if ((vga_line>=TITLE_OFS_Y) && (vga_line<TITLE_OFS_Y+TITLE_HEIGHT*3/2)) {
		if (vga_line<TITLE_OFS_Y+TITLE_HEIGHT) {
			ofs=0;
			abs_y = vga_line-TITLE_OFS_Y;
			darken=0;
		} else {
			int phi = (2*vga_frame+8*vga_line)%256;
			ofs = sine(phi) * ((int)vga_line-TITLE_OFS_Y-TITLE_HEIGHT)/(16*64); 
			abs_y = TITLE_HEIGHT-1-(int)(vga_line-TITLE_OFS_Y-TITLE_HEIGHT)*2;
			darken=1;
		}
		ofs += TITLE_OFS_X;
	
		// speedups if inlined + special cased
		switch(ofs&3) {
			case 0: _draw_tiles_title(ofs,abs_y,darken); break;
			case 1: _draw_tiles_title(ofs,abs_y,darken); break;
			case 2: _draw_tiles_title(ofs,abs_y,darken); break;
			case 3: _draw_tiles_title(ofs,abs_y,darken); break;
		}

	} else {
		memset(draw_buffer,0,VGA_H_PIXELS);
	}

}


#define LEVELTITLE_OFS_X ((320-8*16)/2)
#define LEVELTITLE_STOPFR 30

void leveltitle_line8(void)
{
	uint8_t * restrict draw8 = (uint8_t*)draw_buffer;
	int abs_y;

	// Animation. TODO cubic / elastic easing ? 
	if (vga_frame<=LEVELTITLE_STOPFR) {
		abs_y = vga_line-vga_frame*3;
	} else if (LEVELTITLE_STOPFR<vga_frame && vga_frame<=40+LEVELTITLE_STOPFR) {
		abs_y = vga_line-LEVELTITLE_STOPFR*3;
	} else {
		abs_y = vga_line-(vga_frame-40)*3;
	}

	if (abs_y<0 || abs_y>=4*16) {
		memset(draw_buffer,0,VGA_H_PIXELS);
		// top/bottom borders
		if (abs_y==4*16+1 || abs_y ==-2) {
			memset(draw8+LEVELTITLE_OFS_X,0xff,8*16); 
		} else if (abs_y==4*16 || abs_y ==-1) {
			draw8 [LEVELTITLE_OFS_X-1] = draw8[LEVELTITLE_OFS_X+16*8]=0xff;
		}
	} else {
		for (int tile=0;tile<8;tile++) {
			// read tilemap
			uint8_t tile_id = data[ (8+level/2+abs_y/16+TILE_TITLE_Y*16)*IMAGE_WIDTH + tile + TILE_TITLE_X*16+(level%2)*8];
			memcpy(LEVELTITLE_OFS_X + draw8 + tile*16 ,
			      &data[((tile_id/16)*16 + abs_y%16 )*IMAGE_WIDTH +  // tile line + line ofs			 
			      (tile_id%16)*16]  // tile column
				  ,16);
		}

		// black left and right
		for (int i=0;i<LEVELTITLE_OFS_X;i++) draw8[i]=0;
		for (int i=LEVELTITLE_OFS_X+8*16;i<VGA_H_PIXELS;i++) draw8[i]=0;

		// LR border 
		draw8 [LEVELTITLE_OFS_X-2]     =0xff;
		draw8 [LEVELTITLE_OFS_X+16*8+1]=0xff;
	}
}

// used to detect if we need to draw title (intro ?)
void graph_frame(void) {}

void graph_line8()
{
	if (vga_odd) 
		return;

	if (frame_handler==frame_error) 
		memset(draw_buffer,RGB8(0,0,70),VGA_H_PIXELS);
	else if (frame_handler==frame_play || frame_handler==frame_die)
		screen_line8();
	else if (frame_handler==frame_title)
		title_line8();
	else if (frame_handler==frame_leveltitle)
		leveltitle_line8();
	else if (frame_handler==0)
		memset(draw_buffer,RGB8(70,0,70),VGA_H_PIXELS);		
	else {
		message("!!! error unknown frame type \n");
		die(8,4);
	}
}