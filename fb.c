/*
 * fb.c - a framebuffer console driver (Assignment 1, CSE 597)
 * Copyright 2025 Ruslan Nikolaev <rnikola@psu.edu>
 */

#include <fb.h>
#include <types.h>

extern unsigned char __ascii_font[2048];

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

#define FB_STATUS_BOX		56
#define FB_STATUS_MARGIN	10
#define FB_STATUS_WIDTH		18
#define FB_STATUS_HEIGHT	36

#define MOUSE_STATUS_X 500

static unsigned int *Fb;
static unsigned int Width, Height, PosX, PosY, MaxX, MaxY;

static unsigned int StatusCurr[2];
static unsigned int StatusStart[2];
static unsigned int StatusEnd[2];
static unsigned int StatusY;

static unsigned int TextCursorX = 0, TextCursorY = 0;
static int TextCursorVisible = 0;

static unsigned int ArrowSavedPixels[16 * 11];
static int ArrowLastX = -1, ArrowLastY = -1;

#define HELLO_STATEMENT \
    "Framebuffer Console (CSE 597)\nCopyright (C) 2024 Ruslan Nikolaev\n\n"

void fb_init(unsigned int *fb, unsigned int width, unsigned int height)
{
    size_t i, num = (size_t) width * height;
    const char *__hello_statement = HELLO_STATEMENT;

    for (i = 0; i < num; i++) {
        fb[i] = 0x00000000U;
    }

    Fb = fb;
    Width = width;
    Height = height;
    PosX = 0;
    PosY = 0;
    MaxX = width / FONT_WIDTH;
    MaxY = (height - FB_STATUS_BOX) / FONT_HEIGHT;

    for (i = 0; i < sizeof(HELLO_STATEMENT)-1; i++) {
        fb_output(__hello_statement[i]);
    }

    StatusY = height - FB_STATUS_BOX + FB_STATUS_MARGIN;
    StatusCurr[0] = FB_STATUS_MARGIN;
    StatusCurr[1] = width / 2 + FB_STATUS_MARGIN;
    StatusStart[0] = FB_STATUS_MARGIN;
    StatusStart[1] = width / 2 + FB_STATUS_MARGIN;
    StatusEnd[0] = width / 2 - FB_STATUS_MARGIN;
    StatusEnd[1] = width - FB_STATUS_MARGIN;
}

static void fb_draw_text_cursor_internal(int x, int y)
{
    size_t cur = (size_t)x + (size_t)y * Width;
    for (size_t j = 0; j < FONT_HEIGHT; j++) {
        Fb[cur] = 0xFF00FF00U;
        Fb[cur + 1] = 0xFF00FF00U;
        cur += Width;
    }
}

static void fb_erase_text_cursor_internal(int x, int y)
{
    size_t cur = (size_t)x + (size_t)y * Width;
    for (size_t j = 0; j < FONT_HEIGHT; j++) {
        Fb[cur] = 0x00000000U;
        Fb[cur + 1] = 0x00000000U;
        cur += Width;
    }
}

static int is_overlapping_text_cursor(int x, int y, int width, int height)
{
    if (!TextCursorVisible) return 0;
    
    int tcx = TextCursorX * FONT_WIDTH;
    int tcy = TextCursorY * FONT_HEIGHT;
    
    return !(x + width <= tcx || x >= tcx + 2 ||
             y + height <= tcy || y >= tcy + FONT_HEIGHT);
}

void fb_draw_arrow_cursor(int x, int y)
{
    static const uint16_t arrow_pattern[16] = {
        0b10000000000,
        0b11000000000,
        0b10100000000,
        0b10010000000,
        0b10001000000,
        0b10000100000,
        0b10000010000,
        0b10000001000,
        0b10000000100,
        0b10000000010,
        0b10000111110,
        0b10011100000,
        0b10100000000,
        0b11000000000,
        0b10000000000,
        0b00000000000
    };

    int overlaps = is_overlapping_text_cursor(x, y, 11, 16);
    if (overlaps && TextCursorVisible) {
        fb_erase_text_cursor_internal(TextCursorX * FONT_WIDTH, TextCursorY * FONT_HEIGHT);
    }

    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 11; col++) {
            int px = x + col;
            int py = y + row;
            if (px >= 0 && px < Width && py >= 0 && py < Height) {
                ArrowSavedPixels[row * 11 + col] = Fb[py * Width + px];
            }
        }
    }

    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 11; col++) {
            int px = x + col;
            int py = y + row;
            
            if (px >= 0 && px < Width && py >= 0 && py < Height) {
                if (arrow_pattern[row] & (1 << (10 - col))) {
                    Fb[py * Width + px] = 0xFFFFFFFFU;
                }
            }
        }
    }
    
    ArrowLastX = x;
    ArrowLastY = y;
}

void fb_erase_arrow_cursor(int x, int y)
{
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 11; col++) {
            int px = x + col;
            int py = y + row;
            if (px >= 0 && px < Width && py >= 0 && py < Height) {
                Fb[py * Width + px] = ArrowSavedPixels[row * 11 + col];
            }
        }
    }
    
    if (TextCursorVisible) {
        fb_draw_text_cursor_internal(TextCursorX * FONT_WIDTH, TextCursorY * FONT_HEIGHT);
    }
    
    ArrowLastX = -1;
    ArrowLastY = -1;
}

void fb_set_text_cursor(int x, int y)
{
    unsigned int char_x = x / FONT_WIDTH;
    unsigned int char_y = y / FONT_HEIGHT;
    
    if (char_x >= MaxX) char_x = MaxX - 1;
    if (char_y >= MaxY) char_y = MaxY - 1;
    
    if (TextCursorVisible) {
        fb_erase_text_cursor_internal(TextCursorX * FONT_WIDTH, TextCursorY * FONT_HEIGHT);
    }
    
    TextCursorX = char_x;
    TextCursorY = char_y;
    
    int overlaps = (ArrowLastX >= 0 && is_overlapping_text_cursor(ArrowLastX, ArrowLastY, 11, 16));
    if (!overlaps) {
        fb_draw_text_cursor_internal(TextCursorX * FONT_WIDTH, TextCursorY * FONT_HEIGHT);
    }
    
    TextCursorVisible = 1;
}

void fb_output_at_cursor(char ch)
{
    if (TextCursorVisible) {
        fb_erase_text_cursor_internal(TextCursorX * FONT_WIDTH, TextCursorY * FONT_HEIGHT);
        TextCursorVisible = 0;
    }
    
    PosX = TextCursorX;
    PosY = TextCursorY;
    
    fb_output(ch);
    
    TextCursorX = PosX;
    TextCursorY = PosY;
    
    int overlaps = (ArrowLastX >= 0 && is_overlapping_text_cursor(ArrowLastX, ArrowLastY, 11, 16));
    if (!overlaps) {
        fb_draw_text_cursor_internal(TextCursorX * FONT_WIDTH, TextCursorY * FONT_HEIGHT);
    }
    
    TextCursorVisible = 1;
}

static void fb_print_char_at(int x, int y, char ch, uint32_t color)
{
    unsigned char *ptr = &__ascii_font[(unsigned char)ch * (FONT_WIDTH * FONT_HEIGHT / 8)];
    size_t cur = (size_t)x + (size_t)y * Width;
    
    for (size_t j = 0; j < FONT_HEIGHT; j++) {
        signed char bitmap = ptr[j];
        for (size_t i = 0; i < FONT_WIDTH; i++) {
            if (bitmap & 0x80) {
                Fb[cur + i] = color;
            } else {
                Fb[cur + i] = 0x00000000U;
            }
            bitmap <<= 1;
        }
        cur += Width;
    }
}

static void fb_print_string_at(int x, int y, const char *str, uint32_t color)
{
    int cur_x = x;
    while (*str) {
        fb_print_char_at(cur_x, y, *str, color);
        cur_x += FONT_WIDTH;
        str++;
    }
}

static void fb_print_number_at(int x, int y, int num, uint32_t color)
{
    char buffer[12];
    int i = 0;
    int is_negative = 0;
    
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    if (num == 0) {
        buffer[i++] = '0';
    } else {
        while (num > 0) {
            buffer[i++] = '0' + (num % 10);
            num /= 10;
        }
    }
    
    if (is_negative) {
        buffer[i++] = '-';
    }
    
    buffer[i] = '\0';
    
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - 1 - j];
        buffer[i - 1 - j] = temp;
    }
    
    fb_print_string_at(x, y, buffer, color);
}

void fb_mouse_status_update(int x, int y, uint8_t left, uint8_t right, uint8_t middle)
{
    int status_y = Height - FB_STATUS_BOX + 5;
    uint32_t white = 0xFFFFFFFFU;
    
    for (int row = Height - FB_STATUS_BOX; row < Height; row++) {
        for (int col = MOUSE_STATUS_X; col < Width; col++) {
            Fb[row * Width + col] = 0x00000000U;
        }
    }
    
    fb_print_string_at(MOUSE_STATUS_X, status_y, "X:", white);
    fb_print_number_at(MOUSE_STATUS_X + FONT_WIDTH * 2, status_y, x, white);
    
    fb_print_string_at(MOUSE_STATUS_X + FONT_WIDTH * 6, status_y, "Y:", white);
    fb_print_number_at(MOUSE_STATUS_X + FONT_WIDTH * 8, status_y, y, white);
    
    status_y += FONT_HEIGHT + 2;
    
    if (left) {
        fb_print_string_at(MOUSE_STATUS_X, status_y, "Left Click", white);
    }
    
    if (right) {
        fb_print_string_at(MOUSE_STATUS_X, status_y + FONT_HEIGHT + 2, "Right Click", white);
    }
    
    if (middle) {
        fb_print_string_at(MOUSE_STATUS_X, status_y + (FONT_HEIGHT + 2) * 2, "Middle Click", white);
    }
}

void fb_status_update(unsigned int task_id)
{
    unsigned int curr, i, j;

    if (task_id >= 2)
        return;

    curr = StatusCurr[task_id];

    if (curr >= StatusStart[task_id] + FB_STATUS_WIDTH) {
        for (j = StatusY; j < StatusY + FB_STATUS_HEIGHT; j++) {
            for (i = curr - FB_STATUS_WIDTH; i < curr; i++) {
                Fb[j * Width + i] = 0x00000000U;
            }
        }
    }

    if (curr + FB_STATUS_WIDTH > StatusEnd[task_id])
        curr = StatusStart[task_id];

    for (j = StatusY; j < StatusY + FB_STATUS_HEIGHT; j++) {
        for (i = curr; i < curr + FB_STATUS_WIDTH; i++) {
            Fb[j * Width + i] = 0x88888888U;
        }
    }
    StatusCurr[task_id] = curr + FB_STATUS_WIDTH;
}

static void fb_scrollup(void)
{
    size_t cur = 0, count = Width * ((MaxY - 1) * FONT_HEIGHT);
    size_t row = Width * FONT_HEIGHT;
    do {
        Fb[cur] = Fb[cur+row];
        cur++;
    } while (--count != 0);

    do {
        Fb[cur] = 0x00000000U;
        cur++;
    } while (--row != 0);
}

void fb_output(char ch)
{
    size_t cur;
    unsigned char *ptr;
    if ((signed char) ch <= 0) {
        if (ch == 0) return;
        ch = '?';
    }
    if (ch == '\n' || PosX == MaxX) {
        PosX = 0;
        PosY++;
    }
    if (PosY == MaxY) {
        PosY--;
        fb_scrollup();
    }
    if (ch == '\n')
        return;
    ptr = &__ascii_font[(unsigned char) ch * (FONT_WIDTH * FONT_HEIGHT / 8)];
    cur = (size_t) PosX * FONT_WIDTH + (PosY * FONT_HEIGHT) * Width;
    for (size_t j = 0; j < FONT_HEIGHT; j++) {
        signed char bitmap = ptr[j];
        for (size_t i = 0; i < FONT_WIDTH; i++) {
            signed char color = (bitmap >> 7);
            Fb[cur + i] = (signed int) color;
            bitmap <<= 1;
        }
        cur += Width;
    }
    PosX++;
}