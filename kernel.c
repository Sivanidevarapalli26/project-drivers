/*
 * kernel.c - main kernel file (Assignment 1, CSE 597)
 * Copyright 2025 Ruslan Nikolaev <rnikola@psu.edu>
 */

#include <types.h>
#include <multiboot2.h>
#include <fb.h>
#include <apic.h>
#include <printf.h>

#define PAGE_SIZE 4096
#define TOTAL_MEMORY 4294967296
#define TOTAL_PAGES  (TOTAL_MEMORY + PAGE_SIZE -1 )/ PAGE_SIZE
#define PT_PAGES (TOTAL_PAGES+ 512 -1) / 512
#define PD_PAGES (PT_PAGES  + 512 -1)/ 512
#define PDP_PAGES (PD_PAGES  + 512 -1)/ 512
#define PMLE4_PAGES (PDP_PAGES  + 512 -1)/ 512

/* Multiboot2 header */
struct multiboot_info {
	uint32_t total_size;
	uint32_t pad;
};

/* Locate the framebuffer address */
void *find_fb(struct multiboot_info *info)
{
	struct multiboot_tag *curr = (struct multiboot_tag *) (info + 1);
	while (curr->type != MULTIBOOT_TAG_TYPE_END) {
		if (curr->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
			struct multiboot_tag_framebuffer *fb =
				(struct multiboot_tag_framebuffer *) curr;
			if (fb->common.framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB
					&& fb->common.framebuffer_bpp == 32
					&& fb->common.framebuffer_width == 800
					&& fb->common.framebuffer_height == 600
					&& fb->common.framebuffer_pitch == 3200) {
				return (void *) fb->common.framebuffer_addr;
			}
		}
		curr = (struct multiboot_tag *) /* next tag, 8-byte aligned */
			(((uintptr_t) curr + curr->size + 7ULL) & ~7ULL);
	}
	return NULL;
}

void initialize_page_with_zeroes(uint8_t *ptr) {

	for(size_t i = 0; i<PAGE_SIZE; i++) {
		ptr[i] = 0;
	}

}

void* align_to_page(void *base) {
	return (void *)(((uint64_t) base + 4095ULL) & (~4095ULL));
}

void write_cr3(uint64_t cr3_value) {
	__asm__ volatile("mov %0, %%cr3"
						:
						: "r" (cr3_value)
						: "memory"
					);
}

void setup_pagetable(void *free_mem_base) {
	printf("Values of the memory initialization %d %d %d %d", PT_PAGES, PD_PAGES, PDP_PAGES, PMLE4_PAGES);

	printf("value of free base without alignment: %d\n", (uint64_t)free_mem_base);
	uint8_t* base = (uint8_t *) align_to_page(free_mem_base);

	printf("value of base with alignment: %d\n", (uint64_t)base);

	uint64_t *p = (uint64_t*)base;

	for (uint64_t i = 0; i < TOTAL_PAGES; i++) {
		p[i] = i * PAGE_SIZE;
		p [i] = p[i] | 0x1 | 0x2 ;
	}

	uint64_t *pd = (uint64_t*) (p + PT_PAGES * 512);
	printf("starting address pd pages: %d\n", (uint64_t)pd);

	for(uint64_t j = 0; j < PT_PAGES; j++) {
		pd[j] = (uint64_t) (p + 512 * j);
		pd[j] = pd[j] | 0x1 | 0x2;
	}

	uint64_t *pdp = (uint64_t *)(pd + PD_PAGES * 512);
	printf("starting address pdp pages: %d\n", (uint64_t)pdp);

	initialize_page_with_zeroes((uint8_t*)pdp);
	for(size_t k = 0; k < PD_PAGES; k++) {
		pdp[k] = (uint64_t)(pd + 512 * k);
		pdp[k] = pdp[k] | 0x1 | 0x2;
	}

	uint64_t* pmle4 = (uint64_t *)(pdp + PDP_PAGES * 512);
	printf("starting address pmle4 pages: %d\n", (uint64_t)pmle4);

	initialize_page_with_zeroes((uint8_t*)pmle4);
	for(size_t l = 0; l< PDP_PAGES; l++) {
		pmle4[l] = (uint64_t)(pdp + 512 * l);
		pmle4[l] = pmle4[l] | 0x1 | 0x2;
	}

	write_cr3((uint64_t)pmle4);


}

void kernel_start(struct multiboot_info *info, void *free_mem_base)
{
	fb_init(find_fb(info), 800, 600);
	setup_pagetable(free_mem_base);
	

	while (1) {} /* Never return! */
}
