#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <conio.h>
#include <time.h>

#include "rp_2a03.h"
#include "rp_2a03.c"

#include "nes.h"
#include "nes.c"

int main()
{
    srand(time(0));

    NES nes;
    nes_power_up(&nes);
    while (true)
    {
        getch();
        nes_cycle(&nes);
    }
}