NAME = FORSKN3D
DESCRIPTION = "Forsaken-style survival"
ICON = assets/gfx/program_icon.png
COMPRESSED = YES
SRCDIR = assets
GFXDIR = assets/gfx
BINDIR = bin
OBJDIR = obj
DEPS = assets/gfx/gfx.h
CFLAGS = -Wall -Wextra -Oz

.PHONY: all
all: gfx build

include $(shell cedev-config --makefile)
