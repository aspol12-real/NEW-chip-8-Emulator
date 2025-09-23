#define MINIAUDIO_IMPLEMENTATION

#include "miniaudio.h"
#include <fstream>
#include <cstdint>
#include <iostream>
#include <vector>
#include <raylib.h>
#include <cstdlib>
#include <iomanip>


//chip-8


const int lowWidth = 64;
const int lowHeight = 32;

const int hiWidth = 128;
const int hiHeight = 64;

const int screenSpaceX  = 1024;
const int screenSpaceY = screenSpaceX/2;

const int screenMarginSides = 300;
const int screenMarginBottom = 200;

const int screenWidth = screenSpaceX + screenMarginSides * 2;
const int screenHeight = screenSpaceY + screenMarginBottom;

const int hicellsizeX = screenSpaceX / hiWidth;
const int hicellsizeY = screenSpaceY / hiHeight;

const int locellsizeX = screenSpaceX / lowWidth;
const int locellsizeY = screenSpaceY / lowHeight;

const int DISPLAY_HZ = 60;

int CPU_HZ = 1000; 
int instructionsPerFrame = CPU_HZ / DISPLAY_HZ;

int spriteHeight;

class cpu {
    public:

        //memory
        uint8_t mem[4096] = {
        
            // regular characters 0-F

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
            0xF0, 0x80, 0xF0, 0x80, 0x80, // F
                                          
            //BIG HEX 0-9

            0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C, //0
            0x30, 0x70, 0xB0, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, //1
            0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, //2
            0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, //3
            0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, //4
            0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, //5
            0x3E, 0x7C, 0xE0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, //6
            0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, //7
            0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, //8
            0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C  //9

        };

        bool lowScreen[lowHeight][lowWidth];
        bool hiScreen[hiHeight][hiWidth];
        uint16_t stack[16];

        //regs
        uint8_t regs[16];
        uint16_t PC = 0x200;
        uint16_t I = 0;
        uint8_t delay = 0; //60 Hz
        uint8_t sound = 0;
        bool keys[16] = {false};
        uint8_t SP = 0; //stack pointer
        bool vblank = false;
        bool hires = false;
        int audioPitch = 1000; //1000hz
        bool debugOverlay = false;
        bool paused;
};


void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    float* pOutputF = (float*)pOutput;
    // Get the audio pitch from your chip8 object.
    cpu* chip8 = (cpu*)pDevice->pUserData;
    double phase_increment = 2.0 * MA_PI * (double)chip8->audioPitch / (double)48000;
    static double time = 0.0;

    for (ma_uint32 i = 0; i < frameCount; ++i) {
        float sample = sin(time) * 0.5f;
        *pOutputF++ = sample;
        time += phase_increment;
    }
    
    (void)pInput; // Avoid unused parameter warning
}


void cycle(cpu& chip8);
void cls(cpu& chip8);


int main( int argc, char *argv[] ) {
    cpu chip8; //init cpu object

    for (int i = 0; i < lowHeight; i++) {
        for (int j = 0; j < lowWidth; j++) {
            chip8.lowScreen[i][j] = false;
        }
    } //init screen buffer
    for (int i = 0; i < hiHeight; i++) {
        for (int j = 0; j < hiWidth; j++) {
            chip8.hiScreen[i][j]  = false;
        }
    }

    if (argc < 2) {
        std::cout << "USAGE: ./schip8 [filename].ch8 \n";
        exit( 1 );
    }

    
    for (int i = 0; i < 16; i++) {
        chip8.stack[i] = 0; //initialize stack to be empty
    }

    InitWindow(screenWidth, screenHeight, "SUPER CHIP8");
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

    ma_result result;
    ma_device_config deviceConfig;
    ma_device device;

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    deviceConfig.playback.channels = 1;
    deviceConfig.sampleRate        = 48000;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = &chip8; 

     result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize MiniAudio device." << std::endl;
        return -1;
    }

    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to start playback device." << std::endl;
        ma_device_uninit(&device);
        return -1;
    }


    
    while (!WindowShouldClose()) {  //main runtime

        instructionsPerFrame = CPU_HZ / DISPLAY_HZ;;

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
        if (IsKeyPressed(KEY_FIVE)) {
            chip8.paused = true;
        }
        if (IsKeyPressed(KEY_SIX)) {
            chip8.paused = false;
        }
        if (IsKeyPressed(KEY_SEVEN)) { //step
            cycle(chip8);
        }
        if (IsKeyDown(KEY_SPACE)) {
            CPU_HZ = 2000;
        } else {
            CPU_HZ = 1000; 
        }
        if (IsKeyPressed(KEY_EIGHT)) { //reset
            cls(chip8);
            for (int i = 0; i < 16; i++) {
                chip8.stack[i] = 0;
            }
            chip8.SP = 0;
            for (int i = 0; i < 16; i++) {
                chip8.regs[i] = 0;
            }
            chip8.PC = 512;
        }
        if (IsKeyPressed(KEY_TAB)) {
            chip8.debugOverlay = true;
        }
        if (IsKeyPressed(KEY_LEFT_SHIFT)) {
            chip8.debugOverlay = false;
        }

        if (chip8.delay > 0) {
        chip8.delay--;
        }
        if (chip8.sound > 0) {
            chip8.sound--;
        }

        if (!chip8.paused) {
            // cycle ipf (11) times per frame
            for (int i = 0; i < instructionsPerFrame; i++) {
                cycle(chip8);
                

                uint16_t lastOp = (chip8.mem[chip8.PC - 2] << 8) | chip8.mem[chip8.PC - 1];
                if ((lastOp >> 12) == 0xD) {
                    break;
                }


            } 
        }

        //draw screenbuffer every frame


        ClearBackground(BLACK);

        //draw screen border

        DrawRectangleLines(screenMarginSides, 0, screenSpaceX + 1, screenSpaceY + 1, WHITE);

        if (!chip8.hires) { //LOW RES MODE

            int yOffset = 0;
            for (int i = 0; i < lowHeight; i++) {
                int xOffset = screenMarginSides;
                for (int j = 0; j < lowWidth; j++) {
                    if (chip8.lowScreen[i][j] == true) {
                        DrawRectangle(xOffset, yOffset, locellsizeX, locellsizeY, WHITE);
                    } else {
                        DrawRectangle(xOffset, yOffset, locellsizeX, locellsizeY, BLACK);
                    }
                    xOffset += locellsizeX;
                }
                yOffset += locellsizeY;
            }
        } else {

            int yOffset = 0;
            for (int i = 0; i < hiHeight; i++) {
                int xOffset = screenMarginSides;
                for (int j = 0; j < hiWidth; j++) {
                    if (chip8.hiScreen[i][j] == true) {
                        DrawRectangle(xOffset, yOffset, hicellsizeX, hicellsizeY, WHITE);
                    } else {
                        DrawRectangle(xOffset, yOffset, hicellsizeX, hicellsizeY, BLACK);
                    }
                    xOffset += hicellsizeX;
                }
                 yOffset += hicellsizeY;
            }


        }
        
        if (chip8.sound > 0) {
            if (ma_device_get_state(&device) == ma_device_state_stopped) {
                ma_device_start(&device);
            }
        } else {
            if (ma_device_get_state(&device) == ma_device_state_started) {
                ma_device_stop(&device);
            }
        }
        if(chip8.debugOverlay) {
            DrawText("DEBUG", 0, 0, 20, RED);
            DrawText(TextFormat("CURRENT INSTRUCTION: %04X", (chip8.mem[chip8.PC] << 8) | chip8.mem[chip8.PC + 1]), 0, screenSpaceY + 50, 20, RED);
            DrawText(TextFormat("V0 = %d", chip8.regs[0]), 0, 50, 20, RED);
            DrawText(TextFormat("V1 = %d", chip8.regs[1]), 0, 70, 20, RED);
            DrawText(TextFormat("V2 = %d", chip8.regs[2]), 0, 90, 20, RED);
            DrawText(TextFormat("V3 = %d", chip8.regs[3]), 0, 110, 20, RED);
            DrawText(TextFormat("V4 = %d", chip8.regs[4]), 0, 130, 20, RED);
            DrawText(TextFormat("V5 = %d", chip8.regs[5]), 0, 150, 20, RED);
            DrawText(TextFormat("V6 = %d", chip8.regs[6]), 0, 170, 20, RED);
            DrawText(TextFormat("V7 = %d", chip8.regs[7]), 0, 190, 20, RED);
            DrawText(TextFormat("V8 = %d", chip8.regs[8]), 0, 210, 20, RED);
            DrawText(TextFormat("V9 = %d", chip8.regs[9]), 0, 230, 20, RED);
            DrawText(TextFormat("VA = %d", chip8.regs[10]), 0, 250, 20, RED);
            DrawText(TextFormat("VB = %d", chip8.regs[11]), 0, 270, 20, RED);
            DrawText(TextFormat("VC = %d", chip8.regs[12]), 0, 290, 20, RED);
            DrawText(TextFormat("VD = %d", chip8.regs[13]), 0, 310, 20, RED);
            DrawText(TextFormat("VE = %d", chip8.regs[14]), 0, 330, 20, RED);
            DrawText(TextFormat("VF = %d", chip8.regs[15]), 0, 350, 20, RED);
            DrawText(TextFormat("I = %d", chip8.I), (screenWidth - 100), 0, 20, RED);
            DrawText(TextFormat("EXEC HZ = %d", CPU_HZ), (screenWidth - 200), 30, 20, RED);
            DrawText(TextFormat("STACK = [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d]", chip8.stack[0], chip8.stack[1], chip8.stack[2], chip8.stack[3], chip8.stack[4], chip8.stack[5], chip8.stack[6], chip8.stack[7], chip8.stack[8], chip8.stack[9], chip8.stack[10], chip8.stack[12], chip8.stack[13], chip8.stack[14], chip8.stack[15]), (screenWidth - 700), screenSpaceY + 30, 20, RED);
            DrawText(TextFormat("DELAY = %d", chip8.delay), 0, 390, 20, RED);
            DrawText(TextFormat("SOUND = %d", chip8.sound), 0, 410, 20, RED);
            DrawText(TextFormat("SP = %d", chip8.SP), 0, 430, 20, RED);

            DrawText("Current Sprite:", 0, screenSpaceY + 80, 20, RED);
            if (chip8.I < 512) {
                spriteHeight = 10;
            } else {
                spriteHeight = 8;
            }

            int yOffset = screenSpaceY + 50;
            for (int i = 0; i < spriteHeight; i++) {
                uint8_t spriteByte = chip8.mem[chip8.I + i];
                int xOffset = screenMarginSides + 30;
                for (int bit = 0; bit < 8; bit++) {        
                        bool pixelOn = (spriteByte >> (7 - bit)) & 1;

                        if (pixelOn) {
                            DrawRectangle(xOffset, yOffset, hicellsizeX, hicellsizeY, WHITE);  
                        } else {
                            DrawRectangle(xOffset, yOffset, hicellsizeX, hicellsizeY, BLACK); 
                        }
                        xOffset += hicellsizeX;
                }
                yOffset += hicellsizeY;
            }

        
        } 

        if (chip8.paused) {
                DrawText("PAUSED", screenWidth/2, screenSpaceY + 100, 50, RED);
            }
        EndDrawing();

    }
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
        case 0: //00CN, 00E0, 00EE, 00FB, 00FC, 00FD, 00FC, 00FF

            if ((opcode & 0x00F0) == 0xC0) { //00CN scroll down N lines

                if (!chip8.hires) {
                    for (int i = lowHeight - 1; i >= N; i--) {
                        for (int j = 0; j < (lowWidth - 1); j++) {
                            chip8.lowScreen[i][j] = chip8.lowScreen[i - N][j];
                        } 
                    }

                    for (int i = 0; i < N; i++) {
                        for (int j = 0; j < lowWidth; j++) {
                            chip8.lowScreen[i][j] = false;
                        } 
                    }
                } else {
                    for (int i = hiHeight - 1; i >= N; i--) {
                        for (int j = 0; j < (hiWidth - 1); j++) {
                            chip8.hiScreen[i][j] = chip8.hiScreen[i - N][j];
                        }
                    }

                    for (int i = 0; i < N; i++) {
                        for (int j = 0; j < hiWidth; j++) {
                            chip8.hiScreen[i][j] = false;
                        }
                    }
                }
            }
            switch(NN) {

                case 0xFB: //scroll right four pixels

                
                    if (!chip8.hires) {
                            for (int i = 0; i < lowHeight; i++) {
                                for (int j = lowWidth - 1; j >= 4; j--) {
                                    chip8.lowScreen[i][j] = chip8.lowScreen[i][j + 4];  
                                }
                            }

                            //clean 4 rightmost columns
                            for (int i = 0; i < lowHeight; i++) {
                                for (int j = lowWidth - 4; j < lowWidth; j++) {
                                    chip8.lowScreen[i][j] = false;
                                }
                            }

                        } else {
                            for (int i = 0; i < hiHeight; i++) {
                                for (int j = hiWidth - 1; j >= 4; j--) {
                                    chip8.hiScreen[i][j] = chip8.hiScreen[i][j + 4];  
                                }
                            }

                            //clean 4 rightmost columns
                            for (int i = 0; i < hiHeight; i++) {
                                for (int j = hiWidth - 4; j < hiWidth; j++) {
                                    chip8.hiScreen[i][j] = false;
                                }
                            }

                        }
                    break;

                case 0xFC: //scroll left four pixels
                    if (!chip8.hires) {
                        for (int i = 0; i < lowHeight; i++) {
                            for (int j = 0 ; j < lowWidth - 4; j++) {
                                chip8.lowScreen[i][j] = chip8.lowScreen[i][j - 4];  
                            }
                        }

                        //clean 4 leftmost columns
                        for (int i = 0; i < lowHeight; i++) {
                            for (int j = 0; j < 3; j++) {
                                chip8.lowScreen[i][j] = false;
                            }
                        }

                    } else {
                        for (int i = 0; i < hiHeight; i++) {
                            for (int j = 0; j < hiWidth - 4; j++) {
                                chip8.hiScreen[i][j] = chip8.hiScreen[i][j - 4];  
                            }
                        }

                        //clean 4 leftmost columns
                        for (int i = 0; i < hiHeight; i++) {
                            for (int j = 0; j < 3; j++) {
                                chip8.hiScreen[i][j] = false;
                            }
                        }

                    }

                    break;
                    break;

                case 0xE0:
                    cls(chip8);
                    break;

                case 0xEE:

                    if (chip8.SP >= 0) {
                        chip8.stack[chip8.SP] = 0;
                        chip8.SP--;
                        chip8.PC = chip8.stack[chip8.SP];
                    } else {
                        std::cout << "STACK UNDERFLOW! \n";
                    }

                    break;

                case 0xFD: // exit interpreter
                    exit( 1 );
                    break;

                case 0xFE: //lores mode
                    chip8.hires = false;
                    break;

                case 0xFF: //hires mode
                    chip8.hires = true;
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

                    break;
                case 2:

                    chip8.regs[vX] &= chip8.regs[vY];

                    break;
                case 3:

                    chip8.regs[vX] ^= chip8.regs[vY];

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
        case 0xB: //BXNN
                  
            chip8.PC = NNN + chip8.regs[vX];

            break;
        case 0xC: //CXNN
                  
            chip8.regs[vX] = random & NN;

            break;
        case 0xD: //DXYN
            {

        

            if (N != 0) { // lores
                chip8.regs[15] = 0;

            

                for (int row = 0; row < N; row++) {
                    uint8_t spriteByte = chip8.mem[chip8.I + row];
                    
                    for (int bit = 0; bit < 8; bit++) {
                        
                        bool pixelOn = (spriteByte >> (7 - bit)) & 1;


                        if (pixelOn) {

                            if (!chip8.hires) {
                                int startX = x % 64; //wrap x and y first!
                                int startY = y % 32;
                                int scrX = (startX + bit);
                                int scrY = (startY + row);
                                
                                if (scrX >= 0 && scrX < 64 && scrY >= 0 && scrY < 32) { // if out of bounds, AFTER WRAPPING, don't draw

                                    if (chip8.lowScreen[scrY][scrX]) {
                                        chip8.regs[15] = 1; //collision detected
                                    }

                                    chip8.lowScreen[scrY][scrX] ^= true;
                                }   
                            } else {
                                int startX = x % 128; 
                                int startY = y % 64;
                                int scrX = (startX + bit);
                                int scrY = (startY + row);
                                
                                if (scrX >= 0 && scrX < 128 && scrY >= 0 && scrY < 64) { // if out of bounds, AFTER WRAPPING, don't draw

                                    if (chip8.hiScreen[scrY][scrX]) {
                                        chip8.regs[15] = 1; //collision detected
                                    }

                                    chip8.hiScreen[scrY][scrX] ^= true;
                                }  

                            }
                        }
                    }
                }
                
            } {
            } if (N == 0) {
                
                chip8.regs[15] = 0;

                int startX = x % 128; //wrap x and y first!
                int startY = y % 64;

                for (int row = 0; row < 16; row++) {
                    uint16_t spriteByte = (chip8.mem[chip8.I + row * 2] << 8) | chip8.mem[chip8.I + row * 2 + 1];
                    
                    for (int bit = 0; bit < 16; bit++) {
                        
                        bool pixelOn = (spriteByte >> (15 - bit)) & 1;


                        if (pixelOn) {

                            int scrX = (startX + bit);
                            int scrY = (startY + row);

                            if (scrX >= 0 && scrX < 128 && scrY >= 0 && scrY < 64) { // if out of bounds, AFTER WRAPPING, don't draw

                                if (chip8.hiScreen[scrY][scrX]) {
                                    chip8.regs[15] = 1; //collision detected
                                }

                                chip8.hiScreen[scrY][scrX] ^= true;
                            }   
                        }
                    }
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
        case 0xF: //FX07, FX0A, FX15, FX18, FX1E, FX29, FX30, FX33, FX55, FX65
                  
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

                case 0x29: //get 5 high font data

                    chip8.I = (uint16_t)chip8.regs[vX] * 5;

                    break;
                case 0x30: //get 10 high font data

                    chip8.I = 0x50 + (uint16_t)chip8.regs[vX] * 10;

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

void cls(cpu& chip8) {

    for (int i = 0; i < lowHeight; i++) { //clear low screen
        for (int j = 0; j < lowWidth; j++) {
            chip8.lowScreen[i][j] = false;
        }
    }
    for (int i = 0; i < hiHeight; i++) { //clear high screen
        for (int j = 0; j < hiWidth; j++) {
            chip8.hiScreen[i][j]  = false;
        }
    }
}
