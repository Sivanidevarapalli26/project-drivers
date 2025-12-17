#include <types.h>
#include <pic.h>
#include <fb.h>
#include <printf.h>
#include <keyboard.h>

extern void x86_fillgate(int num, void *fun, int ist);
extern void keyboard_isr(void);

static const char scancode_set1[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 
    0,                                                    
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,                                                    
    '\\',
    'z','x','c','v','b','n','m',',','.','/',
    0,                                                    
    '*',
    0,                                                    
    ' ',                                                  
};

void keyboard_handler_func()
{
    uint8_t sc = inb(0x60);
    // printf("[KBD IRQ] scancode=%x\n", sc);
    if (sc & 0x80) {
        goto eoi;
    }
    if (sc < sizeof(scancode_set1)) {
        char ch = scancode_set1[sc];
        if (ch == '\n') {
            fb_output_at_cursor('\n');
        } else if (ch == '\b') {
            fb_output_at_cursor('^');
        } else if (ch != 0) {
            fb_output_at_cursor(ch);
        } else {
        }
    }

eoi:
    outb(0x20, 0x20);
}

void keyboard_init(void)
{
    x86_fillgate(0x21, keyboard_isr, 0);
    printf("Unmasking IRQ1...\n");
    pic_unmask_irq(1);
    printf("Keyboard driver initialized (IRQ1 -> vector 0x21)\n");
}
