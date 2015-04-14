
# Copyright (c) 2015 Simo Bergman

CC=gcc
EXE=stm32ld
OBJ=main.o serial_posix.o stm32ld.o
CFLAGS= -Wall

all: $(EXE)

$(EXE):$(OBJ)

clean: 
	rm -f $(OBJ) $(EXE)
