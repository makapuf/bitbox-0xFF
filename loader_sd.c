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

// XXX no just cycle and keey dir structure
int nb_files;
char filenames[MAX_FILES][13]; // 8+3 +. + 1 chr0
int current_file;
char full_path[80]; // not on stack

// compare simplt two names
static int cmp(const void *p1, const void *p2){
    return strcmp( (char * const ) p1, (char * const ) p2);
}

int loader_init()
{
	memset(&fs, 0, sizeof(FATFS));
	return f_mount(&fs,"",1); // mount now
	// open dir here ?
}

void scan_files()
{

    FRESULT res;
    FILINFO fno;
    DIR dir;

    char *fn;   /* This function is assuming non-Unicode cfg. */

    res = f_opendir(&dir, ROOT_DIR);                       /* Open the root directory */
    if (res == FR_OK) {
        for (nb_files=0;nb_files<MAX_FILES;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            // if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
            fn = fno.fname;
            if (!(fno.fattrib & AM_DIR)) { // not a dir
            	// check extension : only keep .bmp files
            	if (strstr(fn,".bmp") || strstr(fn,".BMP")) { // search ignoring case
                	strcpy(filenames[nb_files],fn);
                	nb_files +=1;
            	}
            }
        }
        if (res != FR_OK) {
	        message("Error reading directory !");
        }
        f_closedir(&dir);
    } else {
        message("Error opening directory !");
    }

    // sort it
    qsort(filenames, nb_files, 13, cmp);
}


/* fills global variables headers */
int  load_bmp(const char *filename)
{
	int res;
	unsigned long nb;

	// make full path
	strcpy(full_path,ROOT_DIR);
	strcat(full_path,"/");
	strcat(full_path,filename);

	// mount drive


	res=f_open (&file,full_path, FA_READ);
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


void close_file()
{
	f_close(&file);
}