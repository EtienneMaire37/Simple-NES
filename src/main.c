#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <conio.h>
#include <time.h>

#include "rp_2a03.h"
#include "rp_2a03.c"

#include "ines.h"

#include "nes.h"
#include "nes.c"

int main(int argc, char** argv)
{
    if (argc <= 1)
        return 0;   // No game rom given

    char* path_to_rom = argv[1];
    srand(time(0));

    NES nes;
    nes_load_game(&nes, path_to_rom);
    nes_power_up(&nes);

    while (true)
    {
        getch();
        nes_cycle(&nes);
    }
}