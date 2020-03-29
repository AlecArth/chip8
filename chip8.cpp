#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include "chip8.h"

unsigned char chip8_fontset[80] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// Constructor.
chip8::chip8(){

}

void chip8::initialise(){
    // Initialize registers and memory once.
    opcode = 0;
    I = 0;
    pc = 0x0200;
    sp = 0;
    
    // Clear display.
    for(auto i = 0; i < 2048; i++) {
        gfx[i] = 0;
    }
    
    // Clear stack and registers.
    for(auto i = 0; i < 16; i++) {
        stack[i] = 0;
        V[i] = 0;
    }   
    
    // Clear memory.
    for(auto i = 0; i < 4096; i++) {
        memory[i] = 0;
    }
    
    // Reset timers.
    delayTimer = 0;
    soundTimer = 0;
    
    // Load fontset
    for(auto i = 0; i < 80; i++) {
        memory[i] = chip8_fontset[i];
    }
    
    drawFlag = true;
    
    srand (time(NULL));
}

void chip8::loadGame(const char* file){
    
    // Open file.
    FILE *f = fopen(file, "rb");
	if(f == NULL){
		std::cout << "Error: could not open file."<< std::endl;
		exit(1);
	}

	// Get the file size.
	fseek(f, 0L, SEEK_END);
	int fsize = ftell(f);
	fseek(f, 0L, SEEK_SET);
    
    // Allocate buffer.
    char *buffer = (char*)malloc(sizeof(char)*fsize);
    
    // Copy into the buffer, and close file.
    fread(buffer, 1, fsize, f);
	fclose(f);
    
    // Copy buffer into chip8 memory.
    for(auto i = 0; i < fsize; i++){
        memory[i+512] = buffer[i];
    }
    
    // Free buffer.
    free(buffer);
}

void chip8::emulateCycle(){
    // Fetch Opcode
    opcode = memory[pc] << 8 | memory[pc+1];
    
    // Decode Opcode
    switch(opcode & 0xF000){
        // 00E0 and 00E0
        case 0x0000:
            switch(opcode & 0x000F){
                case 0x0000: // 0x00E0: Clears the screen        
                    for(auto i = 0; i < 2048; ++i)
                        gfx[i] = 0x0;
                    drawFlag = true;
                    pc += 2;
                break;

                case 0x000E: // 0x00EE: Returns from subroutine
                    sp--;
                    pc = stack[sp];
                    pc += 2;
                break;

                default:
                    printf ("Error: opcode not known: 0x%X\n", opcode);
            }
        break;
        
        // 1NNN
        case 0x1000: // 0x1NNN: Jumps to address NNN.
            pc = opcode & 0x0FFF;
        break;
        
        // 2NNN
        case 0x2000: // 0x2NNN: Calls subroutine at NNN.
            stack[sp] = pc;
            sp++;
            pc = opcode & 0x0FFF;
        break;
        
        // 3XNN
        case 0x3000: // 0x3XNN: Skips the next instruction if VX equals NN.
            if(V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)){
                pc += 4;
            }
            else{
                pc += 2;
            }
        break;
        
        // 4XNN
        case 0x4000: // 0x4XNN: Skips the next instruction if VX doesn't equal NN.
            if(V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)){
                pc += 4;
            }
            else{
                pc += 2;
            }
        break;
        
        // 5XY0
        case 0x5000: // 0x5XY0: Skips the next instruction if VX equals VY.
            if(V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]){
                pc += 4;
            }
            else{
                pc += 2;
            }
        break;
        
        // 6XNN
        case 0x6000: // 0x6XNN: Sets VX to NN.
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
        break;
        
        // 7XNN
        case 0x7000: // 0x7XNN: Adds NN to VX.
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
        break;
        
        // 8XY0, 8XY1, 8XY2, 8XY3, 8XY4, 8XY5, 8XY6, 8XY7, 8XYE
        case 0x8000:
            switch(opcode & 0x000F){
                case 0x0000: // 0x8XY0: Sets VX to the value of VY.
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0001: // 0x8XY1: Sets VX to VX or VY.
                    V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0002: // 0x8XY2: Sets VX to VX and VY.
                    V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0003: // 0x8XY3: Sets VX to VX xor VY.
                    V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0004: // 0x8XY4: Adds VY to VX.
                    if(V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8])){
                        V[0xF] = 1; // Carry.
                    }
                    else{
                        V[0xF] = 0;
                    }
                    V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0005: // 0x8XY5: VY is subtracted from VX.
                    if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]){
                        V[0xF] = 0; // Borrow.
                    }
                    else{
                        V[0xF] = 1;
                    }
                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0006: // 0x8XY6: Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
                    V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    pc += 2;
                break;

                case 0x0007: // 0x8XY7: Sets VX to VY minus VX.
                    if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x0F0) >> 4]){
                        V[0xF] = 0; // Borrow.
                    }
                    else{
                        V[0xF] = 1;
                    }
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                break;
                
                case 0x000E: // 0x8XYE: Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
                    V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    pc += 2;
                break;

                default:
                    printf ("Error: opcode not known: 0x%X\n", opcode);
            }
        break;
        
        // 9XY0
        case 0x9000: // 0x9XY0: Skips the next instruction if VX doesn't equal VY.
            if(V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]){
                pc += 4;
            }
            else{
                pc += 2;
            }
        break;
        
        // ANNN
        case 0xA000: // 0xANNN: Sets I to the address ANNN
            I = opcode & 0x0FFF;
            pc += 2;
        break;
        
        // BNNN
        case 0xB000: // 0xBNNN: Jumps to the address NNN plus V0.
            pc = V[0] + (opcode & 0x0FFF);
        break;
        
        // CXNN
        case 0xC000: // 0xCXNN: Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
            V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
            pc += 2;
        break;
        
        // DXYN
        case 0xD000: // 0xDXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
        {
			unsigned short x = V[(opcode & 0x0F00) >> 8];
			unsigned short y = V[(opcode & 0x00F0) >> 4];
			unsigned short height = opcode & 0x000F;
			unsigned short pixel;

			V[0xF] = 0;
			for (auto yline = 0; yline < height; yline++){
				pixel = memory[I + yline];
				for(auto xline = 0; xline < 8; xline++){
					if((pixel & (0x80 >> xline)) != 0){
						if(gfx[(x + xline + ((y + yline) * 64))] == 1){
							V[0xF] = 1;                                    
						}
						gfx[x + xline + ((y + yline) * 64)] ^= 1;
					}
				}
			}
						
			drawFlag = true;			
			pc += 2;
		}
        
        // EX9E, EXA1
        case 0xE000:
            switch(opcode & 0x00FF){
                case 0x009E: // 0xEX9E: Skips the next instruction if the key stored in VX is pressed.
                    if(key[V[(opcode & 0x0F00) >> 8]] != 0){
                        pc += 4;
                    }
                    else{
                        pc += 2;
                    }
                break;

                case 0x00A1: // 0x00A1: Skips the next instruction if the key stored in VX isn't pressed.
                    if(key[V[(opcode & 0x0F00) >> 8]] == 0){
                        pc += 4;
                    }
                    else{
                        pc += 2;
                    }
                break;

                default:
                    printf ("Error: opcode not known: 0x%X\n", opcode);
            }
        break;
        
        // FX07, FX0A, FX15, FX18, FX1E, FX29, FX33, FX55, FX65
        case 0xF000:
            switch(opcode & 0x00FF){
                case 0x0007: // 0xFX07: Sets VX to the value of the delay timer.
                    V[(opcode & 0x0F00) >> 8] = delayTimer;
					pc += 2;
                break;

                case 0x000A: // 0xFX0A: A key press is awaited, and then stored in VX.
                {
                    bool keyPress = false;
                    for(auto i = 0; i < 16; i++){
                        if(key[i] != 0){
                            V[(opcode & 0x0F00) >> 8] = i;
                            keyPress = true;
                        }
                    }
                    if(!keyPress){						
                        return;
                    }
                    pc += 2;
                }	
                break;

                case 0x0015: // 0xFX15: Sets the delay timer to VX.
                    delayTimer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                break;

                case 0x0018: // 0xFX18: Sets the sound timer to VX.
                    soundTimer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                break;

                case 0x001E: // 0xFX1E: Adds VX to I.
                    if((V[(opcode & 0x0F00) >> 8] + I) > 0xFFF){
                        V[0xF] = 1; // Overflow.
                    }
                    else{
                        V[0xF] = 0;
                    }
                    I += V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                break;

                case 0x0029: // 0xFX29: Sets I to the location of the sprite for the character in VX.
                    I = V[(opcode & 0x0F00) >> 8] * 0x5;
                    pc += 2;
                break;

                case 0x0033: // 0xFX33: 
                    memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;					
                    pc += 2;
                break;

                case 0x0055: // 0xFX55: Stores V0 to VX (including VX) in memory starting at address I.
                    for(auto i = 0; i < ((opcode & 0x0F00) >> 8); i++){
                        memory[I + i] = V[i];
                    }
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                break;

                case 0x0065: // 0xFX65: Fills V0 to VX (including VX) with values from memory starting at address I.
                    for(auto i = 0; i < ((opcode & 0x0F00) >> 8); i++){
                        V[i] = memory[I + i];
                    }
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                break;

                default:
                    printf ("Error: opcode not known: 0x%X\n", opcode);
            }
        break;
        
        default:
            printf ("Error: opcode not known: 0x%X\n", opcode);
    }
    
    // Update timers
    if(delayTimer > 0){
        delayTimer--;
    }
    if(soundTimer > 0){
        if(soundTimer == 1){
            std::cout << "A sound! (need to add sound)" << std::endl;
        }
        soundTimer--;
    }
}

// Deconstructor.
chip8::~chip8(){

}
