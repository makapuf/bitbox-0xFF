LEVELS_TMX = castle flappy shooter mario pac

# export those variables to sub makes. (could also use export globally)

export NAME := 0xFF
export DEFINES += VGA_MODE=320 VGA_BPP=8
#DEFINES += DISABLE_ESC_EXIT
export GAME_C_FILES= main.c graph.c loader_sd.c mappers.c sound.c player.c sprite.c
export USE_SDCARD = 1
# if not use sdcard, use loader_flash + make data


LEVELS_BMP = $(LEVELS_TMX:%=levels/%.bmp)

all: emu $(LEVELS_BMP)

emu: defs.h 
	$(MAKE) -f $(BITBOX)/kernel/bitbox_posix.mk TYPE=sdl
bitbox : defs.h 
	$(MAKE) -f $(BITBOX)/kernel/bitbox_arm.mk BOARD=bitbox


levels/%.bmp: levels/%.tmx
	python2 tmx2lvl.py $^

defs.h: REFERENCE.md
	./mk_defs.py REFERENCE.md > $@

todo: main.c graph.c loader_sd.c
	grep -n TODO $^


.PHONY: todo all emu bitbox
clean::
	rm -f defs.h
	rm -f $(LEVELS_BMP)
	$(MAKE) -f $(BITBOX)/kernel/bitbox_posix.mk TYPE=sdl clean
	$(MAKE) -f $(BITBOX)/kernel/bitbox_arm.mk BOARD=bitbox clean
