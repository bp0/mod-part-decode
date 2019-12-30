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
#include <stdarg.h>
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>

#include "mod_part_decode.h"

static size_t bp_strlcat(char *dest, const char *src, size_t size) {
    size_t l = strlen(dest);
    size_t ls = strlen(src);
    strncpy(dest+l, src, size-l);
    return l+ls;
}
static size_t list_app(char *dest, const char *src, size_t size) {
    if (src && *src) {
        bp_strlcat(dest, src, size);
        return bp_strlcat(dest, " ", size);
    }
    return 0;
}

#define sizeof_member(type, member) sizeof(((type *)0)->member)
#define MPD_PROD_STR_MAX (sizeof_member(mem_mod_data, product))
#define MPD_STRS_MAX (sizeof_member(mem_mod_data, strs))
#define SEQ(s1, s2) (strcmp((s1), (s2)) == 0)
#define MIN(a,b) (((a)<(b))?(a):(b))
#define PROD_SET(p, str) strncpy((p)->product, (str) ? (str) : "", MPD_PROD_STR_MAX-1)
#define PROD_CAT(p, str) list_app((p)->product, (str) ? (str) : "", MPD_PROD_STR_MAX-1)

static const char *mem_mod_add_str(mem_mod_data *p, const char *str) {
    /* init */
    if (!p->next_str) p->next_str = p->strs;

    /* remainging len, copy len */
    size_t rl = MPD_STRS_MAX-(p->next_str-p->strs);
    size_t cl = MIN(strlen(str), rl-1);

    char *ret = p->next_str;
    if (cl > 0) {
        strncpy(p->next_str, str, cl);
        p->next_str += cl + 1;
    }
    return ret;
}
static const char *mem_mod_add_str_printf(mem_mod_data *p, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
static const char *mem_mod_add_str_printf(mem_mod_data *p, const char *fmt, ...)
{
    char buf[24] = "";
    int len;
    va_list args;
    va_start(args, fmt);
    len = vsnprintf(buf, 23, fmt, args);
    va_end(args);
    if (len < 0) return NULL;
    return mem_mod_add_str(p, buf);
}

typedef struct { char *code, *str; int nz_num; } mem_code_tab;
#define TAB_END {NULL}
static const char *tab_lookup(const mem_code_tab *tab, const char *code) {
    int i = 0;
    for(;tab[i].code;i++)
        if (SEQ(tab[i].code, code)) return tab[i].str;
    return NULL;
}
static int tab_lookup_num(const mem_code_tab *tab, const char *code) {
    int i = 0;
    for(;tab[i].code;i++)
        if (SEQ(tab[i].code, code)) return tab[i].nz_num;
    return 0;
}
#define FIELD_TAB(p, TAB, FI) p->field_meanings[FI] = tab_lookup(TAB, p->fields[FI]);

static int match_samsung(mem_mod_data *p) {
    int mc, ddrv = 0;

    mem_code_tab forms[] = {
        {"3", "DIMM"}, {"4", "SODIMM"}, TAB_END
    };
    mem_code_tab comp_type[] = {
        {"B", "DDR3"}, {"A", "DDR4"}, TAB_END
    };
    mem_code_tab mod_type_ddr3[] = {
        {"71", "x64 204pin Unbuffered SODIMM"},
        {"74", "x72 204pin ECC Unbuffered SODIMM"},
        {"78", "x64 240pin Unbuffered DIMM"},
        {"86", "x72 240pin LR DIMM"},
        {"90", "x72 240pin VLP Unbuffered DIMM"},
        {"91", "x72 240pin ECC Unbuffered DIMM"},
        {"92", "x72 240pin VLP Registered DIMM"},
        {"93", "x72 240pin Registered DIMM"},
        TAB_END
    };
    mem_code_tab speeds_ddr3[] = {
        {"F7", "DDR3-800 (400MHz@CL=6, tRCD=6, tRP=6)"},
        {"F8", "DDR3-1066 (533MHz@CL=7, tRCD=7, tRP=7)"},
        {"H9", "DDR3-1333 (667MHz@CL=9, tRCD=9, tRP=9)"},
        {"K0", "DDR3-1600 (800MHz@CL=11, tRCD=11, tRP=11)"},
        {"MA", "DDR3-1866 (933MHz@CL=13, tRCD=13, tRP=13)"},
        TAB_END
    };
    mem_code_tab reg_vendors[] = {
        {"0", "Inphi"},
        {"1", "Integrated Device Technology"},
        {"2", "Montage Technology"},
        {"3", "Inphi"},
        {"4", "Montage Technology"},
        TAB_END
    };
    mem_code_tab temp_power_ddr3[] = {
        /* num is millivolts */
        {"C", "Commercial (0-85C) 1.5V", 1500},
        {"Y", "Commercial (0-85C) Low VDD 1.35V", 1350},
    };
    mem_code_tab bit_org[] = {
        {"0", "x4", 4},
        {"3", "x8", 8},
        {"4", "x16", 16},
        TAB_END
    };
    mem_code_tab packages[] = {
        {"Z", "FBGA (Lead-free)" },
        {"H", "FBGA (Lead and halogen-free)" },
        {"J", "FBGA (Lead-free, DDP)" },
        {"M", "FBGA (Lead and halogen-free, DDP)" },
        {"B", "FBGA (Lead and halogen-free, flip-chip)" },
        {"E", "FBGA (Lead-free, QDP)" },
        {"O", "FBGA (Lead-free, QDP for 64GB LRDIMM)" },
        TAB_END
    };
    mem_code_tab depths[] = {
        {"32", "32M", 32}, {"33", "32M (for 128Mb/512Mb)", 32},
        {"64", "64M", 64}, {"65", "64M (for 128Mb/512Mb)", 64},
        {"28", "128M", 128}, {"29", "128M (for 128Mb/512Mb)", 128},
        {"56", "256M", 256}, {"57", "256M (for 512Mb/2Gb)", 256},
        {"51", "512M", 512}, {"52", "512M (for 512Mb/2Gb)", 512},
        {"1G", "1G", 1024}, {"1K", "1G (for 2Gb)", 1024},
        {"2G", "2G", 2048}, {"2K", "2G (for 2Gb)", 2048},
        {"4G", "4G", 4096}, {"8G", "8G", 8192},
        TAB_END
    };

    mc = sscanf(p->part_number,
        "M%1[34]"  // forms[]
        "%2[0-9]"  // mod_type[]
        "%1[BA]"   // comp_type[]
        "%2[0-9G]" // depths[]
        "%1[0-9]"    // # of bank in comp. interface -
        "%1[034]"  // bit_org[]
        "%1[A-Z]"  // component revision, "X-die" -
        "%1[A-Z]"  // packages[]
        "%2[0-9S]-" // pcb revision -
        "%1[CY]"   // temp_power[]
        "%2[0-9A-Z]" // speeds[]
        "%1[0-9]"  // reg_vendors[] (memory buffer)
                ,
        p->fields[0], p->fields[1], p->fields[2], p->fields[3], p->fields[4],
        p->fields[5], p->fields[6], p->fields[7], p->fields[8], p->fields[9],
        p->fields[10], p->fields[11] );
    if (mc >= 7) {
        p->hit = mc;
        p->want = 12;

        if (SEQ(p->fields[2], "A")) ddrv = 4;
        else if (SEQ(p->fields[2], "B")) ddrv = 3;

        FIELD_TAB(p, forms, 0);
        if (ddrv == 3) {
            FIELD_TAB(p, mod_type_ddr3, 1);
        }
        FIELD_TAB(p, comp_type, 2);
        FIELD_TAB(p, depths, 3);
        if (*p->fields[4]) {
            p->field_meanings[4] =
            mem_mod_add_str_printf(p, "bank=%s", p->fields[4]);
        }
        FIELD_TAB(p, bit_org, 5);
        if (*p->fields[6]) {
            p->field_meanings[6] =
            mem_mod_add_str_printf(p, "dieRev=%s", p->fields[6]);
        }
        FIELD_TAB(p, packages, 7);
        if (*p->fields[8]) {
            p->field_meanings[8] =
            mem_mod_add_str_printf(p, "pcbRev=%s", p->fields[8]);
        }
        if (ddrv == 3) {
            FIELD_TAB(p, temp_power_ddr3, 9);
            FIELD_TAB(p, speeds_ddr3, 10);
        }

        FIELD_TAB(p, reg_vendors, 11);

        p->vendor = "Samsung";
        p->reg_vendor = p->field_meanings[11];


        //NO :( -- p->capacity_MiB = tab_lookup_num(depths, p->fields[3]) * tab_lookup_num(bit_org, p->fields[5]);

        PROD_SET(p, ""); /* clear */
        PROD_CAT(p, p->field_meanings[2]);
        PROD_CAT(p, p->field_meanings[0]);
        PROD_CAT(p, p->field_meanings[10]);
    }
    return p->hit;
}

static int match_ct(mem_mod_data *p) {
    int mc;

    mem_code_tab ddr_type[] = {
        {"3", "DDR3"}, {"4", "DDR4"}, TAB_END
    };

    mc = sscanf(p->part_number, "CT%[0-9]G%1[34]",
        p->fields[0], p->fields[1]);
    if (mc == 2) {
        p->hit = mc;
        p->want = 10; //
        p->vendor = "Crucial";
        FIELD_TAB(p, ddr_type, 1);

        PROD_SET(p, ""); /* clear */
        PROD_CAT(p, p->field_meanings[1]);
    }
    return p->hit;
}

static int match_ksm(mem_mod_data *p) {
    int mc;

    mem_code_tab speeds[] = {
        {"24", "2400 MT/s"},
        {"26", "2666 MT/s"},
        {"29", "2933 MT/s"},
        {"32", "3200 MT/s"},
        TAB_END
    };
    mem_code_tab forms[] = {
        {"E", "Unbuffered DIMM (ECC)"},
        {"R", "Registered DIMM"},
        {"L", "Load Reduced DIMM"},
        {"SE", "Unbuffered SODIMM (ECC)"},
        TAB_END
    };
    mem_code_tab ranks[] = {
        {"S", "Single-rank", 1},
        {"D", "Dual-rank", 2},
        {"Q", "Quad-rank", 4},
        TAB_END
    };
    mem_code_tab dram_vendors[] = {
        {"H", "SK Hynix"}, {"M", "Micron Technology"}, TAB_END
    };
    mem_code_tab reg_vendors[] = {
        {"I", "Integrated Device Technology"},
        {"M", "Montage Technology"},
        {"R", "Rambus"},
        TAB_END
    };

    mc = sscanf(p->part_number,"KSM%2[0-9]%1[ERL]%1[SDQ]%1[48]%1[L]/%[0-9]%1[HM]%1[ABE]%1[IMR]%[0-9]",
        p->fields[0], p->fields[1], p->fields[2], p->fields[3], p->fields[4],
        p->fields[5], p->fields[6], p->fields[7], p->fields[8], p->fields[9] );
    if (mc < 6)
    mc = sscanf(p->part_number,"KSM%2[0-9]%2[SE]%1[SDQ]%1[48]%1[L]/%[0-9]%1[HM]%1[ABE]%1[IMR]%[0-9]",
        p->fields[0], p->fields[1], p->fields[2], p->fields[3], p->fields[4],
        p->fields[5], p->fields[6], p->fields[7], p->fields[8], p->fields[9] );
    if (mc >= 6) {
        FIELD_TAB(p, speeds, 0);
        FIELD_TAB(p, forms, 1);
        FIELD_TAB(p, ranks, 2);
        //3,4
        p->field_meanings[5] =
        mem_mod_add_str_printf(p, "%sGiB", p->fields[5]);
        FIELD_TAB(p, dram_vendors, 6);
        if (*p->fields[7]) {
            p->field_meanings[7] =
            mem_mod_add_str_printf(p, "dieRev=%s", p->fields[7]);
        }
        FIELD_TAB(p, reg_vendors, 8);

        p->vendor = "Kingston";
        PROD_SET(p, "Server Premier DDR4 ");

        p->ranks = tab_lookup_num(ranks, p->fields[2]);
        p->capacity_MiB = atoi(p->fields[5]) * 1024;
        p->dram_vendor = p->field_meanings[6];
        p->reg_vendor = p->field_meanings[8];

        p->hit = mc;
        p->want = 10;
    }

    return p->hit;
}

static int match_kvm(mem_mod_data *p) {

//KVR%2[0-9]%1[LU]%1[ENRLS]%2[0-9]%1[SDQ]%1[48]%1[LH]K%1[234]/%1[4]
//KVR%2[0-9]%1[ENRLS]%2[0-9]%1[SDQ]%1[48]%1[LH]K%1[234]/%1[4]
    return 0;
}

typedef int (*match_func)(mem_mod_data *p);
int mem_part_scan(mem_mod_data *p) {
    int fi = 0;

    match_func match_funcs[] = {
        match_ksm, match_kvm, match_ct,
        match_samsung,
        NULL
    };

    p->hit = 0;
    if (*p->part_number) {
        for(;match_funcs[fi];fi++) {
            if (match_funcs[fi](p)) break;
        }
    }

    return p->hit;
}

mem_mod_data *mem_part_new(const char *part_number) {
    mem_mod_data *p = memset(malloc(sizeof(mem_mod_data)), 0, sizeof(mem_mod_data));
    strncpy(p->part_number, part_number, 27);
    mem_part_scan(p);
    return p;
}

void mem_part_fee(mem_mod_data *p) {
    if (p) free(p);
}
