#include <types.h>
#include <kernel.h>
#include <printf.h>

struct gate_descriptor {
	unsigned long gd_looffset:16;	/* gate offset (lsb) */
	unsigned long gd_selector:16;	/* gate segment selector */
	unsigned long gd_ist:3;		/* IST select */
	unsigned long gd_xx1:5;		/* reserved */
	unsigned long gd_type:5;	/* segment type */
	unsigned long gd_dpl:2;		/* segment descriptor priority level */
	unsigned long gd_p:1;		/* segment descriptor present */
	unsigned long gd_hioffset:48;	/* gate offset (msb) */
	unsigned long gd_xx2:8;		/* reserved */
	unsigned long gd_zero:5;	/* must be zero */
	unsigned long gd_xx3:19;	/* reserved */
} __attribute__((__packed__));

struct gate_descriptor idt[256] __attribute__((aligned(16)));

void
x86_fillgate(int num, void *fun, int ist)
{
	struct gate_descriptor *gd = &idt[num];

	gd->gd_hioffset = (unsigned long)fun >> 16;
	gd->gd_looffset = (unsigned long)fun & 0xffff;

	gd->gd_selector = 0x10;
	gd->gd_ist = ist;
	gd->gd_type = 14;
	gd->gd_dpl = 0;
	gd->gd_p = 1;

	gd->gd_zero = 0;
	gd->gd_xx1 = 0;
	gd->gd_xx2 = 0;
	gd->gd_xx3 = 0;
}

void handle_exception() {
    printf("CPU Exception occured");
}

extern void default_trap();

void init_idt() {
    for(int i = 0; i< 256;i++) {
        x86_fillgate(i, default_trap, 0);
    }

    struct idt_pointer idtp;
    idtp.limit = (uint16_t)(sizeof(idt)-1);
    idtp.base = (uint64_t)(uintptr_t)(void *)idt;
    load_idt(&idtp);
    printf("IDT Loaded \n"); 
    
}