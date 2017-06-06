// edit.c : inline editor for 0xff.
#include "bitbox.h"
#include <memory.h>
#include "game.h"

#include "font.h" // 4x6 font

#define NB_OF(x) (sizeof(x)/sizeof(x[0]))
#define BGCOLOR 148
#define LIGHTGREY  RGB(200,200,200)
#define DARKGREY   RGB(64,64,64) 

// structs 

struct Widget {
	uint8_t height;
	void (*draw)   (int y);
	void (*onclick)(int x, int y);
};

const static char hex_digits[16]="0123456789ABCDEF";
const static uint8_t level_colors[4] = {terrain_level1, terrain_level2, terrain_level3, terrain_level4};
static const struct {uint8_t color; char *label; char *values; char *altvalues;} control_titles[] = {
	{0  , "Ctrl : classic", "< X Walk/Y Lad", "> X Run/Y Grav"},
	{100, "Ctrl : side",    "< Accel/speed ", "> X:autoscroll"},
	{249, "Ctrl : modern",  "< X Walk/Y Lad", "> X Run/Y Grav"},
	{255, "Ctrl : infjumps","< X Walk/Y Lad", "> X Run/Y Grav"},
};

const static struct {uint8_t color; char * text;} terrain_labels[]  = {
    {TRANSPARENT ,     " <None>" },

	{terrain_level1,        " Level 1 map"},
    {terrain_level2,        " Level 2 map"},
    {terrain_level3,        " Level 3 map"},
    {terrain_level4,        " Level 4 map"},
     
	{terrain_empty   ,      " Tile Empty"},
	{terrain_obstacle,      " Tile Obstacle"},
    {terrain_ladder,        " Tile Ladder" },
    {terrain_ice,           " Tile Ice" },
    {terrain_kill,          " Tile Kill"},
	{terrain_animated_empty," Tile Anim Empty"},
    {terrain_anim_kill,     " Tile Anim Kill"},
    {terrain_platform,      " Tile Platform" },
    
	{182,			        " Sprites" },
	{terrain_pattern,       " Music Patterns" },
	
	{0, " (Reserved)"}, 

};



// variables

const struct Widget *panel = 0; // panel
void (*main_area_draw) (); // main area 
void (*main_area_click)(void); // main area click x,y are m_x and m_y directly
char *header_title;


int current_widget_y;
const struct Widget *current_widget;


uint8_t terrain_selected;
uint8_t sprite_selected;

int pan_x,pan_y;
uint8_t control_id; // id in the control_types of the current elvel

int pen; // 0-255 : tileid

int m_x, m_y, d_x, d_y, m_pressed, m_clicked, m_oldpressed; // cursor coordinates / buttons

inline void set_property(const int object_id, const int property, uint8_t value)
{
	const uint8_t x = property+(object_id>=16)*8;
	const uint8_t y = object_id%16;
	data[(0xF0+y)*256+ 16 + x] = value;
}

void update_mouse()
{
	// also moves with cursor, accel
	m_oldpressed = m_pressed;
	m_pressed = gamepad_buttons[0] | mouse_buttons;
	m_clicked = m_pressed & ~m_oldpressed;

	d_x=mouse_x; 
	d_y=mouse_y;
	mouse_x=0;	
	mouse_y=0; 

	if (GAMEPAD_PRESSED(0,left))  d_x=-2;
	if (GAMEPAD_PRESSED(0,right)) d_x=+2;
	if (GAMEPAD_PRESSED(0,up))    d_y=-2;
	if (GAMEPAD_PRESSED(0,down))  d_y=+2;
	
	m_x += d_x;	
	m_y += d_y; 

	if (m_x<0) m_x=0;
	if (m_x>VGA_H_PIXELS) m_x=VGA_H_PIXELS;
	if (m_y<0) m_y=0;
	if (m_y>VGA_V_PIXELS) m_y=VGA_V_PIXELS;
}

void change_level(void) 
{

	if (pan_x>level_x2*16-1) 
		pan_x=level_x2*16-1;
	if (pan_x<level_x1*16) 
		pan_x=level_x1*16;
	if (pan_y>level_y2*16-1) 
		pan_y=level_y2*16-1;
	if (pan_y<level_y1*16) 
		pan_y=level_y1*16;

	// load level control id
	uint8_t c = get_property(level,level_control);	
	for (int i=0;i<NB_OF(control_titles);i++) {
		if (control_titles[i].color==c) {
			control_id = i;
			break;
		}
	}
}

// --------------------------------------------------------------------------------
// draw utilitiles 

inline void draw_line_letter(int x, int y, uint8_t c, uint8_t color)
{
	*(uint32_t *) &draw_buffer[x*4] = font4x8[c-32][y]*color; // simple affectation or |=
}

void draw_textline(int y,const char *str, uint8_t color)
{
	int i=0;
	while (str[i] && i<16) {
		draw_line_letter(256/4+i, y,str[i],color);
		i++;
	}
	while (i<16) {
		*(uint32_t *) &draw_buffer[256+i*4] = 0x01010101*color;
		i++;
	}
}

void draw_grid()
{
	if (vga_line%4==0)	{
		for (int i=0;i<16;i++)
			draw_buffer[i*16]=148;
	}
}

void draw_mouse_cursor()
{
	if (vga_line>=m_y && vga_line<m_y+12) {		
		draw_buffer[m_x]=RGB(0,0,0);
		draw_buffer[m_x+vga_line-m_y]=RGB(0,0,0);
		if (vga_line>m_y)
			memset(draw_buffer + m_x +1 , RGB(255,255,255),vga_line-m_y-1);
	}
}

// --------------------------------------------------------------------------------
// widgets

#define HEIGHT_HEADER 16
void draw_header(int y) 
{
	if (y<8) 
		draw_textline(y,header_title,47);
	else 
		draw_textline(y-8,"\x88\x88 \x82\x83 \x86\x87 \x80\x81 \x84\x85",47);
}

void click_header(int x, int y)
{
	if (m_clicked & mousebut_left && y>8) {
		enter_edit(x/12);
	}
}

void draw_separator (int y)
{
	memset(
		draw_buffer+257,
		y ? LIGHTGREY : DARKGREY,
		318-256
	);
}

void draw_empty (int y)
{
	memset(
		draw_buffer+257,
		RGB(128,128,128),
		318-256
	);
}

// --------------------------------------------------------------------------------
// minimap editor

void draw_tileset_mini(int y)
{
	if (y<64) {
		for (int i=0;i<16;i++)
			for (int j=0;j<4;j++)
				draw_buffer[256+i*4+j] = data[(y*4)*256 + i*16 + j*4];
	} else {
		// draw the tile / color selected
		memset(draw_buffer+256,BGCOLOR,64); // overdraw
		memcpy(draw_buffer+256+24, &data[((pen/16)*16+y%16)*256+(pen%16)*16],16);
	}
}

void click_tileset_mini(int x, int y)
{
	if (y<64)
		pen = (y/4)*16+x/4;
}


void draw_terrain_list(int y) 
{
	// label
	uint8_t color = (y/8 == terrain_selected) ? 0xf4 : BGCOLOR;
	draw_textline(y%8,terrain_labels[y/8].text, color);
	// color of this terrain
    draw_buffer[256+1] = draw_buffer[256+2] = draw_buffer[256+3] = terrain_labels[y/8].color;
}

void click_terrain_list(int x, int y)
{
	// avoid selecting reserved ?
	terrain_selected = y/8;
}

const struct Widget main_panel[] = {
	{16, draw_header, click_header},
	{ 2, draw_separator, 0},
	{NB_OF(terrain_labels)*8, draw_terrain_list, click_terrain_list},
	{ 2, draw_separator, 0},
	{200, draw_empty, 0},
};

void minimap_draw()
{
	// data tiles annotated by minimap, only in 16x15 to fit screen
	memcpy(draw_buffer, &data[(vga_line+vga_line/16)*256],256);

	draw_grid();
	
	// overdraw with tile type - taken from minimap , use 4x6 font, precalc 8bpp grey/black colors
	if (vga_line%15<8) { // height of tiles is 15
		for (int i=0;i<16;i++) {
			const uint8_t c=get_terrain(vga_line/15*16 + i);
			if (c != TRANSPARENT)
				for (int j=0;j<8-vga_line%15;j++) {
					draw_buffer[i*16+j]=c;
				}			
		}
	}
}

void minimap_click(void)
{
	data[(240 + m_y/15)*256 + m_x/16] = terrain_labels[terrain_selected].color;
}



// --------------------------------------------------------------------------------
// level editor
// --------------------------------------------------------------------------------

/*
 *
 * right :  
 *  properties:  
 *     player sprite : non prendre le premier sur l'ecran
 * 	   control
 *     maxtime
 * tile selector

 * song editor
 *    pattern ids a selectionner ... (splitter ?) comme un level en fait ? jouer en boucle le niveau 
 * */


static const char *level_titles[] = {"Level 1", "Level 2", "Level 3", "Level 4"};
void draw_levelselect(int y)
{
	draw_textline(y, level_titles[level], BGCOLOR);
}
void click_levelselect(int x, int y)
{
	if (m_clicked) {	
		level += 1;
		level %= 4;
		get_level_boundingbox();
		change_level();
	}
}


void draw_controlselect(int y)
{
	draw_textline(y, control_titles[control_id].label, BGCOLOR);
}

void click_controlselect(int x, int y)
{
	if (m_clicked) {	
		control_id += 1;
		if (control_id==NB_OF(level_titles))	
			control_id=0;
	}
}


// 4 values XY in the same row
#define HEIGHT_LEVEL_VALUES 32
void draw_level_values(int y)
{

	if (y<16) {

		memset(draw_buffer+256+00,get_property(level, level_accel      )>>4 == y ? 0 : BGCOLOR,16);
		draw_buffer[256+   (get_property(level, level_accel      )&0xf)]=0;
		memset(draw_buffer+256+16,get_property(level, level_maxspeed   )>>4 == y ? 0 : 107,16);
		draw_buffer[256+16+(get_property(level, level_maxspeed   )&0xf)]=0;
		memset(draw_buffer+256+32,get_property(level, level_altaccel   )>>4 == y ? 0 : BGCOLOR,16);
		draw_buffer[256+32+(get_property(level, level_altaccel   )&0xf)]=0;
		memset(draw_buffer+256+48,get_property(level, level_altmaxspeed)>>4 == y ? 0 : 107,16);
		draw_buffer[256+48+(get_property(level, level_altmaxspeed)&0xf)]=0;

	} else if (y<24) {
		draw_textline(y-16, control_titles[level].values, BGCOLOR);
	} else if (y<32) {
		draw_textline(y-24, control_titles[level].altvalues, BGCOLOR);
	}
}

void click_level_values(int x, int y)
{
	if (y<16) {
		const uint8_t property = (const uint8_t[4]){level_accel, level_maxspeed, level_altaccel, level_altmaxspeed}[x/16];
		const uint8_t val =  (y%16)<<4 | x;
		set_property(level, property,val);
	}
}

const struct Widget level_panel[] = {
	{16, draw_header, click_header}, 
	{2,  draw_separator, 0},
	{8,  draw_levelselect, click_levelselect},
	{2,  draw_separator, 0},
	{64+16, draw_tileset_mini,click_tileset_mini},
	{2,  draw_separator, 0},
	{8, draw_controlselect, click_controlselect},
	{2,  draw_separator, 0},
	{HEIGHT_LEVEL_VALUES, draw_level_values, click_level_values}, 
	{2,  draw_separator, 0},
	{200, draw_empty,0},

};

void tilemap_draw()
{
	for (int i=0;i<16;i++) {
		int tile_x = i+pan_x;
		int tile_y = vga_line/16+pan_y;

		// check within level. if not, paint grey
		
		if (get_terrain(tile_x/16+ tile_y/16*16) != level_colors[level]) {
			memset(&draw_buffer[i*16],BGCOLOR,16);
		} else {
			uint8_t tile_id = data[tile_y*256+tile_x];
			memcpy(
				&draw_buffer[i*16], 
				&data[
				((tile_id/16)*16 + vga_line%16)*IMAGE_WIDTH +  // tile line + line ofs			 
			     (tile_id%16)*16]  // tile column
			 	,16);
		}	
	}
}

void tilemap_click(void )
{
	// left : pose currently selected tile
	if (m_pressed & mousebut_left) {
		data[m_x/16+pan_x+(m_y/16+pan_y)*256]=pen;
	} else if (m_pressed & mousebut_right) {
		pen = data[m_x/16+pan_x+(m_y/16+pan_y)*256];
	} else if (m_pressed & mousebut_middle) {
		pan_x -= d_x;
		pan_y -= d_y;
		change_level();
	}
}

// ------------------------------------------------------------------------------------
// Sprite / Object type

/*  object type editor :
 * left = pixel editor pixels 4x4 tiles donc fenetre 64x64 pixels donc
 *
 * 
 * tile ref : (clic on map with middle mouse button) (display hex 0xFF)
 * show movement 
 * 
 * movement : click many times
 * collision
 * spawn
 *  + alts ?
 *
 * paint pen :
 *    (after) tool ?
 *    current color / sel
 *    color sel : 64x64 - pixels de 4x4 ou tile sel
 * 
 * 
 */

static uint8_t movement; // index in movement_labels table
const static struct {uint8_t color; char *label; uint8_t frames;} movement_labels[] = {
	{ mov_nomove,     "1fr static"},
    { mov_alternate1, "2fr static"},
    { mov_alternate2, "2fr static slow"},
    { mov_alternate3, "3fr ABCBA"},
    { mov_alternate4, "4fr ABCDABCD"},

    { mov_throbbing,  "1fr throbbing"},
    { mov_singleanim4,"4fr & destroy"},
    { mov_singleanim2,"2fr & destroy"},
    { mov_singleup8,  "up & destroy"}, 
    { mov_flybounce,  "fly bounce"},
    { mov_walk,       "2fr walk"},
    { mov_bulletL,    "fly left"},
    { mov_bulletR,    "fly right"},
    // player ...    
};

void change_sprite()
{
	// get movement from color
	uint8_t m = get_property(sprite_selected+4, property_movement);
	for (int i=0;i<NB_OF(movement_labels);i++) {
		if (m==movement_labels[i].color) {
			movement=i;
			break;
		}
	}
	// position map
	m = get_property(sprite_selected+4, property_color);
	pan_x = (m%16)*16;
	pan_y = (m/16)*16;
}

#define HEIGHT_SPR_SELECT 32
void draw_spr_select (int y)
{
	uint8_t c=BGCOLOR;
	if (y%8==0) {
		c=LIGHTGREY;
	} else if (y%8==7) {
		c=DARKGREY;
	}
	memset(draw_buffer+256+2,c,8*7);

	for (int i=0;i<7;i++)
	{
		draw_buffer[256+i*8+2]   = LIGHTGREY;
		draw_buffer[256+i*8+7+2] = DARKGREY;
	}

	if (sprite_selected/7==y/8 && y%8 != 0 && y%8!=7) {
		memset(draw_buffer+256+sprite_selected%7*8+3, 0, 6);
	}
}

void click_spr_select(int x, int y)
{
	sprite_selected = (y/8)*7+(x-2)/8;
	change_sprite();
}

void draw_spr_movement(int y) {
	if (y<8)
		draw_textline(y,"Movement: ",BGCOLOR);
	else 
		draw_textline(y-8,movement_labels[movement].label,BGCOLOR);
}

void click_spr_movement(int x, int y) 
{
	if (m_clicked & mousebut_left) 
		movement++;
	else if (m_clicked & mousebut_right) 
		movement--;
	movement %= NB_OF(movement_labels);
}
#define HEIGHT_COLORMAP (64+4)
uint8_t colormap_mode; 
void draw_colormap(int y)
{
	uint8_t c;
	y/=4;
	if (y==16) {
		memset(draw_buffer+256,pen,64);
		return;
	}

	for (int i=0;i<16;i++) {
		switch (colormap_mode) {
			case 0: c = (y<<4 | i); break;
			case 1: 
				// c=(y<<5) | (y<8?0:1)<<4 | (i<8?0:1)<<3 | i%8 | ((vga_frame/64) % 2 ? 1 : 0); // 4 G squares, RRR vertical 0-8, BBL horizontal
				c=(y<<5) | (i&6)<<2 | (i&1) | (y<8?0:1)<<2 | (i<8?0:1)<<1 ; // 4 G squares, RRR vertical 0-8, BBL horizontal
				break;
			default: 
				c=0;
				break;
		}
		*(uint32_t*)&draw_buffer[256+i*4] = c*0x01010101UL;
	}
}

void click_colormap(int x, int y) 
{
	x /= 4; y/=4;
	if (m_pressed & mousebut_left) 
		pen = y<<4 | x;
	else if (m_clicked & mousebut_right) 
		colormap_mode = 1-colormap_mode;
}


const struct Widget sprite_panel[] = {
	{HEIGHT_HEADER,	draw_header, click_header}, 
	{2, draw_separator, 0},
	{HEIGHT_SPR_SELECT, draw_spr_select ,click_spr_select},
	{2, draw_separator, 0},
	{16, draw_spr_movement, click_spr_movement},
	{2, draw_separator, 0},
	{HEIGHT_COLORMAP, draw_colormap, click_colormap},

	{200, draw_empty,0},
};


static void sprite_draw(void)
{
	// only show sprite pixels
	if (pan_y+vga_line/4 >= 256) {
		memset(draw_buffer,DARKGREY,256);
		return;
	}

	for (int i=0;i<64;i++) // or halt before end / set color dark if out of screen
		*(uint32_t*) &draw_buffer[4*i] = data[i+pan_x+256*(pan_y+vga_line/4)]*0x01010101UL;

	// grid
	if ((pan_y*4+vga_line)%64==0)
		for (int i=0;i<32;i++)
			*(uint32_t*) &draw_buffer[8*i] = LIGHTGREY*0x01010101UL;
	else if ((pan_y+vga_line/4)%2)
		for (int i=0;i<5;i++)
			draw_buffer[64*i+(-pan_x)%16*4] = LIGHTGREY;
}

static void sprite_click(void)
{
	if (m_pressed & mousebut_left) {
		data[m_x/4+pan_x+(m_y/4+pan_y)*256]=pen;
	} else if (m_pressed & mousebut_right) {
		pen = data[m_x/4+pan_x+(m_y/4+pan_y)*256];
	} else if (m_pressed & mousebut_middle) {
		pan_x -= d_x;
		if (pan_x<0) pan_x=0;
		if (pan_x>256) pan_x=256;
		pan_y -= d_y;
		if (pan_y<0) pan_y=0;
		if (pan_y>256) pan_y=256;
	}
}

/* pattern / sfx editor :
 * select type
 * show */

/* tileset editor
 *
 * limited to tileset
 *
 * left : 
 *
 * tileset editor : idem sprite editor en fait sans proprietes, retricted to terrain tiles 
 * + definit terrain avec mmb ? dessus 
 * 
 * */


// ----------------------------------------------------------------------------------------------------------------
// General code

void draw_right_panel( void )
{
	if (!current_widget || vga_line == 0) {
		current_widget = panel;
		current_widget_y = 0;
	} else if (vga_line-current_widget_y == current_widget->height) {
		current_widget += 1;
		current_widget_y = vga_line;
	}

	current_widget->draw(vga_line-current_widget_y);

	// borders
	draw_buffer[256] = RGB(255,255,255);
	draw_buffer[319] = RGB(64,64,64);
}

// game data is loaded now and we will edit it
void frame_edit_main()
{
	update_mouse();

	if (mouse_buttons || gamepad_buttons[0] & ( gamepad_A | gamepad_B | gamepad_X ) ) {		
		if (m_x<256) {
			main_area_click();
		} else {
			int y=0;
			for (const struct Widget * w = panel ; ; w++ ) {
				if (m_y<y+w->height) {
					if (w->onclick) w->onclick(m_x-256, m_y-y);
					break;
				}
				y += w->height;
			}
		}
	}

	// TODO : gamepad R/L pressed : next editing mode

	mouse_x=0; mouse_y=0;
}

void line_edit_main()
{
	main_area_draw();
	draw_right_panel();
	draw_mouse_cursor();
}



void enter_edit(uint8_t mode)
{
	message("enter EDIT mode %d\n",mode);
	switch(mode)
	{
		case 0 : 
			main_area_draw = minimap_draw;
			main_area_click = minimap_click;
			panel = main_panel;
			header_title = "\x88\x88 Minimap      ";
			break;
		case 1 : 
			main_area_draw = tilemap_draw;
			main_area_click = tilemap_click;
			panel = level_panel;
			header_title = "\x82\x83 Levels       ";
			get_level_boundingbox();
			change_level();

			break;
		case 2: 
			main_area_draw  = sprite_draw;
			main_area_click = sprite_click;
			panel = sprite_panel;
			header_title = "\x84\x85 Sprites";
			change_sprite();
			break;
	}
	frame_handler = frame_edit_main;
}
