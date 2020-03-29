
CC=g++
CFLAGS=-c -Wall
LIBRARIES=-lGL -lGLU -lglut -lGLEW

all: emulator

emulator: emulator.o chip8.o
	$(CC) chip8.o emulator.o  -o emu $(LIBRARIES)

emulator.o: emulator.cpp
	$(CC) $(CFLAGS) emulator.cpp

chip8.o: chip8.cpp
	$(CC) $(CFLAGS) chip8.cpp

clean:
	rm -rf *o emu
