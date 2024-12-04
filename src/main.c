#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <conio.h>
#include <time.h>

#include <SFML/Graphics.h>

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

    sfVideoMode mode = {800, 600, 32};
    sfRenderWindow* window = sfRenderWindow_create(mode, "NES Emulator", sfResize | sfClose, NULL);
    sfEvent event;

    sfRenderWindow_setActive(window, true);

    if (!window)
        return 1;

    char* path_to_rom = argv[1];
    srand(time(0));

    NES nes = nes_create();
    nes_init(&nes);
    nes_load_game(&nes, path_to_rom);
    nes_power_up(&nes);

    bool running = false;

    while (sfRenderWindow_isOpen(window))
    {
        while (sfRenderWindow_pollEvent(window, &event))
        {
            if (event.type == sfEvtClosed)
                sfRenderWindow_close(window);
            if (event.type == sfEvtResized)
            {
                sfFloatRect viewrect = {0, 0, event.size.width, event.size.height};
                sfView* viewport = sfView_createFromRect(viewrect);
                sfRenderWindow_setView(window, viewport);
                sfView_destroy(viewport);
            }
        }

        if (running)
            nes_cycle(&nes);

        sfRenderWindow_clear(window, sfBlack);

        // {
        //     sfRectangleShape* rect = sfRectangleShape_create();
        //     sfVector2f rectsize = {500, 500};
        //     sfRectangleShape_setSize(rect, rectsize);
        //     sfRenderWindow_drawRectangleShape(window, rect, NULL);
        //     sfRectangleShape_destroy(rect);
        // }

        sfRenderWindow_display(window);
    }

    sfRenderWindow_destroy(window);

    nes_destroy(&nes);
}

#pragma GCC diagnostic pop