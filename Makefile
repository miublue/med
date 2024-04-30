OUT = ./med
LIB = -lncurses
INC = -I ./src -I ../mutils/
SRC = ./src/*.c ../mutils/mstring.c

all:
	tcc -o $(OUT) $(SRC) $(INC) $(LIB)

install: all
	mv $(OUT) ~/bin/
