#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>

#include "rp_2a03.h"
#include "rp_2a03.c"

int main()
{
    CPU cpu;
    cpu_power_up(&cpu);
    while (true)
    {
        getch();
        cpu_cycle(&cpu);
    }
}