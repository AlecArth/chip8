#ifndef CHIP8_H
#define CHIP8_H

class chip8{
    public:
        chip8();
        void initialise();
        void loadGame(const char* gameName);
        bool drawFlag;
        void emulateCycle();
        unsigned char gfx[64*32];
        unsigned char key[16];
        ~chip8();
    protected:
    private:
        unsigned short opcode; // Current opcode.
        unsigned char memory[4096]; // Ram where entire program and rom is stored.
        unsigned char V[16]; // General purpose registers, Vx where x is hex(O to F). F should not be used.
        unsigned short I; // Address register.
        unsigned short pc; // Program counter.
        unsigned char sp; // Stack pointer, stores top position of stack.
        unsigned short stack[16]; // Stores return addresses when calling subroutines.
        unsigned char delayTimer; // Decrement by 1 at 60hz while non zero.
        unsigned char soundTimer; // Decrement by 1 at 60hz while non zero.
};

#endif
