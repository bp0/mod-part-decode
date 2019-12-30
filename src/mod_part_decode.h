/*
 * mod-part-decode - https://github.com/bp0/mod-part-decode
 * Copyright (C) 2019  Burt P. <pburt0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/*
 * Attempt to decode memory module part numbers.
 *
 * References:
 * https://forums.tomshardware.com/threads/meaning-of-ram-product-item-numbers-codes.3113155/
 * https://www.kingston.com/us/memory/memory-part-number-decoder
 * https://www.hyperxgaming.com/us/decoder
 *
 */


#ifndef _MOD_PART_DECODE_H_
#define _MOD_PART_DECODE_H_

#define MPD_FIELDS_MAX 12

typedef struct {
    char part_number[28]; /* copy */
    /* if !hit, any other data is unreliable */
    /* hit = fields matched, want = best possible for this hit */
    /* confidence might be calculated: hit/want */
    int hit, want;

    char strs[128], *next_str;
    char fields[MPD_FIELDS_MAX][3];
    const char *field_meanings[MPD_FIELDS_MAX]; /* const pointers or pointers into strs[] */

    const char *vendor;
    char product[100];

    const char *dram_vendor;
    const char *reg_vendor;
    unsigned long capacity_MiB;
    unsigned int ranks;

    //...

} mem_mod_data;

mem_mod_data *mem_part_new(const char *part_number);
void mem_part_fee(mem_mod_data *p);

int mem_part_scan(mem_mod_data *p);

#endif
