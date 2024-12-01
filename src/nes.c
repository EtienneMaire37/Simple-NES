#pragma once

void nes_reset(NES* nes)
{
    cpu_reset(&nes->cpu);
}

void nes_power_up(NES* nes)
{
    cpu_power_up(&nes->cpu);

    nes->cycle_alignment = rand() % 3;
}

void nes_cycle(NES* nes)
{
    nes->cycle_alignment++;
    nes->cycle_alignment %= 3;

    if (nes->cycle_alignment == 0)
    {
        cpu_cycle(&nes->cpu);
    }
}