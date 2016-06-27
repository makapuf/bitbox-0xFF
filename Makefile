NAME := sideproject

DEFINES += VGAMODE_320
#DEFINES += DISABLE_ESC_EXIT

GAME_C_FILES= main.c graph.c loader_sd.c loader.c
#USE_SAMPLER=1
USE_SDCARD = 1
# if not usesdcard, use loader_flash + make data
	
include $(BITBOX)/lib/bitbox.mk

main.c:game.h

todo: main.c graph.c loader_sd.c
	grep -n TODO $^

.PHONY: todo 