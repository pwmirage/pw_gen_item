/* SPDX-License-Identifier: MIT
 * Copyright(c) 2021 Darek Stojaczyk for pwmirage.com
 */

#include "common.h"

void
patch_mem(uintptr_t addr, const char *buf, unsigned num_bytes)
{
	void *page = (void *)(addr & ~(PAGE_SIZE - 1));
	unsigned mprot_bytes = (addr & (PAGE_SIZE - 1)) + num_bytes;
	char tmp[1024];
	size_t tmplen = 0;
	int i;

	for (i = 0; i < num_bytes; i++) {
		tmplen += snprintf(tmp + tmplen, MAX(0, sizeof(tmp) - tmplen), "0x%x ", (unsigned char)buf[i]);
	}
	fprintf(stderr, "patching %d bytes at 0x%x: %s\n", num_bytes, addr, tmp);

	mprotect(page, mprot_bytes, PROT_READ | PROT_WRITE);
	memcpy((void *)addr, buf, num_bytes);
	mprotect(page, mprot_bytes, PROT_READ | PROT_EXEC);
}

void
u32_to_str(char *buf, uint32_t u32)
{
	union {
		char c[4];
		uint32_t u;
	} u;

	u.u = u32;
	buf[0] = u.c[0];
	buf[1] = u.c[1];
	buf[2] = u.c[2];
	buf[3] = u.c[3];
}

void
detour(uintptr_t addr, uintptr_t fn)
{
	char buf[5];

	buf[0] = 0xe9;
	u32_to_str(&buf[1], fn - addr - 5);

	patch_mem(addr, buf, 5);
}

void
trampoline_fn(void **orig_fn, unsigned replaced_bytes, void *fn)
{
	uint32_t addr = (uintptr_t)*orig_fn;
	char orig_code[32];
	char buf[32];
	char *orig;

	memcpy(orig_code, (void *)addr, replaced_bytes);

	orig = calloc(1, (replaced_bytes + 5 + 0xFFF) & ~0xFFF);
	if (orig == NULL) {
		assert(false);
		return;
	}

	/* copy original code to a buffer */
	memcpy(orig, (void *)addr, replaced_bytes);
	/* follow it by a jump to the rest of original code */
	orig[replaced_bytes] = 0xe9;
	u32_to_str(orig + replaced_bytes + 1, (uint32_t)(uintptr_t)addr + replaced_bytes - (uintptr_t)orig - replaced_bytes - 5);

	/* patch the original code to do a jump */
	buf[0] = 0xe9;
	u32_to_str(buf + 1, (uint32_t)(uintptr_t)fn - addr - 5);
	memset(buf + 5, 0x90, replaced_bytes - 5);
	patch_mem(addr, buf, replaced_bytes);

	mprotect(orig, (replaced_bytes + 5 + 0xFFF) & ~0xFFF, PROT_READ | PROT_EXEC);

	*orig_fn = orig;
}
