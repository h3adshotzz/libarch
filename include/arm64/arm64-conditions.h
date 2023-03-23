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

#ifndef __LIBARCH_ARM64_CONDITIONS_H__
#define __LIBARCH_ARM64_CONDITIONS_H__

#include <stdlib.h>

typedef enum arm64_cond_t
{
    ARM64_BRANCH_CONDITION_EQ = 0,      // Equal
    ARM64_BRANCH_CONDITION_NE,          // Not Equal
    ARM64_BRANCH_CONDITION_CS,          // Carry Set
    ARM64_BRANCH_CONDITION_CC,          // Carry Clear
    ARM64_BRANCH_CONDITION_MI,          // Minus, Negative
    ARM64_BRANCH_CONDITION_PL,          // Plus, positive, zero
    ARM64_BRANCH_CONDITION_VS,          // Overflow
    ARM64_BRANCH_CONDITION_VC,          // No Overflow
    ARM64_BRANCH_CONDITION_HI,          // Unsigned higher
    ARM64_BRANCH_CONDITION_LS,          // Unsigned lower or same
    ARM64_BRANCH_CONDITION_GE,          // Signed greater than or equal
    ARM64_BRANCH_CONDITION_LT,          // Signed less than
    ARM64_BRANCH_CONDITION_GT,          // Signed greater than
    ARM64_BRANCH_CONDITION_LE,          // Signed less than or equal
    ARM64_BRANCH_CONDITION_AL,          // Always
    ARM64_BRANCH_CONDITION_NV,          // Always
} arm64_cond_t;

static const char *A64_CONDITIONS_STR[] =
{
    "eq", "ne", "cs", "cc", "mi", "pl",
    "vs", "vc", "hi", "ls", "ge", "lt",
    "gt", "le", "al", "nv",
};
static uint64_t A64_CONDITIONS_STR_LEN = sizeof (A64_CONDITIONS_STR) / sizeof (*A64_CONDITIONS_STR);

#endif /* __libarch_arm64_conditions_h__ */
