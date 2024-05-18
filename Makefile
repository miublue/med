OUT = ./med
LIB = -lncurses
INC = -I ./src -I ../mutils/ -I .
SRC = ./src/*.c ../mutils/mstring.c ../mutils/mexec.c

all:
	tcc -o $(OUT) $(SRC) $(INC) $(LIB)

install: all
	mv $(OUT) ~/bin/
