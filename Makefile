OUT = ./med
LIB = -lncurses
INC = -I ./src -I ../mutils/

all:
	tcc -o $(OUT) ./src/*.c $(INC) $(LIB)

install: all
	mv $(OUT) ~/bin/
