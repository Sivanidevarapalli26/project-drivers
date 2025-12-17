#include <types.h>
#include <pic.h>
#include <printf.h>
#include <mouse.h>
#include <fb.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

static int mouse_x = SCREEN_WIDTH / 2;
static int mouse_y = SCREEN_HEIGHT / 2;
static int prev_mouse_x = SCREEN_WIDTH / 2;
static int prev_mouse_y = SCREEN_HEIGHT / 2;
static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

extern void x86_fillgate(int num, void *fun, int ist);
extern void mouse_isr(void);

static inline void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) { 
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

static inline void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4); 
    mouse_wait(1);
    outb(0x60, write);
}

static inline uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_handler_func() {
    uint8_t status = inb(0x64);
  
    if (!(status & 0x20)) {
        outb(0xA0, 0x20);
        outb(0x20, 0x20);
        return;
    }
    
    uint8_t data = inb(0x60);

    switch(mouse_cycle) {
        case 0:
            if ((data & 0x08) == 0) break; 
            mouse_byte[0] = data;
            mouse_cycle++;
            break;
        case 1:
            mouse_byte[1] = data;
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = data;
            mouse_cycle = 0;

            int8_t x_rel = (int8_t)mouse_byte[1];
            int8_t y_rel = (int8_t)mouse_byte[2];

            /* Save previous position */
            prev_mouse_x = mouse_x;
            prev_mouse_y = mouse_y;

            /* Update position */
            mouse_x += x_rel;
            mouse_y -= y_rel; 

            /* Clamp to screen bounds */
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x >= SCREEN_WIDTH) mouse_x = SCREEN_WIDTH - 1;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y >= SCREEN_HEIGHT) mouse_y = SCREEN_HEIGHT - 1;

            /* Check for left button click */
            uint8_t left_button = mouse_byte[0] & 0x01;
            uint8_t right_button = mouse_byte[0] & 0x02;
            uint8_t middle_button = mouse_byte[0] & 0x04;

            /* Erase old arrow cursor and draw new one */
            fb_erase_arrow_cursor(prev_mouse_x, prev_mouse_y);
            fb_draw_arrow_cursor(mouse_x, mouse_y);

            /* If left button clicked, set text cursor position */
            if (left_button) {
                fb_set_text_cursor(mouse_x, mouse_y);
            }

            /* Update status display */
            fb_mouse_status_update(mouse_x, mouse_y, left_button, right_button, middle_button);
            break;
    }
    
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void mouse_init() {
    uint8_t status;

    mouse_wait(1);
    outb(0x64, 0xA8);

    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = (inb(0x60) | 2); 
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    mouse_write(0xF6);
    mouse_read(); 

    mouse_write(0xF4);
    mouse_read(); 

    x86_fillgate(0x2C, mouse_isr, 0);
    
    pic_unmask_irq(12);
    pic_unmask_irq(2); 

    printf("Mouse initialized.\n");
}