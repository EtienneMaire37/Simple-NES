#pragma once

typedef enum NAMETABLE_MIRRORING
{
    MR_HORIZONTAL = 0,
    MR_VERTICAL = 1,
    MR_ONESCREEN_LOWER = 2,
    MR_ONESCREEN_HIGHER = 3,
    MR_ALTERNATIVE = 4
} NAMETABLE_MIRRORING;

typedef enum PALETTE_BG_SPRITE
{
    PL_BACKGROUND = 0,
    PL_SPRITE = 1
} PALETTE_BG_SPRITE;

typedef enum PATTERN_TABLE_SIDE
{
    PT_LEFT = 0,
    PT_RIGHT = 1
} PATTERN_TABLE_SIDE;

char* mirroring_text[] =
{
    "Horizontal",
    "Vertical",
    "Onescreen lower bank",
    "Onescreen higher bank",
    "Alternative"
};

struct PPU_STATUS
{
    uint8_t open_bus : 5;
    uint8_t sprite_overflow : 1;
    uint8_t sprite_0_hit : 1;
    uint8_t vblank : 1;
} __attribute__((packed));

struct PPU_SCROLL_ADDRESS
{
    uint8_t coarse_x : 5;
    uint8_t coarse_y : 5;
    uint8_t nametable_select : 2;
    uint8_t fine_y : 3;

    uint8_t padding : 1;
} __attribute__((packed));

struct SPRITE_ATTRIBUTES
{
    uint8_t palette : 2;
    uint8_t zero : 3;
    uint8_t priority : 1;   // (0: in front of background; 1: behind background)
    uint8_t flip_x : 1;
    uint8_t flip_y : 1;
} __attribute__((packed));

struct OAM_SPRITE_ENTRY
{
    uint8_t sprite_y;
    uint8_t tile_index;
    struct SPRITE_ATTRIBUTES attributes;
    uint8_t sprite_x;
} __attribute__((packed));

struct PPU_CONTROL
{
    uint8_t base_nametable_address : 2; // (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
    uint8_t address_increment : 1;      // (0 = add 1 (horizontal); 1 = add 32 (vertical))
    uint8_t sprite_pattern_table_address : 1;       // (0: $0000; 1: $1000; ignored in 8x16 mode)
    uint8_t background_pattern_table_address : 1;   // (0: $0000; 1: $1000)
    uint8_t sprite_size : 1;            // (0: 8x8 pixels; 1: 8x16 pixels)
    uint8_t ppu_master_slave : 1;       // (0: read backdrop from EXT pins; 1: output color on EXT pins)
    uint8_t nmi_enable : 1;
} __attribute__((packed));

typedef struct PPU_MASK
{
    uint8_t grayscale : 1;
    uint8_t show_bg_left : 1;   // Show bg in leftmost 8 pixels of the screen
    uint8_t show_sprites_left : 1;   // Same but with sprites
    uint8_t enable_bg : 1;
    uint8_t enable_sprites : 1;
    uint8_t emphasize_red : 1;
    uint8_t emphasize_green : 1;
    uint8_t emphasize_blue : 1;
} PPU_MASK;

typedef struct RP_2C02_PPU
{
    struct PPU_CONTROL PPUCTRL;    // $2000
    struct PPU_MASK PPUMASK;       // $2001
    struct PPU_STATUS PPUSTATUS;   // $2002
    uint8_t OAMADDR;        // $2003
    uint8_t OAMDATA;        // $2004
    // uint16_t PPUSCROLL;     // $2005
    // uint16_t PPUADDR;       // $2006
    uint8_t PPUDATA;        // $2007
    uint8_t OAMDMA;         // $4014

    NAMETABLE_MIRRORING mirroring;

    // https://www.nesdev.org/wiki/PPU_scrolling
    bool w;                 
    struct PPU_SCROLL_ADDRESS t;
    struct PPU_SCROLL_ADDRESS v;
    uint8_t x, fine_x;
    bool odd_frame;

    uint8_t last_read;

    uint8_t VRAM[0x1000];
    uint8_t palette_ram[32];
    uint8_t oam_memory[256];

    uint8_t ntsc_palette[192];

    uint16_t scanline;
    uint16_t cycle;

    struct OAM_SPRITE_ENTRY sprites_to_render[8];
    uint8_t num_sprites_to_render;
    uint8_t sprite_0_rendered;

    uint8_t screen[256 * 240 * 4];
    uint8_t screen_buffer[256 * 240 * 4];

    bool frame_finished;
    bool can_nmi;
    bool horizontal_increment, vertical_increment;

    NES* nes;
} PPU;

#define ppu_rendering_enabled(ppu_ptr)  ((ppu_ptr)->PPUMASK.enable_bg || (ppu_ptr)->PPUMASK.enable_sprites)
#define ppu_forced_blanking(ppu_ptr)    (((ppu_ptr)->PPUMASK.enable_bg == 0) && ((ppu_ptr)->PPUMASK.enable_sprites == 0))
#define ppu_vblank(ppu_ptr)             (((ppu_ptr)->cycle >= 1 && (ppu_ptr)->scanline == 241) || ((ppu_ptr)->scanline > 241 && (ppu_ptr)->scanline < 261) || ((ppu_ptr)->scanline == 261 && (ppu_ptr)->cycle == 0))

void ppu_reset(PPU* ppu);
void ppu_power_up(PPU* ppu);
uint8_t ppu_read_byte(PPU* ppu, uint16_t address);
void ppu_write_byte(PPU* ppu, uint16_t address, uint8_t byte);
void ppu_load_palette(PPU* ppu, char* path_to_palette);
uint8_t ppu_read_palette(PPU* ppu, PALETTE_BG_SPRITE background_sprite, uint8_t palette_number, uint8_t index);
uint8_t ppu_read_pattern_table_plane_0(PPU* ppu, PATTERN_TABLE_SIDE side, uint8_t tile, uint8_t off_y);
uint8_t ppu_read_pattern_table_plane_1(PPU* ppu, PATTERN_TABLE_SIDE side, uint8_t tile, uint8_t off_y);
uint8_t ppu_read_pattern_table(PPU* ppu, PATTERN_TABLE_SIDE side, uint8_t tile, uint8_t off_x, uint8_t off_y);
uint8_t ppu_read_nametable(PPU* ppu, uint8_t nametable, uint16_t bg_tile);
void ppu_cycle(PPU* ppu);