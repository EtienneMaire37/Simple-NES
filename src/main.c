#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <conio.h>
#include <time.h>

// #define LOG_INSTRUCTIONS

#include "log.h"

#include "ines.h"

#include "rp_2a03_cpu.h"
#include "rp_2c02.h"
#include "nes.h"

#include "rp_2a03_cpu.c"
#include "rp_2c02.c"
#include "nes.c"

int main(int argc, char** argv)
{
    if (argc <= 1)
        return 0;   // No game rom given

    char* path_to_rom = argv[1];
    srand(time(0));

    NES nes = nes_create();
    nes_init(&nes);
    nes_load_game(&nes, path_to_rom);
    nes_power_up(&nes);

    // nes.cpu.PC = 0xc000; // ! - Needed for nestest in log mode

    bool running = false;

    while (true)
    {
        bool key_pressed = false;
        bool space_pressed = false;
        char k;
        if (kbhit())
        {
            k = getch();
            key_pressed = true;
        }
        if (k == 32 && key_pressed)
        {
            running ^= true;
            space_pressed = true;
        }
        if (key_pressed)
        {
            while (nes.cpu.cycle > 0)
                nes_cycle(&nes);
            nes_cycle(&nes);
            nes_cycle(&nes);
            nes_cycle(&nes);
        }
        if (running)
            nes_cycle(&nes);
    }

    nes_destroy(&nes);
}