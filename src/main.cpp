#include <fstream>
#include <cstdint>
#include <iostream>
#include <vector>
#include <raylib.h>
#include <cstdlib>
#include <iomanip>

const int width = 64;
const int height = 32;
const int cellsize = 20;

const int screenWidth  = width * cellsize;
const int screenHeight = height * cellsize;

const int instructionsPerFrame = 11;

class cpu {
    public:

        //memory
        uint8_t mem[4096] = {
        
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

        bool screen[height][width];
        uint16_t stack[16];

        //regs
        uint8_t regs[16];
        uint16_t PC = 0x200;
        uint16_t I = 0;
        uint8_t delay = 0; //60 Hz
        uint8_t sound = 0;
        bool keys[16] = {false};
        uint8_t SP = 0; //stack pointer
};

void cycle(cpu& chip8);


int main( int argc, char *argv[] ) {

    cpu chip8; //init cpu object

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            chip8.screen[i][j] = false;
        }
    } //init screen buffer

    if (argc < 2 || argc > 2) {
        std::cout << "USAGE: ./chip8 [filename].ch8 \n";
        exit( 1 );
    }

    InitAudioDevice();
    Sound beep = LoadSound("sound/beep.wav");

    InitWindow(screenWidth, screenHeight, "CHIP8");
    SetTargetFPS(60);

    std::cout << "\n\n\n\n";

    std::string rom = argv[1];

    std::ifstream file;
    file.open(rom, std::ios::in | std::ios::binary);

    char byte;
    int index = 0;
    while (file.get(byte) && index < 4096 - 512) {
        unsigned char ubyte = static_cast<unsigned char>(byte);
        chip8.mem[index + 512] = ubyte;
        index++;
    }

    // ^ load bytes from rom into memory ^

    if (!file.is_open()) {
        std::cout << "Could not locate source file! \n";
        exit( 1 );
    } else {
        std::cout << "Loading rom file: " << rom << "\n";
    }

    

    std::cout << "\n";
    while (!WindowShouldClose()) {  //main runtime
       
        //handle inputs
        chip8.keys[0x1] = IsKeyDown(KEY_ONE);
        chip8.keys[0x2] = IsKeyDown(KEY_TWO);
        chip8.keys[0x3] = IsKeyDown(KEY_THREE);
        chip8.keys[0xC] = IsKeyDown(KEY_FOUR);
        chip8.keys[0x4] = IsKeyDown(KEY_Q);
        chip8.keys[0x5] = IsKeyDown(KEY_W);
        chip8.keys[0x6] = IsKeyDown(KEY_E);
        chip8.keys[0xD] = IsKeyDown(KEY_R);
        chip8.keys[0x7] = IsKeyDown(KEY_A);
        chip8.keys[0x8] = IsKeyDown(KEY_S);
        chip8.keys[0x9] = IsKeyDown(KEY_D);
        chip8.keys[0xE] = IsKeyDown(KEY_F);
        chip8.keys[0xA] = IsKeyDown(KEY_Z);
        chip8.keys[0x0] = IsKeyDown(KEY_X);
        chip8.keys[0xB] = IsKeyDown(KEY_C);
        chip8.keys[0xF] = IsKeyDown(KEY_V);

        if (chip8.delay > 0) {
        chip8.delay--;
        }
        if (chip8.sound > 0) {
            chip8.sound--;
        }

        // cycle ipf (11) times per frame
        for (int i = 0; i < instructionsPerFrame; i++) {
            cycle(chip8);
        } 


        ClearBackground(BLACK);

        //draw screenbuffer every frame

        int yOffset = 0;
        for (int i = 0; i < height; i++) {
            int xOffset = 0;
            for (int j = 0; j < width; j++) {
                if (chip8.screen[i][j] == true) {
                    DrawRectangle(xOffset, yOffset, cellsize, cellsize, WHITE);
                } else {
                    DrawRectangle(xOffset, yOffset, cellsize, cellsize, BLACK);
                }
                xOffset += cellsize;
            }
            yOffset += cellsize;
        }
      
        int op = (chip8.mem[chip8.PC] << 8 | chip8.mem[chip8.PC + 1]);

        DrawText(TextFormat("CURRENT INSTRUCTION: %i", op), 0, 0, 20, RED);
        if (chip8.sound > 0) {
            PlaySound(beep);
        }

        EndDrawing();

    }
    UnloadSound(beep);
    CloseAudioDevice();
    CloseWindow();

    return 1;

}


void cycle(cpu& chip8) {
    
    
    // fetch
    uint16_t opcode = (chip8.mem[chip8.PC] << 8) | chip8.mem[chip8.PC + 1];

    uint8_t inst =   opcode >> 12;
    uint8_t vX   =  (opcode & 0x0F00) >> 8;
    uint8_t vY   =  (opcode & 0x00F0) >> 4;
    uint8_t N    =   opcode & 0x000F;
    uint8_t NN   =   opcode & 0x00FF;
    uint16_t NNN =   opcode & 0x0FFF;

    //for DXYN exclusively
    uint8_t x = chip8.regs[vX];
    uint8_t y = chip8.regs[vY];

    uint8_t random = rand() % 0xFF; //gen random number
    uint8_t temp = chip8.regs[vX];

    uint8_t keyIndex = chip8.regs[vX];

    // increment
    chip8.PC += 2;

    //decode & execute
    switch(inst) {
        case 0: //00E0 and 00EE

            switch(N) {
                case 0:
                    for (int i = 0; i < height; i++) {
                        for (int j = 0; j < width; j++) {
                            chip8.screen[i][j] = false;
                        }
                    }
                    break;
                case 0xE:

                    if (chip8.SP >= 0) {
                        chip8.SP--;
                        chip8.PC = chip8.stack[chip8.SP];
                    } else {
                        std::cout << "STACK UNDERFLOW! \n";
                    }

                    break;
            }
            break;

        case 1: //1NNN
                
            chip8.PC = NNN;

            break;
        case 2: //2NNN
            
            if (chip8.SP < 16) {
                chip8.stack[chip8.SP] = chip8.PC;
                chip8.PC = NNN;
                chip8.SP++;
            } else {
                std::cout << "STACK OVERFLOW! \n";
            }

            break;
        case 3: //3XNN
                
            if (chip8.regs[vX] == NN) {
                chip8.PC += 2;
            }

            break;
        case 4: //4XNN
                
            if (chip8.regs[vX] != NN) {
                chip8.PC += 2;
            }

            break;
        case 5: //5XY0
                
            if (chip8.regs[vX] == chip8.regs[vY]) {
                chip8.PC += 2;
            }

            break;
        case 6: //6XNN
                
            chip8.regs[vX] = NN;

            break;
        case 7: //7XNN
                
            chip8.regs[vX] += NN;

            break;
        case 8: //8XY0, 8XY1, 8XY2, 8XY3, 8XY4, 8XY5, 8XY6, 8XY7, 8XYE

            switch(N) {
                case 0:

                    chip8.regs[vX] = chip8.regs[vY];

                    break;
                case 1:

                    chip8.regs[vX] |= chip8.regs[vY];
                    chip8.regs[15] = 0;

                    break;
                case 2:

                    chip8.regs[vX] &= chip8.regs[vY];
                    chip8.regs[15] = 0;

                    break;
                case 3:

                    chip8.regs[vX] ^= chip8.regs[vY];
                    chip8.regs[15] = 0;

                    break;
                case 4:
                    {
                    int result = chip8.regs[vX] + chip8.regs[vY];
                    chip8.regs[vX] = (result % 256);

                    if (result > 255) {
                        chip8.regs[15] = 1;
                    } else {
                        chip8.regs[15] = 0;
                    }

                    }
                    break;
                case 5:
                    chip8.regs[vX] -= chip8.regs[vY];

                    if (temp >= chip8.regs[vY]) {
                        chip8.regs[15] = 1;
                    } else {
                        chip8.regs[15] = 0;
                    }

                    break;
                case 6:
                    
                    chip8.regs[vX] = chip8.regs[vY];
                    chip8.regs[vX] >>= 1;
                    chip8.regs[15] = temp & 0b00000001;

                    break;
                case 7:

                    chip8.regs[vX] = chip8.regs[vY] - chip8.regs[vX];
                    if (chip8.regs[vY] >= temp) {
                        chip8.regs[15] = 1;
                    } else {
                        chip8.regs[15] = 0;
                    }

                    break;
                case 0xE:

                    chip8.regs[vX] = chip8.regs[vY];
                    chip8.regs[vX] <<= 1;
                    chip8.regs[15] = temp >> 7;

                    break;

            }
            break;
        case 9: //9XY0
                
            if (chip8.regs[vX] != chip8.regs[vY]) {
                chip8.PC += 2;
            }

            break;
        case 0xA: //ANNN
                  
            chip8.I = NNN;
                  
            break;
        case 0xB: //BNNN
                  
            chip8.PC = NNN + chip8.regs[0];

            break;
        case 0xC: //CXNN
                  
            chip8.regs[vX] = random & NN;

            break;
        case 0xD: //DXYN       

            chip8.regs[15] = 0;
            for (int row = 0; row < N; row++) {
                uint8_t spriteByte = chip8.mem[chip8.I + row];
                if (x > 63) { 
                    x = x % 64;
                }
                if (y > 31) {
                    y = y % 32;
                }

                for (int bit = 0; bit < 8; bit++) {
                    
                    bool pixelOn = (spriteByte >> (7 - bit)) & 1;
                    int scrX = (x + bit) % 64;
                    int scrY = y + row;

                    if (pixelOn) {

                        if (chip8.screen[scrY][scrX]) {
                            chip8.regs[15] = 1; //collision detected
                        }
                        chip8.screen[scrY][scrX] ^= true;
                    }

                }
            }

            break;

        case 0xE: //EX9E, EXA1
                  

            if (NN == 0x9E) {
                if (keyIndex < 16 && chip8.keys[keyIndex]) {
                    chip8.PC += 2; //skip next opcode if key == regs[vX]
                }
            } else if (NN == 0xA1) {
                if (keyIndex < 16 && !chip8.keys[keyIndex]) {
                    chip8.PC += 2; //skip next opcode if key != regs[vX]
                }
            }
                  

            break;
        case 0xF: //FX07, FX0A, FX15, FX18, FX1E, FX29, FX33, FX55, FX65
                  
            switch(NN) {

                case 0x07:

                    chip8.regs[vX] = chip8.delay;

                    break;

                case 0x0A:

                    chip8.PC -= 2; 

                    for (int i = 0; i < 16; ++i) {
                        if (chip8.keys[i]) {
                            chip8.regs[vX] = i;
                            chip8.PC += 2; 
                            break;
                        }
                    }
                    break;
                    
                case 0x15:

                    chip8.delay = chip8.regs[vX];

                    break;

                case 0x18:

                    chip8.sound = chip8.regs[vX];

                    break;

                case 0x1E:

                    chip8.I += chip8.regs[vX];

                    break;

                case 0x29:

                    chip8.I = (uint16_t)chip8.regs[vX] * 5;

                    break;

                case 0x33:

                    chip8.mem[chip8.I] = chip8.regs[vX] / 100;
                    chip8.mem[chip8.I + 1] = (chip8.regs[vX] / 10) % 10;
                    chip8.mem[chip8.I + 2] = chip8.regs[vX] % 10;

                    break;

                case 0x55: //save

                    for (int i = 0; i <= vX; ++i) {
                        chip8.mem[chip8.I + i] = chip8.regs[i];
                    }

                    break;

                case 0x65: //load

                    for (int i = 0; i <= vX; ++i) {
                        chip8.regs[i] = chip8.mem[chip8.I + i];
                    }

                    break;



            }

            break;
    }
}

