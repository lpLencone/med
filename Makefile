BIN		:= med
SRC		:= $(shell find src -name "*.c")
PKGS	:= glfw3 glew freetype2
LIBS	:= `pkg-config --libs $(PKGS)` -lm
CFLAGS	:= -Wall -Wextra -pedantic -ggdb `pkg-config --cflags $(PKGS)`
INCLUDE	:= -Iinclude

$(BIN): src/main.c
	$(CC) $(INCLUDE) $(CFLAGS) $(LIBS) -o $(BIN) $(SRC)
