/* SPDX-License-Identifier: MIT
 * Copyright(c) 2021 Darek Stojaczyk for pwmirage.com
 */

#ifndef PW_GEN_ITEM_COMMON_H
#define PW_GEN_ITEM_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <assert.h>

#define PAGE_SIZE 4096
/* gcc5 doesn't know cdecl, weird */
#define __cdecl __attribute__((__cdecl__))

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

void patch_mem(uintptr_t addr, const char *buf, unsigned num_bytes);
void u32_to_str(char *buf, uint32_t u32);
void detour(uintptr_t addr, uintptr_t fn);
void trampoline_fn(void **orig_fn, unsigned replaced_bytes, void *fn);

#endif /* PW_GEN_ITEM_COMMON_H */
