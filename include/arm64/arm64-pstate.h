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

#ifndef __LIBARCH_ARM64_PSTATE_H__
#define __LIBARCH_ARM64_PSTATE_H__

#include <stdlib.h>

/**
 *  \brief  AArch64 PSTATE Fields
 * 
 *          Instructions such as `msr` take as an Operand a "PSTATE Field". These
 *          PSTATE Fields are defined here, with string representations in the
 *          A64_PSTATE_STR array.
 * 
 */
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

/**
 *  \brief  String representations of the PSTATE Fields. 
 */
static const char *A64_PSTATE_STR[] =
{
    "SPSel", "DAIFSet", "DAIFClr", "UAO",
    "PAN", "ALLINT", "SSBS", "DIT", "SVCRSM",
    "SVCRZA", "SVCRSMZA", "TC0",
};

/**
 *  \brief  Length of the A64_PSTATE_STR array.
 */
static uint64_t A64_PSTATE_STR_LEN = sizeof (A64_PSTATE_STR) / sizeof (*A64_PSTATE_STR);

#endif /* __libarch_arm64_pstate_h__ */