// loader
#include <string.h>

#include "game.h"
#include "bitbox.h" // message
#include "defs.h"

int minstd_rand()
{
	static int s=24154;
	s=(48271*s)%((1UL<<31)-1);
	return s;
}

inline uint8_t minimap(const int x,const int y)
{
	return data[(15*16+y)*256+x];
}


#if 0
// get biggest vertical rectangle possible as w,h from pos
static void search_rect_v(int x, int y ,int *dw, int *dh)
{
	int j,w,h;
	const uint8_t terrain=data[y*256+x];

	for (h=0 ; data[(y+h)*256+x]==terrain && (y + h) < 256; h++ ) ;

	w=0;
	while(1) {
		for (j=0;data[(y+j)*256+x+w]==terrain && j<h;j++); // try all next column
		if (j==h) // full column, continue
			w+=1;
		else
			break;
	}; // continue while we have full comumns

	*dw=w; *dh=h;
}
#endif 

// get biggest horizontal rectangle possible as w,h from pos
static void search_rect_h(int x, int y ,int *dw, int *dh)
{
	int j,w,h;
	const uint8_t terrain=data[y*256+x];

	for (w=1;data[y*256+x+w]==terrain && x+w<256;w++);

	for (h=1;y+h<256;h++) {
		// try all next line
		int all_line=1;
		for (j=0;j<w;j++)
			all_line &= data[(y+h)*256+x+j]==terrain;

		if (!all_line)
			break;
	} // continue while we have full lines

	*dw=w; *dh=h;
}


void inspect_mem (void *ptr, int len)
{
	uint8_t *src=(uint8_t*) ptr;

	for(int i=0;i<len;i++)
	{
		message("%03d ",*src++);
		if(i%IMAGE_WIDTH==IMAGE_WIDTH-1)
			message("\n");
	}
}


// works for terrains AND tiles
static uint8_t mapper_get_terrain(const uint8_t tile_id) 
{
	if (tile_id<64) // this is a tile
		return data[(15*16+(tile_id/16))*256+tile_id%16]; // minimap values
	else {
		switch (tile_id) {
			case blk_terrain_obstacle2 : 
				return blk_terrain_obstacle;
				break;
			case blk_terrain_decor : 
			case blk_terrain_decor2 : 
			case blk_terrain_alt : 
				return blk_terrain_empty;
			default : 
				return tile_id; // other values are the terrain itself.
		}
	}
}

/* interpret level from data (tiles, ..) - modify terrain to tiles inplace */
void black_mapper()
{
	// assert level is raw_loaded

	// ajouter en variables les variations random ?
	int w,h;

	// inspect_mem(data + TILEMAP_START,20);

	// analyze all tilemaps as terrains and replace them with tile ids
	for (int y=4*16;y<15*16;y++)
		for (int x=0;x<256;x++)	{
			uint8_t lvl = minimap(x/16,y/16);
			// TODO check it here 
			if (lvl==TRANSPARENT || (lvl != get_property(0,0) && lvl != get_property(0,1) && lvl != get_property(0,2) && lvl != get_property(0,3))) { // not a level : skip block line
				x += 15;
				continue;
			}
			switch(data[y*256+x]) {
				case blk_terrain_obstacle : 
				case blk_terrain_obstacle2 : 
					search_rect_h(x,y,&w,&h);

					message("   mapper: obstacle %d,%d %dx%d\n",x,y,w,h);
					
					if (h==1) {        // horizontal pipe
						if (w==1) {
							data[y*256+x] = blk_tile_obstacle_unique;
						} else {
							data[y*256+x] = blk_tile_pipe1h;
							for (int j=1;j<w-1;j++)
								data[y*256+x+j] = blk_tile_pipe1h+1;
							data[y*256+x+w-1] = blk_tile_pipe1h+2;
						}
					} else if (w==1) { // vertical width 1 pipe
						data[y*256+x] = blk_tile_pipe1;
						for (int j=1;j<h;j++)
							data[(y+j)*256+x] = blk_tile_pipe1+16;

					} else if (w==2) { // vertical pipe w2
						data[y*256+x]  =blk_tile_pipe2;
						data[y*256+x+1]=blk_tile_pipe2+1;
						for (int j=1;j<h;j++) {
							data[(y+j)*256+x]   = blk_tile_pipe2+16;
							data[(y+j)*256+x+1] = blk_tile_pipe2+16+1;
						}

					} else { // mxn "square"
						for (int j=1;j<h-1;j++) {
							for (int i=1;i<w-1;i++)
								data[x+i+(y+j)*256] = minstd_rand()&0xf0 ? blk_tile_ground : blk_tile_altground; 
							// l/r borders
							data[x+(y+j)*256]=blk_tile_ground-1;
							data[x+w-1+(y+j)*256]=blk_tile_ground+1;
						}
						
						for (int i=1;i<w-1;i++){
							data[x+i+y*256]=blk_tile_ground-16;
							data[x+i+(y+h-1)*256]=blk_tile_ground+16;
						}

						// corners
						data[y*256+x]=blk_tile_ground-16-1;
						data[y*256+x+w-1]=blk_tile_ground-16+1;
						data[(y+h-1)*256+x]=blk_tile_ground+16-1;
						data[(y+h-1)*256+x+w-1]=blk_tile_ground+16+1;

					}
					break;

				case blk_terrain_decor : 
				case blk_terrain_decor2 : 
					search_rect_h(x,y,&w,&h);
					message("    mapper : decor %d,%d %dx%d\n",x,y,h,w);

					if (mapper_get_terrain(data[(y+h)*256+x])==terrain_obstacle) { // under
						if (h==1) {
							if (w==1) {
								data[y*256+x]=blk_tile_decor_one;
							} else {
								data[y*256+x]=blk_tile_decor_h;
								data[y*256+x+w-1]=blk_tile_decor_h+2;
								for (int i=1;i<w-1;i++) data[y*256+x+i]=blk_tile_decor_h+1;
							}
						} else if (w==1) { // vertical decor (but not 1x1)
							data[y*256+x] = blk_tile_decor_v;
							for (int i=1;i<h;i++)
								data[(y+i)*256+x] = blk_tile_decor_v+16;
						} else { // mxn
							for (int j=1;j<h;j++) {
								for (int i=1;i<w-1;i++)
									data[x+i+(y+j)*256] = blk_tile_decor; 
								// l/r borders
								data[x+(y+j)*256]=blk_tile_decor-1;
								data[x+w-1+(y+j)*256]=blk_tile_decor+1;
							}
							
							for (int i=1;i<w-1;i++){
								data[x+i+y*256]=blk_tile_decor-16;
							}

							// corners
							data[y*256+x]=blk_tile_decor-16-1;
							data[y*256+x+w-1]=blk_tile_decor-16+1;
						}
					} else if (mapper_get_terrain(data[(y-1)*256+x])==terrain_obstacle) { // over is blocking
						data[y*256+x]=blk_tile_decor_under;
					} else {
						// whatever the height, we just process one 
						if (w==1) {
							data[y*256+x] = data[y*256+x]==blk_terrain_decor ? blk_tile_cloud : blk_tile_altcloud;						
						} else {
							for (int j=1;j<w-1;j++)
								data[y*256+x+j] = blk_tile_longcloud+1;
							data[y*256+x] = blk_tile_longcloud;
							data[y*256+x+w-1] = blk_tile_longcloud+2;
						}
					} 


					break;

				case blk_terrain_kill : 
					search_rect_h(x,y,&w,&h);

					message("    mapper kill %d,%d %dx%d\n",x,y,h,w);
					if (w==1 && h==1) {
						if (mapper_get_terrain(data[(y+h)*256+x])==terrain_obstacle) { // under
							data[y*256+x]=blk_tile_kill_over;
						} else if (mapper_get_terrain(data[(y-1)*256+x])==terrain_obstacle) { // over
							data[y*256+x]=blk_tile_kill_under;
						} else {
							data[y*256+x]=blk_tile_kill_one;
						}
					} else {
						for (int i=0;i<w;i++) {
							data[y*256+x+i]=blk_tile_water+ (x+i)%2; // first layer
							for (int j=1;j<h;j++)
								data[(y+j)*256+i+x]=blk_tile_water+2; // under 
						}
					}

					break;

				case blk_terrain_empty : 
					data[y*256+x]=blk_tile_empty;
					break;

				case blk_terrain_alt : 
					data[y*256+x]=blk_tile_altbg;
					break;

				case blk_terrain_ladder : 
					data[y*256+x]=blk_tile_ladder;
					break;

			}
	}
}


