SRC		:=  $(shell find src -name "*.c")
PKGS	:= sdl2 glew
LIBS	:= `pkg-config --libs $(PKGS)` -lm
CFLAGS	:= -Wall -Wextra -std=c2x -ggdb `pkg-config --cflags $(PKGS)`
INCLUDE	:= -Iinclude

med: src/main.c
	$(CC) $(INCLUDE) $(CFLAGS) $(LIBS) -o med $(SRC)
