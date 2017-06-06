LEVELS_TMX = castle flappy shooter mario pac

# export those variables to sub makes. (could also use export globally)

NAME := 0xFF
DEFINES += VGA_MODE=320 VGA_BPP=8
#DEFINES += DISABLE_ESC_EXIT
GAME_C_FILES= main.c graph.c loader_sd.c mappers.c sound.c player.c sprite.c edit.c
USE_SDCARD = 1
# if not use sdcard, use loader_flash + make data

LEVELS_BMP = $(LEVELS_TMX:%=levels/%.bmp)

include $(BITBOX)/kernel/bitbox.mk

levels/%.bmp: levels/%.tmx
	python2 tmx2lvl.py $^
main.c: defs.h font.h
defs.h: REFERENCE.md
	./mk_defs.py REFERENCE.md > $@

todo: main.c graph.c loader_sd.c
	grep -n TODO $^
levels: $(LEVELS_BMP)

font.h: 4x8.png font.py 
	python2 font.py > $@

.PHONY: todo 
clean::
	rm -f defs.h
	rm -f $(LEVELS_BMP)
	$(MAKE) -f $(BITBOX)/kernel/bitbox_posix.mk TYPE=sdl clean
	$(MAKE) -f $(BITBOX)/kernel/bitbox_arm.mk BOARD=bitbox clean
