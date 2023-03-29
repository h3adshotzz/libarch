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

#ifndef __LIBARCH_ARM64_TRANSLATION_H__
#define __LIBARCH_ARM64_TRANSLATION_H__

#include <stdlib.h>

/**
 *  \brief  AArch64 Address Translation Operations.
 * 
 *          The AT instruction takes as it's first Operand an Address Translation
 *          Operation. These are defined here, with string representations in the
 *          A64_AT_NAMES_STR array.
 * 
 */
typedef enum arm64_addr_trans_t
{
    ARM64_AT_NAME_S1E1R = 0,
    ARM64_AT_NAME_S1E1W,
    ARM64_AT_NAME_S1E0R,
    ARM64_AT_NAME_S1E0W,
    ARM64_AT_NAME_S1E2R,
    ARM64_AT_NAME_S1E2W,
    ARM64_AT_NAME_S12E1R,
    ARM64_AT_NAME_S12E1W,
    ARM64_AT_NAME_S12E0R,
    ARM64_AT_NAME_S12E0W,
    ARM64_AT_NAME_S1E3R,
    ARM64_AT_NAME_S1E3W,
    ARM64_AT_NAME_S1E1RP,
    ARM64_AT_NAME_S1E1WP, 
} arm64_addr_trans_t;

/**
 *  \brief  String representations of the Address Translation Operations.
 */
static const char *A64_AT_NAMES_STR[] =
{
    "S1E1R", "S1E1W", "S1E0R", "S1E0W",
    "S1E2R", "S1E2W", "S12E1R", "S12E1W",
    "S12E0R", "S12E0W", "S1E3R", "S1E3W",
    "S1E1RP", "S1E1WP",
};

/**
 *  \brief  Length of teh A64_AT_NAMES_STR array.
 */
static uint64_t A64_AT_NAMES_STR_LEN = sizeof (A64_AT_NAMES_STR) / sizeof (*A64_AT_NAMES_STR);

#endif /* __libarch_arm64_translation_h__ */