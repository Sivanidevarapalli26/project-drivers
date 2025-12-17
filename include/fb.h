#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void fb_init(unsigned int *fb, unsigned int width, unsigned int height);
void fb_output(char ch);
void fb_draw_cursor(int x, int y);
void fb_set_cursor_position(int x, int y);
void fb_output_at_cursor(char ch);

void fb_status_update(unsigned int task_id);

#ifdef __cplusplus
}
#endif
