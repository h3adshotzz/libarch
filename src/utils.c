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