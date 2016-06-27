#include <string.h>

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

	// TODO flip with direction / type

	// draw all sprites 
	for (int spr_id=MAX_SPRITES;spr_id>=0;spr_id--) { // reverse order for draw priority
		struct Sprite *spr=&sprite[spr_id];
		if (spr->type==TRANSPARENT) 
			continue;

		struct SpriteType *spt=&sprtype[spr->type];

		if ( abs_y >= spr->y  && \
			 abs_y <  spr->y + spt->h)
		{
			if (spr->x-camera_x>=0 && spr->x+spt->w-camera_x<320) { // TODO better clipping
				for (int i=0;i<spt->w;i++) { // w=16,32,48. use it!
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
	}

	// TODO return sprites to position if out / killed

	// HUD
	if (vga_line<8) {
		// lives
		// time
		// level / world

	}

}

void graph_frame(void) {}
void graph_line8()
{
	// vga_odd ?
	if (vga_odd) return;

	if (game_state==FS_Title)
		title_line8();
 	else if (game_state==FS_Interpreted)
		screen_line8();
	else {
		// dark blue
		memset(draw_buffer,RGB8(0,0,70),VGA_H_PIXELS);
	}
}