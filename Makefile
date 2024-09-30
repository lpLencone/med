SRC		:=  $(shell find src -name "*.c")
PKGS	:= sdl2 glew freetype2
LIBS	:= `pkg-config --libs $(PKGS)` -lm
CFLAGS	:= -Wall -Wextra -std=c2x -pedantic -ggdb `pkg-config --cflags $(PKGS)`
INCLUDE	:= -Iinclude

med: src/main.c
	$(CC) $(INCLUDE) $(CFLAGS) $(LIBS) -o med $(SRC)
