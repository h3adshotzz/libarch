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

unsigned int 
select_bits (unsigned int val, unsigned int start, unsigned int end);
unsigned int
sign_extend (unsigned int bits, int numbits);
void
base10 (unsigned int hex_value, int width);

int 
mstrappend (char **dst, const char *src, ...);

#endif /* __libarch_utils_h__ */