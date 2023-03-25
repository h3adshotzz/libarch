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

#include <stdio.h>
#include "utils.h"

#include "arm64/arm64-misc.h"
#include "arm64/arm64-tlbi-ops.h"

/**
 *  AArch64 Reference Manual shared/functions/common/
 * 
 *  Some of these are taken from 
 *      https://github.com/xerub/macho/blob/master/patchfinder64.c
 */

unsigned int 
select_bits (unsigned int val, unsigned int start, unsigned int end)
{
    unsigned int size = (end - start) + 1;
    unsigned int mask = ((1 << size) - 1) << start;
    return (val & mask) >> start;
}

unsigned int
sign_extend (unsigned int bits, int numbits)
{
    if(bits & (1 << (numbits - 1)))
        return bits | ~((1 << numbits) - 1);
    return bits;
}

unsigned int 
highest_set_bit (unsigned int n, uint32_t imm)
{
    for (int i = n - 1; i >= 0; i--)
        if (imm & (1 << i)) return i;
    return -1;
}

unsigned int
zero_extend_ones (unsigned M, unsigned N)
{
    (void) N;
    return ((uint64_t) 1 << M) - 1;
}

unsigned int
ror_zero_extend_ones (unsigned M, unsigned N, unsigned R)
{
    uint64_t val = zero_extend_ones (M, N);
    if (R == 0) return val;
    return ((val >> R) & (((uint64_t)1 << (N - R)) - 1)) | ((val & (((uint64_t)1 << R) - 1)) << (N - R));
}

unsigned int
replicate (unsigned int val, unsigned bits)
{
    unsigned int ret = val;
    for (unsigned shift = bits; shift < 64; shift += bits)
        ret |= (val << shift);
    return ret;
}

unsigned int
decode_bitmasks (unsigned immN, unsigned imms, unsigned immr, int immediate, uint64_t *newval)
{
    unsigned levels, S, R, esize;
	int len = highest_set_bit (7, (immN << 6) | (~imms & 0x3F));
	if (len < 1)
		return -1;

	levels = zero_extend_ones (len, 6);
	if (immediate && (imms & levels) == levels)
		return -1;

	S = imms & levels;
	R = immr & levels;
	esize = 1 << len;
	*newval = replicate (ror_zero_extend_ones (S + 1, esize, R), esize);
	return 0;
}

int 
move_wide_preferred (unsigned int sf, unsigned int immN, unsigned int immr, unsigned int imms)
{
    int width = sf == 1 ? 64 : 32;
    unsigned int combined = (immN << 6) | imms;

    if(sf == 1 && (combined >> 6) != 1)
        return 0;

    if(sf == 0 && (combined >> 5) != 0)
        return 0;

    if(imms < 16)
        return (-immr % 16) <= (15 - imms);

    if(imms >= (width - 15))
        return (immr % 16) <= (imms - (width - 15));

    return 0;
}

unsigned long
ones (int len, int N)
{
    (void) N;
    unsigned long ret = 0;

    for (int i = len-1; i >= 0; i--)
        ret |= ((unsigned long) 1 << i);
    return ret;
}

int 
is_zero (unsigned N)
{
    return (N == 0);
}

int
is_ones (unsigned N)
{
    return (N == ones (N, 0));
}

int
UInt (unsigned N)
{
    int result = 0;
    for (int i = 0; i < N-1; i++)
        if (((N & (1<<1)) >> 1) == 1) result = result + 2^i;
    return result;
}

int
BFXPreferred (unsigned sf, unsigned uns, unsigned imms, unsigned immr)
{
    // must not match UBFIZ/SBFIX alias
    if (imms < immr) return 0;

    // must not match LSR/ASR/LSL alias
    if (imms == select_bits (sf, 0, 31)) return 0;

    // must not match UXTx/SXTx alias
    if (immr == 0) {
        // must not match 32-bit UXT[BH] or SXT[BH]
        if (sf == 0 && (imms == 0b000111 || imms == 0b001111))
            return 0;
        // must not match 64-bit SXT[BHW]
        if ((select_bits (sf, 0, uns) == 0b10) && (imms == 0b000111 || imms == 0b001111 || imms == 0b011111))
            return 0;
    }

    // must be UBFX/SBFX alias
    return 1;
}

int 
SysOp (unsigned op1, unsigned CRn, unsigned CRm, unsigned op2)
{
    unsigned encoding = (op1 << 11);
    encoding |= (CRn << 7);
    encoding |= (CRm << 3);
    encoding |= op2;

    switch(encoding){
        /* SysOp AT */
        case 0x3c0:
        case 0x23c0:
        case 0x33c0:
        case 0x3c1:
        case 0x23c1:
        case 0x33c1:
        case 0x3c2:
        case 0x3c3:
        case 0x23c4:
        case 0x23c5:
        case 0x23c6:
        case 0x23c7:
            return ARM64_SYSOP_AT;

        /* SysOP DC */
        case 0x1ba1:
        case 0x3b1:
        case 0x3b2:
        case 0x1bd1:
        case 0x3d2:
        case 0x1bd9:
        case 0x1bf1:
        case 0x3f2:
        case 0x1be9:
            return ARM64_SYSOP_DC;

        /* SysOp IC */
        case 0x388:
        case 0x3a8:
        case 0x1ba9:
            return ARM64_SYSOP_IC;

        /* SysOp BRB */
        case 0xb94:
        case 0xb95:
            return ARM64_SYSOP_BRB;

        /* SysOp TLBI */
        case 0x2401:
        case 0x2405:
        case 0x418:
        case 0x2418:
        case 0x3418:
        case 0x419:
        case 0x2419:
        case 0x3419:
        case 0x41a:
        case 0x41b:
        case 0x241c:
        case 0x41d:
        case 0x241d:
        case 0x341d:
        case 0x241e:
        case 0x41f:
        case 0x2421:
        case 0x2425:
        case 0x438:
        case 0x2438:
        case 0x3438:
        case 0x439:
        case 0x2439:
        case 0x3439:
        case 0x43a:
        case 0x43b:
        case 0x243c:
        case 0x43d:
        case 0x243d:
        case 0x343d:
        case 0x243e:
        case 0x43f:
            return ARM64_SYSOP_TLBI;

        /* SysOp Sys */
        default: 
            return ARM64_SYSOP_SYS;
    };
}

int
get_tlbi (unsigned op1, unsigned CRn, unsigned CRm, unsigned op2)
{
    unsigned encoding = (((op1 << 7) | (CRm << 3)) | op2);
    switch (encoding) 
    {
        case 0x18: return ARM64_TLBI_OP_VMALLE1IS;
        case 0x19: return ARM64_TLBI_OP_VAE1IS;
        case 0x1a: return ARM64_TLBI_OP_ASIDE1IS;
        case 0x1b: return ARM64_TLBI_OP_VAAE1IS;
        case 0x1d: return ARM64_TLBI_OP_VALE1IS;
        case 0x1f: return ARM64_TLBI_OP_VAALE1IS;
        case 0x38: return ARM64_TLBI_OP_VMALLE1;
        case 0x39: return ARM64_TLBI_OP_VAE1;
        case 0x3a: return ARM64_TLBI_OP_ASIDE1;
        case 0x3b: return ARM64_TLBI_OP_VAAE1;
        case 0x3d: return ARM64_TLBI_OP_VALE1;
        case 0x3f: return ARM64_TLBI_OP_VAALE1;
        case 0x201: return ARM64_TLBI_OP_IPAS2E1IS;
        case 0x205: return ARM64_TLBI_OP_IPAS2LE1IS;
        case 0x218: return ARM64_TLBI_OP_ALLE2IS;
        case 0x219: return ARM64_TLBI_OP_VAE2IS;
        case 0x21c: return ARM64_TLBI_OP_ALLE1IS;
        case 0x21d: return ARM64_TLBI_OP_VALE2IS;
        case 0x21e: return ARM64_TLBI_OP_VMALLS12E1IS;
        case 0x221: return ARM64_TLBI_OP_IPAS2E1;
        case 0x225: return ARM64_TLBI_OP_IPAS2LE1;
        case 0x238: return ARM64_TLBI_OP_ALLE2;
        case 0x239: return ARM64_TLBI_OP_VAE2;
        case 0x23c: return ARM64_TLBI_OP_ALLE1;
        case 0x23d: return ARM64_TLBI_OP_VALE2;
        case 0x23e: return ARM64_TLBI_OP_VMALLS12E1;
        case 0x318: return ARM64_TLBI_OP_ALLE3IS;
        case 0x319: return ARM64_TLBI_OP_VAE3IS;
        case 0x31d: return ARM64_TLBI_OP_VALE3IS;
        case 0x338: return ARM64_TLBI_OP_ALLE3;
        case 0x339: return ARM64_TLBI_OP_VAE3;
        case 0x33d: return ARM64_TLBI_OP_VALE3;
        case 0x8: return ARM64_TLBI_OP_VMALLE1OS;
        case 0x9: return ARM64_TLBI_OP_VAE1OS;
        case 0xa: return ARM64_TLBI_OP_ASIDE1OS;
        case 0xb: return ARM64_TLBI_OP_VAAE1OS;
        case 0xd: return ARM64_TLBI_OP_VALE1OS;
        case 0xf: return ARM64_TLBI_OP_VAALE1OS;
        case 0x11: return ARM64_TLBI_OP_RVAE1IS;
        case 0x13: return ARM64_TLBI_OP_RVAAE1IS;
        case 0x15: return ARM64_TLBI_OP_RVALE1IS;
        case 0x17: return ARM64_TLBI_OP_RVAALE1IS;
        case 0x29: return ARM64_TLBI_OP_RVAE1OS;
        case 0x2b: return ARM64_TLBI_OP_RVAAE1OS;
        case 0x2d: return ARM64_TLBI_OP_RVALE1OS;
        case 0x2f: return ARM64_TLBI_OP_RVAALE1OS;
        case 0x31: return ARM64_TLBI_OP_RVAE1;
        case 0x33: return ARM64_TLBI_OP_RVAAE1;
        case 0x35: return ARM64_TLBI_OP_RVALE1;
        case 0x37: return ARM64_TLBI_OP_RVAALE1;
        case 0x202: return ARM64_TLBI_OP_RIPAS2E1IS;
        case 0x206: return ARM64_TLBI_OP_RIPAS2LE1IS;
        case 0x208: return ARM64_TLBI_OP_ALLE2OS;
        case 0x209: return ARM64_TLBI_OP_VAE2OS;
        case 0x20c: return ARM64_TLBI_OP_ALLE1OS;
        case 0x20d: return ARM64_TLBI_OP_VALE2OS;
        case 0x20e: return ARM64_TLBI_OP_VMALLS12E1OS;
        case 0x211: return ARM64_TLBI_OP_RVAE2IS;
        case 0x215: return ARM64_TLBI_OP_RVALE2IS;
        case 0x220: return ARM64_TLBI_OP_IPAS2E1OS;

        case 0x224: return ARM64_TLBI_OP_IPAS2LE1OS;
        case 0x308: return ARM64_TLBI_OP_ALLE3OS;
        case 0x309: return ARM64_TLBI_OP_VAE3OS;
        case 0x30d: return ARM64_TLBI_OP_VALE3OS;

        default: return ARM64_TLBI_OP_UNKNOWN;
    }
}

/* --------------------------------------------------------------------------- */

void
base10 (unsigned int hex_value, int width)
{
    for (int i = width; i >= 0; i--) {
        int bit = (hex_value >> i) & 1;
        printf("%d", bit);
    }
    printf ("\n");
}

static int _concat_internal(char **dst, const char *src, va_list args){
    if(!src || !dst)
        return 0;

    size_t srclen = strlen(src), dstlen = 0;

    if(*dst)
        dstlen = strlen(*dst);

    /* Back up args before it gets used. Client calls va_end
     * on the parameter themselves when calling vconcat.
     */
    va_list args1;
    va_copy(args1, args);

    size_t total = srclen + dstlen + vsnprintf(NULL, 0, src, args) + 1;

    char *dst1 = malloc(total);

    if(!(*dst))
        *dst1 = '\0';
    else{
        strncpy(dst1, *dst, dstlen + 1);
        free(*dst);
        *dst = NULL;
    }

    int w = vsnprintf(dst1 + dstlen, total, src, args1);

    va_end(args1);

    *dst = realloc(dst1, strlen(dst1) + 1);

    return w;
}

int 
mstrappend (char **dst, const char *src, ...)
{
    va_list args;
    va_start(args, src);

    int w = _concat_internal(dst, src, args);

    va_end(args);

    return w;
}