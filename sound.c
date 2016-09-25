// music-related elements

#include <stdint.h>
#include "bitbox.h"
#include "game.h"


// songtempo in level+song def apres level titles. 
// 4x (1 line=4x4 u8 for instrs, 3 lines=48patterns refs to patterns. 4 lines per level, 16 beats, 4 lines per level)

// sfx = instr (4) + speed + pattern=11 notes (saut, kill, tir/hit, ) to be played stealing channel 0 (3?)

/* todo : instruments ! include instyument in in sfx / song note play */ 

// TO reference 
#define NB_CHANNELS 4
#define SFX_TILE 0xF4
#define SONGS_TILE 0xF3
#define SFX_REPLACES_CHAN 2 // SFX will be played instead of channel 

struct oscdef {
	// instr def
	uint8_t waveform; // 1: square 12%; 2:square 25%, 3:square 50%, 4:triangle, 5:noise -> refs
	uint8_t sustain; // stay constant for x ticks then decay
	uint8_t decay;   // decay of volume per frame, 0=infinite note, 10:abrupt 
	// attack/decay 2D
	// i8 fsweep/lfo ? 2d
	// bitcrush ..

	
	// frame based
	uint8_t volume; // current volume. 
	uint8_t instfr; // frame in instrument. max 256 = 4s.
	uint16_t freq; // dt in samples ?

	// sample
	uint16_t phase;
} osc[NB_CHANNELS+1]; // last channel is for SFX.

// Song position
static int frame; // 1/60s
static int tick;  // 1/4 note
static int pattern; // 16 ticks
static int play; // 0 : dont play, 1 : play song
int speed=9;

// sfx position.
static int sfx_playing=-1; // 0-15, <0 = none
static int sfx_tick; 
static int sfx_speed=1;


static uint32_t lcgrng=1;

void play_song() 
{
	// defaults
	for (int i=0;i<NB_CHANNELS;i++) {
		osc[i].decay=6; // speed
		osc[i].sustain=5; // 1/60s max ~ 70
		osc[i].waveform=3;
	}

	tick=pattern=frame=0;
	play=1;
}

void stop_song()
{
	play=0;
	for (int i=0;i<NB_CHANNELS;i++)
		osc[i].volume=0;
}

void play_sfx(int n) // n=0-15 or -1 to stop
{
	sfx_playing=n;
	sfx_tick=0;	
	sfx_speed=read_tile(SFX_TILE,0,n);
}


// generate one sample from the oscillators
static inline uint16_t gen_sample()
{
	uint16_t v;

	int accum=0;
	for (int n=0;n<=NB_CHANNELS;n++) {
		// skip rendering a channel if sfx is playing or sfx(=last)  channel  if not 
		if (n==((sfx_playing<0) ? NB_CHANNELS : SFX_REPLACES_CHAN)) 
			continue; 

		osc[n].phase+=osc[n].freq/4;
		v=0;
		switch (osc[n].waveform) {
			case 0 : // square 12%
				v = osc[n].phase>(0x7fff/4) ? 255 : 0; 
				break; 
			case 1 : // square 25%
				v = osc[n].phase>(0x7fff/2) ? 255 : 0; 
				break; 
			case 2 : // square 50%
				v = osc[n].phase>0x7fff ? 255 : 0; 
				break; 
			case 3 : // sawtooth
				v = osc[n].phase>>8;
				break;
			case 4 : // triangle
				v = osc[n].phase < 0x8000  ? osc[n].phase >> 7  : 0xff - ((osc[n].phase - 0x8000) >> 7);
				break;
			case 5 : // LFSR / modulated by freq 
				// XXX  make it more like APU in NES
				if (osc[n].phase>0x7fff) { // ???
					v = lcgrng&0x10000?0xff:0;
				}
				break;

			default : 
				// silence/off
				v=0;
				break;
			} // switch
		accum += v*osc[n].volume; // 16 bit unsigned
	} // oscn

	return accum>>11;
}

static const uint16_t freqtable[] = {
	0x010b, 0x011b, 0x012c, 0x013e, 0x0151, 0x0165, 0x017a, 0x0191, 
	0x01a9,	0x01c2, 0x01dd, 0x01f9, 0x0217, 0x0237, 0x0259, 0x027d, 
	0x02a3, 0x02cb,	0x02f5, 0x0322, 0x0352, 0x0385, 0x03ba, 0x03f3, 
	0x042f, 0x046f, 0x04b2,	0x04fa, 0x0546, 0x0596, 0x05eb, 0x0645, 
	0x06a5, 0x070a, 0x0775, 0x07e6,	0x085f, 0x08de, 0x0965, 0x09f4, 
	0x0a8c, 0x0b2c, 0x0bd6, 0x0c8b, 0x0d4a,	0x0e14, 0x0eea, 0x0fcd, 
	0x10be, 0x11bd, 0x12cb, 0x13e9, 0x1518, 0x1659,	0x17ad, 0x1916, 
	0x1a94, 0x1c28, 0x1dd5, 0x1f9b, 0x217c, 0x237a, 0x2596,	0x27d3, 
	0x2a31, 0x2cb3, 0x2f5b, 0x322c, 0x3528, 0x3851, 0x3bab, 0x3f37, 
	0x42f9, 0x46f5, 0x4b2d, 0x4fa6, 0x5462, 0x5967, 0x5eb7, 0x6459, 
	0x6a51,	0x70a3, 0x7756, 0x7e6f 
};

static inline uint8_t color2note(uint8_t pix)
{
	uint8_t oct=8-(pix/32); 
	uint8_t note=pix%16;
	return 24+12*oct+note;
}

// start a given note (0-127) on a given channel
static void start_note(int n,uint8_t note )
{
	osc[n].volume=0xb0;
	osc[n].freq=freqtable[note]/4; 
	osc[n].instfr=0; // frame of the instr
}


static void update_song()
{
	if (!play) return;

	if (frame==speed) {
		for (int chan=0;chan<NB_CHANNELS;chan++) 
		{
			uint8_t patid=read_tile(SONGS_TILE,pattern,1); // TODO 3 lines song, stop
			if (patid != TRANSPARENT) {
				uint8_t pix=read_tile(patid,tick,chan+level*4);
				if ( pix!=TRANSPARENT && chan!=3 ) {
					start_note(chan,color2note(pix));
				}
			}
		}

		frame=0;
		tick +=1;
		if (tick==16) {
			pattern+=1;
			tick=0;
			message("in pattern %d\n",pattern);
		}
	}
	frame += 1;
}

static void update_sfx()
{
	if (sfx_playing<0) return; // no SFX playing
	uint8_t * const instfr=&osc[NB_CHANNELS].instfr; // shortcut

	if (*instfr==sfx_speed) {

		uint8_t pix=read_tile(SFX_TILE,sfx_tick+5,sfx_playing);
		if (pix!=TRANSPARENT) { // TODO stop
			start_note(NB_CHANNELS,color2note(pix));
		} else {
			osc[NB_CHANNELS].volume=0;
			sfx_playing=-1;
		}
		*instfr=0;
		sfx_tick+=1;
	}
	osc[NB_CHANNELS].instfr+=1;
}

static void update_osc()
{
	for (int n=0;n<NB_CHANNELS;n++) { // voice
		osc[n].instfr++;
		if ( osc[n].instfr > osc[n].sustain ) {
			// decay phase
			if (osc[n].volume>=osc[n].decay) 
				osc[n].volume-=osc[n].decay;
			else 
				osc[n].volume=0;
		}
	}
}

void game_snd_buffer(uint16_t *buffer, int len) {
	// process frame-based info ~ 60Hz (len=512 samples @ 32kHz)

	update_song(); // update song playing 
	update_sfx();  
	update_osc();  // update osc values

	// process sound based info to left/r channels

	for (int i=0;i<len;i++) {
		lcgrng = lcgrng * 1664525 + 1013904223; // updated each time
		uint16_t v=gen_sample();
		buffer[i]+=(v)*0x0101;
	} // samples

}
