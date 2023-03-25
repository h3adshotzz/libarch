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

#ifndef __LIBARCH_ARM64_MISC_H__
#define __LIBARCH_ARM64_MISC_H__

#include <stdlib.h>

/* SysOp's */
enum {
    ARM64_SYSOP_AT = 0,
    ARM64_SYSOP_DC,
    ARM64_SYSOP_IC,
    ARM64_SYSOP_BRB,
    ARM64_SYSOP_TLBI,
    ARM64_SYSOP_SYS,
};

/* address translation names */
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

static const char *A64_AT_NAMES_STR[] =
{
    "S1E1R", "S1E1W", "S1E0R", "S1E0W",
    "S1E2R", "S1E2W", "S12E1R", "S12E1W",
    "S12E0R", "S12E0W", "S1E3R", "S1E3W",
    "S1E1RP", "S1E1WP",
};
static uint64_t A64_AT_NAMES_STR_LEN = sizeof (A64_AT_NAMES_STR) / sizeof (*A64_AT_NAMES_STR);

/* pstate values */
typedef enum arm64_pstate_t
{
    ARM64_PSTATE_SPSEL = 0,
    ARM64_PSTATE_DAIFSET,
    ARM64_PSTATE_DAIFCLR,
    ARM64_PSTATE_UAO,
    ARM64_PSTATE_PAN,
    ARM64_PSTATE_ALLINT,
    ARM64_PSTATE_SSBS,
    ARM64_PSTATE_DIT,
    ARM64_PSTATE_SVCRSM,
    ARM64_PSTATE_SVCRZA,
    ARM64_PSTATE_SVCRSMZA,
    ARM64_PSTATE_TC0,
} arm64_pstate_t;

static const char *A64_PSTATE_STR[] =
{
    "SPSel", "DAIFSet", "DAIFClr", "UAO",
    "PAN", "ALLINT", "SSBS", "DIT", "SVCRSM",
    "SVCRZA", "SVCRSMZA", "TC0",
};
static uint64_t A64_PSTATE_STR_LEN = sizeof (A64_PSTATE_STR) / sizeof (*A64_PSTATE_STR);


/* General Conditions */
typedef enum arm64_cond_t
{
    ARM64_BRANCH_CONDITION_EQ = 0,      // Equal
    ARM64_BRANCH_CONDITION_NE,          // Not Equal
    ARM64_BRANCH_CONDITION_HS,          // Carry Set
    ARM64_BRANCH_CONDITION_LO,          // Carry Clear
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
    "eq", "ne", "hs", "lo", "mi", "pl",
    "vs", "vc", "hi", "ls", "ge", "lt",
    "gt", "le", "al", "nv",
};
static uint64_t A64_CONDITIONS_STR_LEN = sizeof (A64_CONDITIONS_STR) / sizeof (*A64_CONDITIONS_STR);


/* Memory Barrier Conditions */
static const char *A64_MEM_BARRIER_CONDITIONS_STR[] =
{
    "#0x0", "oshld", "oshst", "osh", "#0x4",
    "nshld", "nshst", "nsh", "#0x8", "ishld", 
    "ishst", "ish", "#0xb", "ld", "st", "sy",
};
static uint64_t A64_MEM_BARRIER_CONDITIONS_STR_LEN = sizeof (A64_MEM_BARRIER_CONDITIONS_STR) / sizeof (*A64_MEM_BARRIER_CONDITIONS_STR);


#endif /* __libarch_arm64_misc_h__ */
