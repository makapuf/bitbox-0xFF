// edit.c : inline editor for 0xff.
#include "bitbox.h"
#include <memory.h>
#include "game.h"

#include "font.h" // 4x6 font

int m_x, m_y;

uint8_t terrain_selected=0;

const static char hex_digits[16]="0123456789ABCDEF";

const static struct {uint8_t color; char label; char * text;} terrain_labels[]  = {
	{terrain_empty   , 'S',"[S]ky (Empty)"},
	{terrain_obstacle, 'O',"[O]bstacle"},
    {terrain_ladder,   'L', "[L]adder" },
    {terrain_ice,      'I', "[I]ce" },
    {terrain_kill,     'K',"[K]ill"},
	{terrain_animated_empty,'E',"Anim. [E]mpty"},
    {terrain_anim_kill,'X',"[X] Anim. kill"},
    {terrain_platform, 'P', "[P]latform" },
    
    {TRANSPARENT ,     ' ', "<None>" },
};
#define NB_OF(x) (sizeof(x)/sizeof(x[0]))

// game data is loaded now and we will edit it
// user must have access to a mouse or a gamepad
// main == minimap editor, will give access to the others
void frame_edit_main()
{
	m_x += mouse_x;	m_y += mouse_y; mouse_x=0;mouse_y=0;
	if (m_x<0) m_x=0;
	if (m_x>VGA_H_PIXELS) m_x=VGA_H_PIXELS;
	if (m_y<0) m_y=0;
	if (m_y>VGA_V_PIXELS) m_y=VGA_V_PIXELS;

	// also moves with cursor, accel

	// drawing - spritedefs have priority !
	if (mouse_buttons & mousebut_left) {
		if (m_x<256) {
			data[(240 + m_y/15)*256 + m_x/16] = terrain_labels[terrain_selected].color;
		} else if (m_y<NB_OF(terrain_labels)*8) {
			terrain_selected = m_y/8;
		}
	}
}

inline void draw_line_letter(int x, int y, char c)
{
	*(uint32_t *) &draw_buffer[x*4] = font4x6[c-32][y]; // simple affectation or |=
}

// draw it here in several colors ?
const char map_label(uint8_t color)
{
	for (int i=0;i<NB_OF(terrain_labels);i++)
		if (terrain_labels[i].color==color)
			return terrain_labels[i].label;
			
	return '?';
}


void line_edit_main()
{
	// data tiles annotated by minimap, only in 16x15 to fit screen
	memcpy(draw_buffer, &data[(vga_line+vga_line/16)*256],256);

	// grid
	if (vga_line%4==0)	{
		for (int i=0;i<16;i++)
			draw_buffer[i*16]=148;
	}
	
	// overdraw with tile type - taken from minimap , use 4x6 font, precalc 8bpp grey/black colors
	if (vga_line%15<6) { // height of tiles is 15
		for (int i=0;i<16;i++) {
			const char c=map_label(get_terrain(vga_line/15*16 + i));
			if (c!=' ')
				draw_line_letter(i*4,vga_line%15,c);
			
		}
	}

	// TODO : overdraw if already used as sprite / standard tile / ... (to signal usage - but allows it if needed)
	
	
	// right : draw edit box
	memset(draw_buffer+257,RGB(128,128,128),318-256);

	// list types
	if (vga_line<NB_OF(terrain_labels)*8) {
		for (int i=0;i<4*8;i++) {
			if (terrain_labels[vga_line/8].text[i] && vga_line%8<6)
				draw_line_letter(1+256/4+i, vga_line%8,terrain_labels[vga_line/8].text[i]);
			else
				break;
		}
	}
	// selected ?
	if (vga_line/8 == terrain_selected) {
	    draw_buffer[256+2] = 0xf4;
	    draw_buffer[256+3] = 0xf4;
	}
	
	// borders
	draw_buffer[256] = RGB(255,255,255);
	draw_buffer[319] = RGB(64,64,64);


	// bottom : switch between tile editor, map editor, minimap editor, sprite editor    

	// (ugly) mouse cursor
	if (vga_line>=m_y && vga_line<m_y+12) {		
		draw_buffer[m_x]=RGB(0,0,0);
		draw_buffer[m_x+vga_line-m_y]=RGB(0,0,0);
		if (vga_line>m_y)
			memset(draw_buffer + m_x +1 , RGB(255,255,255),vga_line-m_y-1);
	}
}


/*  sprite editor :
 * left = pixel editor pixels 4x4 tiles donc fenetre 64x64 pixels donc 
 * right :
 * 
 * select sprite 0-16 : direct 16x8 buttons : 4 lignes
 * select tile (map avec 4x4)
 * select type : click many times
 * movement:  click / subclick ou juste click.
 * kill : idem
 * 
 * color sel : 64x64 - pixels de 4x4
*/


/* tileset editor : idem tile editor en fait a part proprietes ? + definit terrain dessus, pas de minimap ??  */ 

/* level editor : paint ok mais ne peut peindre que sur le terrain ? unlock en ligne ? */

/* song editor */ 

/* instr editor */

