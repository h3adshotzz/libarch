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

#ifndef __LIBARCH_ARM64_VECTOR_SPECIFIERS_H__
#define __LIBARCH_ARM64_VECTOR_SPECIFIERS_H__

#include <stdlib.h>

/**
 *  \brief  AArch64 Vector Arrangement Specifiers.
 * 
 *          Instructions involving vector registers, i.e. v20, have an "Arrangement
 *          Specifier" suffix, for example v20.8b. These Arrangement Specifiers
 *          are defined here, with string representations in the A64_VEC_SPECIFIER_STR.
 * 
 */
typedef enum arm64_vec_specifier_t
{
    ARM64_VEC_ARRANGEMENT_B = 0,
    ARM64_VEC_ARRANGEMENT_8B,
    ARM64_VEC_ARRANGEMENT_16B,
    ARM64_VEC_ARRANGEMENT_H,
    ARM64_VEC_ARRANGEMENT_4H,
    ARM64_VEC_ARRANGEMENT_8H,
    ARM64_VEC_ARRANGEMENT_S,
    ARM64_VEC_ARRANGEMENT_2S,
    ARM64_VEC_ARRANGEMENT_4S,
    ARM64_VEC_ARRANGEMENT_D,
    ARM64_VEC_ARRANGEMENT_2D,
} arm64_vec_specifier_t;

/**
 *  \brief  String representatios of the Vector Arrangement Specifiers.
 * 
 */
static const char *A64_VEC_SPECIFIER_STR[] =
{
    "b", "8b", "16b", "h", "4h", "8h", "s", "2s",
    "4s", "d", "2d",
};

/**
 *  \brief  Length of the A64_VEC_SPECIFIER_STR array.
 */
static uint64_t A64_VEC_SPECIFIER_STR_LEN = sizeof (A64_VEC_SPECIFIER_STR) / sizeof (*A64_VEC_SPECIFIER_STR);

#endif /* __libarch_arm64_vector_specifiers_h__ */