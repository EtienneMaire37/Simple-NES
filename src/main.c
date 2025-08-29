#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// #define LOG_INSTRUCTIONS
// #define ENABLE_AUDIO

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
// #include <conio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#ifdef ENABLE_AUDIO
#include <windows.h>
#include <mmsystem.h>
#endif

#include <SFML/Graphics.h>

#ifndef ENABLE_AUDIO
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

// #define M_PI 3.14159265358979323846

float emulation_speed = 1;
bool emulation_running = false;
bool window_focus = false;

#include "log.h"
#include "ines.h"

typedef struct NES NES;

#include "rp_2a03_apu.h"
#include "rp_2a03_cpu.h"
#include "ppu.h"
#include "nes.h"

#include "rp_2a03_cpu.c"
#include "ppu.c"
#include "nes.c"
#include "rp_2a03_apu.c"

#define NES_ASPECT_RATIO    (256.f / 240.f)

char* palettes[5] = 
{"../../palettes/ntsc.pal", "../../palettes/cd.pal", "../../palettes/cd_fbx.pal",
    "../../palettes/nes_classic.pal", "../../palettes/yuv.pal"};
uint8_t palette_number = 2;

int main(int argc, char** argv)
{
    if (sizeof(struct PPU_SCROLL_ADDRESS) != 2 || sizeof(struct APU_STATUS) != 1) // ! - Compiler did not pack the bitfields correctly
    {
        printf("This was built with ms bitfield. Please build it with byte packed bitfields");
        while (true);
        return -1;
    }

    if (argc <= 1)
    {
        printf("Warning : No rom given!\n");
        return 0;   // No game rom given
    }

    char* path_to_rom = argv[1];

    NES nes = nes_create();
    nes_init(&nes);
    ppu_load_palette(&nes.ppu, palettes[palette_number]);

    nes_load_game(&nes, path_to_rom);
    nes_power_up(&nes);

    char title_buffer[128] = {0};

    sfVideoMode mode = {1024, 960, 32};
    sfRenderWindow* window = sfRenderWindow_create(mode, &title_buffer[0], sfResize | sfClose, NULL);
    sfEvent event;

    sfRenderWindow_setActive(window, true);
    #ifdef ENABLE_AUDIO
    sfRenderWindow_setVerticalSyncEnabled(window, true);
    #else
    sfRenderWindow_setFramerateLimit(window, 60.1f);
    #endif

    if (!window)
        return 1;
    srand(time(0));

    bool fullscreen_state = false;

    uint32_t space_pressed = 0, reset_pressed = 0, 
    palette_pressed = 0, system_pressed = 0, power_pressed = 0,
    fullscreen_pressed = 0;

    sfTexture* screen_texture = sfTexture_create(256, 240);
    sfRectangleShape* screen_rect = sfRectangleShape_create();
    sfImage* screen_pixels = sfImage_createFromPixels(256, 240, (sfUint8*)&nes.ppu.screen_buffer);
    sfRectangleShape_setTexture(screen_rect, screen_texture, false);

    sfClock* timer = sfClock_create();
    while (sfRenderWindow_isOpen(window))
    {
        double delta_time = sfClock_restart(timer).microseconds / 1000000.;
        float fps = (float)(1. / delta_time);

        snprintf(&title_buffer[0], 127, "Simple NES | %.1f FPS", fps);
        sfRenderWindow_setTitle(window, &title_buffer[0]);
        
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

        window_focus = sfRenderWindow_hasFocus(window);

        if (window_focus)
        {
            if (sfKeyboard_isKeyPressed(sfKeySpace))
                space_pressed++;
            else
                space_pressed = 0;

            if (sfKeyboard_isKeyPressed(sfKeyLControl))
            {
                if (sfKeyboard_isKeyPressed(sfKeyR))
                    reset_pressed++;
                else
                    reset_pressed = 0;
                    
                if (sfKeyboard_isKeyPressed(sfKeyP))
                    palette_pressed++;
                else
                    palette_pressed = 0;

                if (sfKeyboard_isKeyPressed(sfKeyT))
                    system_pressed++;
                else
                    system_pressed = 0;

                if (sfKeyboard_isKeyPressed(sfKeyU))
                    power_pressed++;
                else
                    power_pressed = 0;
            }
            else
            {
                reset_pressed = 0;
                palette_pressed = 0;
                system_pressed = 0;
                power_pressed = 0;
            }
        }
        else
        {
            reset_pressed = 0;
            palette_pressed = 0;
            space_pressed = 0;
            system_pressed = 0;
            power_pressed = 0;
        }

        if (sfKeyboard_isKeyPressed(sfKeyF11))
            fullscreen_pressed++;
        else
            fullscreen_pressed = 0;

        if (space_pressed == 1)
        {
            emulation_running ^= true;
            printf("Game status | emulation_running : %u\n", emulation_running);
        }

        if (system_pressed == 1)
        {
            nes.system ^= 1;
            printf("Switched to %s system\n", system_text[nes.system]);
        }

        if (reset_pressed == 1)
        {
            nes_reset(&nes);
            printf("NES reset\n");
        }

        if (power_pressed == 1 || system_pressed == 1)
        {
            nes_power_up(&nes);
            printf("NES powered up\n");
        }

        if (palette_pressed == 1)
        {
            palette_number++;
            palette_number %= 5;
            ppu_load_palette(&nes.ppu, palettes[palette_number]);
            // printf("Switched to palette \"%s\"\n", palettes[palette_number]);
        }

        if (fullscreen_pressed == 1)
        {
            fullscreen_state ^= true;
            sfRenderWindow_close(window);
            window = sfRenderWindow_create(mode, &title_buffer[0], fullscreen_state ? sfFullscreen : (sfClose | sfResize), NULL);
            sfRenderWindow_setActive(window, true);
            #ifdef ENABLE_AUDIO
            sfRenderWindow_setVerticalSyncEnabled(window, true);
            #else
            sfRenderWindow_setFramerateLimit(window, 60.1f);
            #endif
        }

        #ifndef ENABLE_AUDIO
        if (window_focus)
            nes_handle_controls(&nes);
        if (emulation_running)
        {
            // for (uint32_t i = 0; i < 341 * 262; i++)
            for (uint32_t i = 0; i < (nes.system == TV_NTSC ? NTSC_MASTER_FREQUENCY : PAL_MASTER_FREQUENCY) * emulation_speed / 60; i++)    // Assumes 60.1 FPS so not vertically synced
                nes_cycle(&nes);
        }
        #endif

        sfRenderWindow_clear(window, sfBlack);

        {
            sfVector2u window_size = sfRenderWindow_getSize(window);
            float screen_size = min(window_size.x / NES_ASPECT_RATIO, window_size.y);
            if (nes.ppu.frame_finished)
            {
                nes.ppu.frame_finished = false;
                memcpy((void*)sfImage_getPixelsPtr(screen_pixels), (void*)&nes.ppu.screen_buffer, 256 * 240 * 4);
            }
            sfTexture_updateFromImage(screen_texture, screen_pixels, 0, 0);
            sfVector2f rect_size = {screen_size * NES_ASPECT_RATIO, screen_size};
            sfVector2f rect_origin = {rect_size.x / 2, rect_size.y / 2};
            sfVector2f rect_pos = {window_size.x / 2, window_size.y / 2};

            sfRectangleShape_setSize(screen_rect, rect_size);
            sfRectangleShape_setOrigin(screen_rect, rect_origin);
            sfRectangleShape_setPosition(screen_rect, rect_pos);

            sfRenderWindow_drawRectangleShape(window, screen_rect, NULL);
        }

        sfRenderWindow_display(window);
    }

    nes_destroy(&nes);

    sfTexture_destroy(screen_texture);
    sfRectangleShape_destroy(screen_rect);
    sfImage_destroy(screen_pixels);
    sfClock_destroy(timer);

    sfRenderWindow_setActive(window, false);
    sfRenderWindow_destroy(window);
}

#pragma GCC diagnostic pop