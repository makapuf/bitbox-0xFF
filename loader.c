// loader
#include <string.h>

#include "game.h"
#include "bitbox.h" // message
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

// get property id (0-7) from object id 
// objects 0-4 are levels ! 
inline uint8_t get_property(const int object_id, const int property) 
{
	return data[(15*16+object_id%16)*256+property+16 + (object_id>16)];
}

void interpret_spritetypes()
{	
	message("size sprtype:%d\n",sizeof(sprtype));
	for (int id=0;id<NB_SPRITETYPES;id++) { // TODO 16 x 2columns
		struct SpriteType *spt = &sprtype[id];
		spt->color = get_property(4+id,0);
		spt->movement = get_property(4+id,1);
		spt->collision = get_property(4+id,2);

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
			spt->color=TRANSPARENT;
			message ("    spritetype %d in error \n",id);
			continue;
		}

		message("sprtype %d - color %d move %d collision %d speed %d ",id,spt->color, spt->movement, spt->speed);
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
uint8_t get_terrain(const uint8_t tile_id) 
{
	if (tile_id<64) // this is a tile
		return data[(15*16+(tile_id/16))*256+tile_id%16]; // minimap values
	else {
		switch (tile_id) {
			case terrain_obstacle2 : 
				return terrain_obstacle;
				break;
			case terrain_decor : 
			case terrain_decor2 : 
			case terrain_alt : 
				return terrain_empty;
			default : 
				return tile_id; // other values are the terrain itself.
		}
	}
}

/* interpret level from data (tiles, ..) - modify terrain to tiles inplace */
void interpret_terrains()
{
	// assert level is raw_loaded

	// ajouter en variables les variations random ?
	int w,h;

	inspect_mem(data + TILEMAP_START,20);

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
				case terrain_obstacle : 
				case terrain_obstacle2 : 
					search_rect_h(x,y,&w,&h);

					message("obstacle %d,%d %dx%d\n",x,y,w,h);
					
					if (h==1) {        // horizontal pipe
						if (w==1) {
							data[y*256+x] = tile_obstacle_unique;
						} else {
							data[y*256+x] = tile_pipe1h;
							for (int j=1;j<w-1;j++)
								data[y*256+x+j] = tile_pipe1h+1;
							data[y*256+x+w-1] = tile_pipe1h+2;
						}
					} else if (w==1) { // vertical width 1 pipe
						data[y*256+x] = tile_pipe1;
						for (int j=1;j<h;j++)
							data[(y+j)*256+x] = tile_pipe1+16;

					} else if (w==2) { // vertical pipe w2
						data[y*256+x]  =tile_pipe2;
						data[y*256+x+1]=tile_pipe2+1;
						for (int j=1;j<h;j++) {
							data[(y+j)*256+x]   = tile_pipe2+16;
							data[(y+j)*256+x+1] = tile_pipe2+16+1;
						}

					} else { // mxn "square"
						for (int j=1;j<h-1;j++) {
							for (int i=1;i<w-1;i++)
								data[x+i+(y+j)*256] = minstd_rand()&0xf0 ? tile_ground : tile_altground; 
							// l/r borders
							data[x+(y+j)*256]=tile_ground-1;
							data[x+w-1+(y+j)*256]=tile_ground+1;
						}
						
						for (int i=1;i<w-1;i++){
							data[x+i+y*256]=tile_ground-16;
							data[x+i+(y+h-1)*256]=tile_ground+16;
						}

						// corners
						data[y*256+x]=tile_ground-16-1;
						data[y*256+x+w-1]=tile_ground-16+1;
						data[(y+h-1)*256+x]=tile_ground+16-1;
						data[(y+h-1)*256+x+w-1]=tile_ground+16+1;

					}
					break;

				case terrain_decor : 
				case terrain_decor2 : 
					search_rect_h(x,y,&w,&h);
					message("decor %d,%d %dx%d\n",x,y,h,w);

					if (get_terrain(data[(y+h)*256+x])==terrain_obstacle) { // under
						if (h==1) {
							if (w==1) {
								data[y*256+x]=tile_decor_one;
							} else {
								data[y*256+x]=tile_decor_h;
								data[y*256+x+w-1]=tile_decor_h+2;
								for (int i=1;i<w-1;i++) data[y*256+x+i]=tile_decor_h+1;
							}
						} else if (w==1) { // vertical decor (but not 1x1)
							data[y*256+x] = tile_decor_v;
							for (int i=1;i<h;i++)
								data[(y+i)*256+x] = tile_decor_v+16;
						} else { // mxn
							for (int j=1;j<h;j++) {
								for (int i=1;i<w-1;i++)
									data[x+i+(y+j)*256] = tile_decor; 
								// l/r borders
								data[x+(y+j)*256]=tile_decor-1;
								data[x+w-1+(y+j)*256]=tile_decor+1;
							}
							
							for (int i=1;i<w-1;i++){
								data[x+i+y*256]=tile_decor-16;
							}

							// corners
							data[y*256+x]=tile_decor-16-1;
							data[y*256+x+w-1]=tile_decor-16+1;
						}
					} else if (get_terrain(data[(y-1)*256+x])==terrain_obstacle) { // over is blocking
						data[y*256+x]=tile_decor_under;
					} else {
						// whatever the height, we just process one 
						if (w==1) {
							data[y*256+x] = data[y*256+x]==terrain_decor ? tile_cloud : tile_altcloud;						
						} else {
							for (int j=1;j<w-1;j++)
								data[y*256+x+j] = tile_longcloud+1;
							data[y*256+x] = tile_longcloud;
							data[y*256+x+w-1] = tile_longcloud+2;
						}
					} 


					break;

				case terrain_kill : 
					search_rect_h(x,y,&w,&h);

					message("kill %d,%d %dx%d\n",x,y,h,w);
					if (w==1 && h==1) {
						if (get_terrain(data[(y+h)*256+x])==terrain_obstacle) { // under
							data[y*256+x]=tile_kill_over;
						} else if (get_terrain(data[(y-1)*256+x])==terrain_obstacle) { // over
							data[y*256+x]=tile_kill_under;
						} else {
							data[y*256+x]=tile_kill_one;
						}
					} else {
						for (int i=0;i<w;i++) {
							data[y*256+x+i]=tile_water+ (x+i)%2; // first layer
							for (int j=1;j<h;j++)
								data[(y+j)*256+i+x]=tile_water+2; // under 
						}
					}

					break;

				case terrain_empty : 
					data[y*256+x]=tile_empty;
					break;
				case terrain_alt : 
					data[y*256+x]=tile_altbg;
					break;

				case terrain_ladder : 
					data[y*256+x]=tile_ladder;
					break;

			}


	}

	game_state = FS_Interpreted;
}


