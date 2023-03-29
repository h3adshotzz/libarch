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

#ifndef __LIBARCH_ARM64_COMMON_H__
#define __LIBARCH_ARM64_COMMON_H__

#include <stdlib.h>
#include "libarch.h"

#include "arm64/arm64-registers.h"

/******************************************************************************
*       Register Size Macro's
*******************************************************************************/

/**
 *  \brief  Set the given variables to the correct value for 64-bit wide 
 *          registers.
 * 
*/
#define _SET_64(sz, regs, len) \
    do { \
        sz = 64; \
        regs = A64_REGISTERS_GP_64; \
        len = A64_REGISTERS_GP_64_LEN; \
    } while (0) \

/**
 *  \brief  Set the given variables to the correct value for 32-bit wide 
 *          registers.
 * 
*/
#define _SET_32(sz, regs, len) \
    do { \
        sz = 32; \
        regs = A64_REGISTERS_GP_32; \
        len = A64_REGISTERS_GP_32_LEN; \
    } while (0) \

/******************************************************************************
*       Decoder Helper Functions
*******************************************************************************/

/**
 *  \brief  Select the bits of given unsigned integer `val` from the index `start`
 *          to `end`, and return the value.
 * 
 *  \param      val     Unsigned integer.
 *  \param      start   Start index.
 *  \param      end     End index.
 * 
 *  \return Selected bits val[start:end].
 */
LIBARCH_API uint32_t
select_bits (uint32_t val, uint32_t start, uint32_t end);

/******************************************************************************
*       AArch64-defined Functions
*******************************************************************************/

#define ARM64_COMMON

/**
 *  \brief  Sign-extend a given unsigned integer `bits`. This function is defined
 *          in the AArch64 Architecture Reference Manual.
 *
 *  \param      bits        Unsigned Integer to sign-extend.
 *  \param      numbits     Desired size.
 * 
 *  \return Sign-extended value `bits`.
 * 
*/
LIBARCH_API ARM64_COMMON uint32_t
arm64_sign_extend (uint32_t bits, int32_t numbits);


/**
 *  \brief  Fetch the highest-set bit in a given unsigned integer, up to a
 *          specified index set in `n`. This function is defined in the 
 *          AArch64 Architecture Reference Manual.
 * 
 *  \param      n       Max index.
 *  \param      imm     Unsigned integer.
 * 
 *  \return Highest set bit of `imm` up to index `n`.
 * 
*/
LIBARCH_API ARM64_COMMON uint32_t
arm64_highest_set_bit (uint32_t imm, uint32_t n);


/**
 *  \brief  Zero-extend a given unsigned integer from specified width `M`
 *          filling the LSBs with ones. This function is defined in the 
 *          AArch64 Architecture Reference Manual.
 * 
 *  \param      M   Specified bit-width.
 *  \param      N   Reserved.
 * 
 *  \return Zero-extended value `M`.
 * 
 */
LIBARCH_API ARM64_COMMON uint32_t
arm64_zero_extend_ones (uint32_t M, uint32_t N);


/**
 *  \brief  Zero-extend and right rotation of an unsigned integer value from 
 *          a specified bit width `M` to a larger bit width `N`, rotating the 
 *          bits `R` positions to the right. This function is defined in the 
 *          AArch64 Architecture Reference Manual.
 * 
 *  \param      M   Specified bit-width.
 *  \param      N   Target bit width of zero-extended value after rotation.
 *  \param      R   Number of bit positions to rotate the value to the right.
 * 
 *  \return Zero-extended, rotated value.
 * 
 */
LIBARCH_API ARM64_COMMON uint32_t
arm64_ror_extend_ones (uint32_t M, uint32_t N, uint32_t R);


/**
 *  \brief  Replicates a given unsigned integer `val` every `bits` bits. This 
 *          function is defined in the AArch64 Architecture Reference Manual.
 * 
 *  \param      val     Unsigned integer to replicate.
 *  \param      bits    The number of bits to replicate.
 * 
 *  \return Replicated value.
 * 
 */
LIBARCH_API ARM64_COMMON uint32_t
arm64_replicate (uint32_t val, uint32_t bits);


/**
 *  \brief  Decodes bitmasks used in ARM64 instructions and returns a new 
 *          value based on the decoded bitmask parameters. This function is 
 *          defined in the AArch64 Architecture Reference Manual.
 * 
 *  \param      immN        The 6-bit immediate value specifying the number of bits 
 *                          to set in the result bitmask.
 *  \param      imms        The 6-bit immediate value specifying the starting bit 
 *                          position of the mask.
 *  \param      immr        The 6-bit immediate value specifying the bit position of 
 *                          the rightmost bit of the mask.
 *  \param      immediate   A flag indicating whether the `imms` and `immr` values 
 *                          represent an immediate value or a register value.
 *  \param      newval      A pointer to a `uint64_t` variable that will hold the 
 *                          decoded bitmask value.
 * 
 *  \return Returns 0 if success, or -1 if an error occurred.
 * 
 */
LIBARCH_API ARM64_COMMON uint32_t
arm64_decode_bitmasks (uint32_t immN, uint32_t imms, uint32_t immr, int32_t immediate, uint64_t *newval);


/**
 * 
 */
LIBARCH_API ARM64_COMMON int32_t
arm64_move_wide_preferred (uint32_t sf, uint32_t immN, uint32_t immr, uint32_t imms);


/**
 * 
 * 
 */
LIBARCH_API ARM64_COMMON uint64_t
arm64_ones (int32_t len, int32_t N);


/**
 * 
 * 
 */
LIBARCH_API ARM64_COMMON int32_t
arm64_is_zero (uint32_t N);


/**
 *
 *  
 */
LIBARCH_API ARM64_COMMON int32_t
arm64_is_ones (uint32_t N);


/**
 * 
 * 
 */
LIBARCH_API ARM64_COMMON int32_t
arm64_uint (uint32_t N);


/**
 * 
 * 
 */
LIBARCH_API ARM64_COMMON int32_t
arm64_bfx_preferred (uint32_t sf, uint32_t uns, uint32_t imms, uint32_t immr);



#endif /* __libarch_arm64_common_h__ */