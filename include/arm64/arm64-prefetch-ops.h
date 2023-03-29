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

#ifndef __LIBARCH_ARM64_PREFETCH_OPS_H__
#define __LIBARCH_ARM64_PREFETCH_OPS_H__

#include <stdlib.h>

/**
 *  \brief  AArch64 Prefetch Operations used when decoding the `prfm` instruction
 *          in the Load and Stores Decode Group.
 * 
 *          The `prfop` operand of the `prfm` instruction is made up of a type,
 *          target and policy. Those potential values are:
 * 
 *          ** Type **
 *           - PLD  -   Prefetch for Load.
 *           - PLI  -   Preload Instructions.
 *           - PST  -   Prefetch for Store.
 * 
 *          ** Target **
 *           - L1   -   Level 1 Cache.
 *           - L2   -   Level 2 Cache.
 *           - L3   -   Level 3 Cache.
 * 
 *          ** Policy **
 *           - KEEP -   Retained or Temporal Prefetch
 *           - STRM -   Streaming or Non-Temporal Prefetch.
 */
enum {
    ARM64_PRFOP_PLDL1KEEP = 0,
    ARM64_PRFOP_PLDL2KEEP,
    ARM64_PRFOP_PLDL3KEEP,
    ARM64_PRFOP_PLDL1STRM,
    ARM64_PRFOP_PLDL2STRM,
    ARM64_PRFOP_PLDL3STRM,

    ARM64_PRFOP_PLIL1KEEP,
    ARM64_PRFOP_PLIL2KEEP,
    ARM64_PRFOP_PLIL3KEEP,
    ARM64_PRFOP_PLIL1STRM,
    ARM64_PRFOP_PLIL2STRM,
    ARM64_PRFOP_PLIL3STRM,
    
    ARM64_PRFOP_PSTL1KEEP,
    ARM64_PRFOP_PSTL2KEEP,
    ARM64_PRFOP_PSTL3KEEP,
    ARM64_PRFOP_PSTL1STRM,
    ARM64_PRFOP_PSTL2STRM,
    ARM64_PRFOP_PSTL3STRM,
};

/**
 *  \brief  String representations of the Prefetch Operations.
 * 
 */
static const char *A64_PRFOP_STR[] =
{
    "pldlel1keep", "pldlel2keep", "pldlel3keep", "pldlel1strm", "pldlel2strm", "pldlel3strm",
    "plilel1keep", "plilel2keep", "plilel3keep", "plilel1strm", "plilel2strm", "plilel3strm",
    "pstlel1keep", "pstlel2keep", "pstlel3keep", "pstlel1strm", "pstlel2strm", "pstlel3strm", 
};

/**
 *  \brief  Length of the A64_PRFOP_STR array.
*/
static uint64_t A64_PRFOP_STR_LEN = sizeof (A64_PRFOP_STR) / sizeof (*A64_PRFOP_STR);



#endif /* __libarch_arm64_prefetch_h__ */