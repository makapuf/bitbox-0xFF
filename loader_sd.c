/* loader for the game */

#include <string.h>
#include <stdlib.h> // qsort

#include "game.h" // common structures
#include "bitbox.h" // message
#include "fatfs/ff.h"


struct __attribute__((packed)) BMPFileHeader {
	char type[2]; // The characters "BM"
	uint32_t size; // The size of the file in bytes
	uint16_t reserved1; // Unused - must be zero
	uint16_t reserved2; // Unused - must be zero
	uint32_t offbits; // Offset to start of Pixel Data
};

struct __attribute__ ((packed)) BMPImgHeader {
	uint32_t headersize; // Header Size - Must be at least 40
	uint32_t width; //Image width in pixels
	uint32_t height; // Image height in pixels
	uint16_t planes; // Must be 1
	uint16_t bitcount; // Bits per pixel - 1, 4, 8, 16, 24, or 32
	uint32_t compression; // Compression type (0 = uncompressed)
	uint32_t sizeimage; // Image Size - may be zero for uncompressed images
	uint32_t xpelspermeter; // Preferred resolution in pixels per meter
	uint32_t ypelspermeter; // Preferred resolution in pixels per meter
	uint32_t clrused; // Number Color Map entries that are actually used
	uint32_t clrimportant; // Number of significant colors
};

#define MAX_FILES 20
#define ROOT_DIR "levels"

static FATFS fs;
static FIL file;

static struct BMPFileHeader file_header;
static struct BMPImgHeader img_header;

// XXX no just cycle and key dir structure
int nb_files;
char filenames[MAX_FILES][13]; // 8+3 +. + 1 chr0


FILINFO fno;
DIR dir;

int loader_init()
{
	memset(&fs, 0, sizeof(FATFS));

	if (f_mount(&fs,"",1) !=  FR_OK ) {  // mount now
    	message("Error mounting filesystem !");die (2,1);
	}
	// change dir 
	if (f_chdir(ROOT_DIR) != FR_OK) {
    	message("Error changing directory !");die (2,2);		
	}
	// open dir
    if (f_opendir(&dir, ".") != FR_OK ) {
    	message("Error opening directory !");die (2,2);
    }

    return 0;
}

int endsWith (char* base, char* str) {
    int blen = strlen(base);
    int slen = strlen(str);
    return (blen >= slen) && (0 == strcmp(base + blen - slen, str));
}

int load_next()
{

    FRESULT res;

    char *fn=0;   /* This function is assuming non-Unicode cfg. */

    do {
	    do {
		    res = f_readdir(&dir, &fno);                   /* Read a directory item */
		    if (res != FR_OK ) {  
		        message("Error reading directory !");
		        die (2,3);
		    }

		    if (fno.fname[0] == 0) { // end of dir ? rescan
		    	f_closedir(&dir);
			    if (f_opendir(&dir, ".") != FR_OK ) {
			    	message("Error reloading directory !");
			    	die (2,4);
			    }
		    }
		} while ( fno.fname[0] == 0 );
		message("debug: next file, now : %s, %d, %x,%x\n",fno.fname,(endsWith(fno.fname,".bmp") || endsWith(fno.fname,".BMP")),fno.fattrib, AM_DIR);


	    if (!(fno.fattrib & AM_DIR)) { // not a dir
	    	// check extension : only keep .bmp files
	    	if (endsWith(fno.fname,".bmp") || endsWith(fno.fname,".BMP")) { // search ignoring case
			    fn = fno.fname; // ok will load it
	    		message("ok, will load %s\n",fn);
	    	}
	    }
	} while (fn==0); 

	message("now loading : %s\n",fn);
	return load_bmp(fn);
}


/* fills global variables headers */
int  load_bmp(const char *filename)
{
	int res;
	unsigned long nb;

	// open file
	res=f_open (&file,filename, FA_READ);
	if (res!=FR_OK){
		message("could not open file %s : error %x \n",filename,res);
		return res;
	}

	res=f_read (&file, &file_header,sizeof(file_header),&nb);
	if (res!=FR_OK){
		message("could not read file %s : error %x \n",filename,res);
		return res;
	}

	message("File header : %c%c %d %d\n", file_header.type[0],file_header.type[1],file_header.size,file_header.offbits);

	res=f_read (&file, &img_header,sizeof(img_header),&nb);
	if (res!=FR_OK){
		message("could not read file %s : error %x \n",filename,res);
		return res;
	}

	message("Image header : %d B. %dx%d planes:%d  bpp:%d compr:%d imgsize:%d colors used:%d\n",
		img_header.headersize, img_header.width, img_header.height,
		img_header.planes, img_header.bitcount,
		img_header.compression, img_header.sizeimage, img_header.clrused
		);

	return 0;
}

/* load N lines of pixels to data from file, reversing data lines.
 assumes no compression from current file position.
 */
static int load_pixels(int nblines, uint8_t *data)
{
	for (int y=nblines-1;y>=0;y--) {
		unsigned long n;
		f_read (&file, &data[y*IMAGE_WIDTH],IMAGE_WIDTH,&n);
		if (n<IMAGE_WIDTH)
			return 250;
		// xxx check n?
	}
	return 0; // OK
}

// 1=OK, 0=error (?)
int load_game_data(uint8_t *data)
{
	// XXX asserts loaded data
	// last line
	f_lseek (&file,file_header.offbits);
	return load_pixels(LEVEL_HEIGHT, data);
}

// rename given file to a .bkp file
static int make_backup(char *filename)
{
	char new_name[14], *c;

	message("renaming %s",fno.fname);
	strncpy(new_name,fno.fname,sizeof(new_name)-1);

	for (c=new_name; *c; c++) {
		if (*c=='.') break;
	}


	c[0]='.';c[1]='b'; c[2]='k'; c[3]='p'; 
	message(" to %s\n",new_name);
	return f_rename(fno.fname,new_name);
	

}

static int save_bmp(char *filename)
{
	int res;
	unsigned long nb, pos=0;

	// close & reopen file
	res=f_open (&file,filename, FA_CREATE_ALWAYS | FA_WRITE);
	if (res!=FR_OK){
		message("could not open file %s for writing : error %x \n",filename,res);
		return res;
	}

	f_write (&file, &file_header,sizeof(file_header),&nb);
	pos +=nb;
	// may write junk after 40 useful bytes ?
	f_write (&file, &img_header ,img_header.headersize,&nb); 
	pos +=nb;
	
	// write color map
	for (int i=0;i<256;i++) {
		uint8_t r = (i&0b11100000);
		uint8_t g = (i&0b00011000)<<3 | (i&1)<<5;
		uint8_t b = (i&0b00000111)<<5;

		uint8_t color[4] = {b|b>>3, g|g>>3, r|r>>3,0}; // RGBA as bgra ?
		f_write(&file, &color,4,&nb);
		pos +=nb;
	}
	
	// write Gap 1 garbage ? 
	if (file_header.offbits-pos) {
		message("Gap1 is %d bytes wide \n",file_header.offbits-pos);
		f_write(&file, data, file_header.offbits-pos, &nb);
		pos +=nb;
	}

	for (int y=LEVEL_HEIGHT-1;y>=0;y--) {
		res = f_write (&file, &data[y*IMAGE_WIDTH],IMAGE_WIDTH,&nb);
	}
	f_close(&file);

	return res;
}


int save_level()
{
	message("saving file %s\n",fno.fname);
	f_close(&file);

	make_backup(fno.fname);
	// save as new file
	int res = save_bmp(fno.fname);
	if (res) {
		message("could not save %s\n",fno.fname);
	} else {
		message("saved %s\n",fno.fname);
	}

	// now reopen file for reading
	res = f_open (&file,fno.fname, FA_READ);
	if (res) {
		message("could not open %s back for reading !",fno.fname);
		die (4,4);
	}
}