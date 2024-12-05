#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <conio.h>
#include <time.h>

#include <SFML/Graphics.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#include "log.h"

#include "ines.h"

#include "rp_2a03_cpu.h"
#include "rp_2c02.h"
#include "nes.h"

#include "rp_2a03_cpu.c"
#include "rp_2c02.c"
#include "nes.c"

#define NES_ASPECT_RATIO    (256.f / 240.f)

int main(int argc, char** argv)
{
    if (argc <= 1)
        return 0;   // No game rom given

    sfVideoMode mode = {800, 600, 32};
    sfRenderWindow* window = sfRenderWindow_create(mode, "NES Emulator", sfResize | sfClose, NULL);
    sfEvent event;

    sfRenderWindow_setActive(window, true);
    sfRenderWindow_setVerticalSyncEnabled(window, false);
    sfRenderWindow_setFramerateLimit(window, 60);

    if (!window)
        return 1;

    char* path_to_rom = argv[1];
    srand(time(0));

    NES nes = nes_create();
    nes_init(&nes);
    ppu_load_palette(&nes.ppu, "..\\palettes\\ntsc.pal");

    nes_load_game(&nes, path_to_rom);
    nes_power_up(&nes);

    bool running = false;
    uint32_t space_pressed = false;

    sfTexture* screen_texture = sfTexture_create(256, 240);
    sfRectangleShape* screen_rect = sfRectangleShape_create();

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

        if (sfKeyboard_isKeyPressed(sfKeySpace))
            space_pressed++;
        else
            space_pressed = 0;

        if (space_pressed == 1)
        {
            running ^= true;
            printf("Game status | running : %u\n", running);
        }

        if (running)
        {
            while (!nes.ppu.PPUSTATUS.vblank)
                nes_cycle(&nes);
            while (nes.ppu.PPUSTATUS.vblank)
                nes_cycle(&nes);
        }

        sfRenderWindow_clear(window, sfBlack);

        {
            sfVector2u window_size = sfRenderWindow_getSize(window);
            float screen_size = min(window_size.x / NES_ASPECT_RATIO, window_size.y);

            sfImage* screen_pixels = sfImage_createFromPixels(256, 240, (sfUint8*)&nes.ppu.screen);
            sfTexture_updateFromImage(screen_texture, screen_pixels, 0, 0);
            sfVector2f rect_size = {screen_size * NES_ASPECT_RATIO, screen_size};
            sfVector2f rect_origin = {rect_size.x / 2, rect_size.y / 2};
            sfVector2f rect_pos = {window_size.x / 2, window_size.y / 2};

            sfRectangleShape_setSize(screen_rect, rect_size);
            sfRectangleShape_setOrigin(screen_rect, rect_origin);
            sfRectangleShape_setPosition(screen_rect, rect_pos);
            sfRectangleShape_setTexture(screen_rect, screen_texture, false);

            sfRenderWindow_drawRectangleShape(window, screen_rect, NULL);

            sfImage_destroy(screen_pixels);
        }

        sfRenderWindow_display(window);
    }

    sfRenderWindow_destroy(window);
    sfTexture_destroy(screen_texture);

    nes_destroy(&nes);
}

#pragma GCC diagnostic pop