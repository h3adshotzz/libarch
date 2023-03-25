//===----------------------------------------------------------------------===//
//
//                       === Libarch Disassembler ===
//
//  This  document  is the property of "Is This On?" It is considered to be
//  confidential and proprietary and may not be, in any form, reproduced or
//  transmitted, in whole or in part, without express permission of Is This
//  On?.
//
//  Copyright (C) 2023, Harry Moulton - Is This On? Holdings Ltd
//
//  Harry Moulton <me@h3adsh0tzz.com>
//
//===----------------------------------------------------------------------===//

#ifndef __LIBARCH_UTILS_H__
#define __LIBARCH_UTILS_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/mman.h>

#define _SET_64(sz, regs, len) \
    do { \
        sz = 64; \
        regs = A64_REGISTERS_GP_64; \
        len = A64_REGISTERS_GP_64_LEN; \
    } while (0) \

#define _SET_32(sz, regs, len) \
    do { \
        sz = 32; \
        regs = A64_REGISTERS_GP_32; \
        len = A64_REGISTERS_GP_32_LEN; \
    } while (0) \



unsigned int
sign_extend (unsigned int bits, int numbits);
unsigned int 
highest_set_bit (unsigned int n, uint32_t imm);
unsigned int
zero_extend_ones (unsigned M, unsigned N);
unsigned int
ror_zero_extend_ones (unsigned M, unsigned N, unsigned R);
unsigned int
replicate (unsigned int val, unsigned bits);
unsigned int
decode_bitmasks (unsigned immN, unsigned imms, unsigned immr, int immediate, uint64_t *newval);
int 
move_wide_preferred (unsigned int sf, unsigned int immN, unsigned int immr, unsigned int imms);
unsigned long
ones (int len, int N);
int 
is_zero (unsigned N);
int
is_ones (unsigned N);
int
UInt (unsigned N);
int
BFXPreferred (unsigned sf, unsigned uns, unsigned imms, unsigned immr);

int 
SysOp (unsigned op1, unsigned CRn, unsigned CRm, unsigned op2);

int
get_tlbi (unsigned op1, unsigned CRn, unsigned CRm, unsigned op2);


unsigned int 
select_bits (unsigned int val, unsigned int start, unsigned int end);
unsigned int
sign_extend (unsigned int bits, int numbits);
void
base10 (unsigned int hex_value, int width);

int 
mstrappend (char **dst, const char *src, ...);

#endif /* __libarch_utils_h__ */