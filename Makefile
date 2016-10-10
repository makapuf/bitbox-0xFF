NAME := 0xFF

DEFINES += VGA_MODE=320 VGA_BPP=8
#DEFINES += DISABLE_ESC_EXIT

GAME_C_FILES= main.c graph.c loader_sd.c mappers.c sound.c player.c sprite.c
USE_SDCARD = 1
# if not usesdcard, use loader_flash + make data

include $(BITBOX)/kernel/bitbox.mk
main.c: defs.h
defs.h: REFERENCE.md
	./mk_defs.py REFERENCE.md > $@

todo: main.c graph.c loader_sd.c
	grep -n TODO $^

.PHONY: todo 
clean::
	rm -f defs.h
