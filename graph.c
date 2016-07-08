#include <string.h>

/* All screen blitting */

#include "bitbox.h"
#include "game.h"

void title_line8(void)
{
	// display title in the center of the screen (no clipping!)
	// xxx wavy water effect ...
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

	// start tile (clipped)	

	tile_id = data[TILEMAP_START + (abs_y/16)*IMAGE_WIDTH + camera_x/16];
	memcpy(draw8,
		&data[(tile_id%16)*16 +  // tile column
			  (tile_id/16)*16*IMAGE_WIDTH +  // tile line
			  abs_y%16*IMAGE_WIDTH +  // line offset
			  (camera_x)%16 // offset
			   ],
		(-camera_x)&15 // pos modulo
		);


	// TODO : check this tile of tilemap is inside level 
	// TODO : skip if no offset
	for (int tile=-camera_x%16?1:0;tile<VGA_H_PIXELS/16+1;tile++) {
		// read tilemap
		tile_id = data[TILEMAP_START + (abs_y/16)*IMAGE_WIDTH + tile + camera_x/16];
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

// used to detect if we need to draw title (intro ?)
extern void frame_title (void);
extern void frame_error (void);

void graph_frame(void) {}
void graph_line8()
{
	// vga_odd ?
	if (vga_odd) return;

	if (frame_handler == frame_title)
		title_line8();
 	else if (frame_handler== frame_error) 
		// dark blue
		memset(draw_buffer,RGB8(0,0,70),VGA_H_PIXELS);
	else 
		screen_line8();
}