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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mod_part_decode.h"

const char *test[] = {
    // bp
    "M471A5244CB0-CTD",
    "CT8G4SFS8266.M8FE",
    "CT8G3S160BM.M16FP",
    // rando
    "M378B5773CH0-CH9", // Samsung DDR3 2G 1333 CL9
    "CT32M64S4W7E",
    "MT46V64M8TG-75",
    "M393B2G70EB0-CK0",
    // Kingston decoder examples
    "KSM26RD4L/32HAI2",
    "KSM32SES4L/16HAI2", // fake
    "KVR21LR15D8LK2/4HBI",
    "KVR16LR11D8LK2/4HB",
    "KVR1066D3LD8R7SLK2/46HB",
    "KVR400X72RC3AK2/1G",
    "HX429C15PB3AK2/32",
    // ADATA
    "AX4U300038G16-SB41",
    "AX4U2400W4G16-BBF",
    "AD4U2666J4G19-S",
    "AX4U3000316G16-QRD",
    // Crucial Ballistix
    "BLE4G4D30AEEA",
    "BLS2K16G4D32AESB",
    "BLT16G4D26BFT4",
    "BLT2K8G4D30BET4K",
    // G.Skill
    "F4-2133C15Q-16GRB",
    "F4-2400C15D-8GVR",
    "F4-3733C18Q-32GTZSW",
    "F4-2666C15Q-16GRR",
    NULL
};

#define sizeof_member(type, member) sizeof(((type *)0)->member)
#define MPD_PROD_STR_MAX (sizeof_member(mem_mod_data, product))
#define MPD_STRS_MAX (sizeof_member(mem_mod_data, strs))

int main() {
    printf("sizeof(mem_mod_data) = %d\n", (int)sizeof(mem_mod_data));
    int i = 0, f = 0;
    while(test[i]) {
        mem_mod_data *p = mem_part_new(test[i]);
        if (!p->hit) {
            printf("%s: unk\n", test[i]);
        } else {
            printf("%s: (fields:%d/%d) %s '%s'[%u/%u]\n", test[i], p->hit, p->want, p->vendor, p->product, (unsigned)strlen(p->product), (unsigned)MPD_PROD_STR_MAX);
            if (p->dram_vendor) printf("... dram_vendor: %s\n", p->dram_vendor);
            if (p->dram_vendor) printf("... reg_vendor: %s\n", p->reg_vendor);
            if (p->capacity_MiB) printf("... capacity_MiB: %lu\n", p->capacity_MiB);
            if (p->ranks) printf("... ranks: %d\n", p->ranks);
            if (p->next_str)
                printf("strs use: %u/%u; ", (unsigned)(p->next_str-p->strs), (unsigned)MPD_STRS_MAX);
            printf("... fields: ");
            for(f=0;f<p->hit;f++) {
                if (p->field_meanings[f])
                    printf("[%s] = '%s' ", p->fields[f], p->field_meanings[f]);
                else
                    printf("[%s] ", p->fields[f]);
            }
            printf("\n");
        }
        mem_part_fee(p);
        i++;
    }
    return 0;
}
