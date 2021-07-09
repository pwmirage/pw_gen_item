/* SPDX-License-Identifier: MIT
 * Copyright(c) 2021 Darek Stojaczyk for pwmirage.com
 */

#ifndef PW_GEN_ITEM_DATA_H
#define PW_GEN_ITEM_DATA_H

#include "common.h"

struct pw_item {
	uint32_t type;
	uint32_t count;
	uint32_t pile_limit;
	uint32_t equip_mask;
	uint32_t proc_type;
	uint32_t classid;
	struct
	{
		uint32_t guid1;
		uint32_t guid2;
	} guid;
	uint32_t price;
	uint32_t expire_date;
	uint32_t content_length;
	char *item_content;
};

struct __attribute__((packed)) pw_item_tag_t {
	char type;
	char size;
};

static void * __cdecl (*pw_get_data_man)(void) = (void *)0x8088476;
static struct pw_item * (*pw_generate_item_from_player)(void *this, uint param_1,void *param_2,uint param_3) = (void *)0x81ed8f0;
static void * __cdecl (*pw_free_item)(struct pw_item *item) = (void *)0x80899da;

#endif /* PW_GEN_ITEM_DATA_H */
