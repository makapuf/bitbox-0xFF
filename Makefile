NAME := 0xFF

DEFINES += VGA_MODE=320 VGA_BPP=8
#DEFINES += DISABLE_ESC_EXIT
LEVELS_TMX = castle flappy shooter mario pac

GAME_C_FILES= main.c graph.c loader_sd.c mappers.c sound.c player.c sprite.c
USE_SDCARD = 1
# if not usesdcard, use loader_flash + make data

LEVELS_BMP = $(LEVELS_TMX:%=levels/%.bmp)

include $(BITBOX)/kernel/bitbox.mk
main.c: defs.h $(LEVELS_BMP)

levels/%.bmp: levels/%.tmx
	python2 tmx2lvl.py $^

defs.h: REFERENCE.md
	./mk_defs.py REFERENCE.md > $@

todo: main.c graph.c loader_sd.c
	grep -n TODO $^

.PHONY: todo 
clean::
	rm -f defs.h
	rm -f $(LEVELS_BMP)
