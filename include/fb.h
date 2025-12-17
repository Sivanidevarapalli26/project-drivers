#pragma once

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

void fb_init(unsigned int *fb, unsigned int width, unsigned int height);
void fb_output(char ch);
void fb_draw_arrow_cursor(int x, int y);
void fb_erase_arrow_cursor(int x, int y);
void fb_set_text_cursor(int x, int y);
void fb_output_at_cursor(char ch);
void fb_mouse_status_update(int x, int y, uint8_t left, uint8_t right, uint8_t middle);


void fb_status_update(unsigned int task_id);

#ifdef __cplusplus
}
#endif
