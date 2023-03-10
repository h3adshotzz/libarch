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

int decode_bit_masks
(unsigned int N, unsigned int imms, unsigned int immr,
        int immediate, unsigned long *out)
{
    unsigned int num = (N << 6) | (~imms & 0x3f);
    unsigned int len = HighestSetBit(num, 7);

    if(len < 1)
        return -1;

    unsigned int levels = Ones(len, 0);

    if(immediate && ((imms & levels) == levels))
        return -1;

    unsigned int S = imms & levels;
    unsigned int R = immr & levels;
    unsigned int esize = 1 << len;

    *out = replicate(RORZeroExtendOnes(S + 1, esize, R), sizeof(unsigned long) * CHAR_BIT, esize);

    return 0;
}

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