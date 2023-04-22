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

#ifndef __LIBARCH_ARM64_INDEX_EXTEND_H__
#define __LIBARCH_ARM64_INDEX_EXTEND_H__

#include <stdlib.h>

/**
 *  \brief  AArch64 Index Extend Specifier
 * 
 *          ** Type **
 *           - UXTW
 *           - SXTW
 *           - SXTX
 * 
 *          Index shift amount can be either 0 or 1.
 */
enum {
    ARM64_INDEX_EXTEND_UXTW = 2,
    ARM64_INDEX_EXTEND_SXTW = 6,
    ARM64_INDEX_EXTEND_SXTX = 7,
};

/**
 *  \brief  String representations of the Index Extend Specifiers.
 * 
 */
static const char *A64_INDEX_EXTEND_STR[] =
{
    "#0", "#0", "uxtw", "#0", "#0", "#0", "sxtw", "sxtx",
};

/**
 *  \brief  Length of the A64_INDEX_EXTEND_STR array.
*/
static uint64_t A64_INDEX_EXTEND_LEN = sizeof (A64_INDEX_EXTEND_STR) / sizeof (*A64_INDEX_EXTEND_STR);



#endif /* __libarch_arm64_index_extend_h__ */