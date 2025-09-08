/*
 * kernel.c - main kernel file (Assignment 1, CSE 597)
 * Copyright 2025 Ruslan Nikolaev <rnikola@psu.edu>
 */

#include <types.h>
#include <multiboot2.h>
#include <fb.h>
#include <apic.h>
#include <printf.h>

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

void kernel_start(struct multiboot_info *info, void *free_mem_base)
{
	fb_init(find_fb(info), 800, 600);

	while (1) {} /* Never return! */
}
