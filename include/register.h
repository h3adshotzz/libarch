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

#ifndef __LIBARCH_REGISTER_H__
#define __LIBARCH_REGISTER_H__

#include "libarch.h"
#include "arm64/arm64-registers.h"

extern const char *
libarch_get_general_register (arm64_reg_t reg, const char **list, uint64_t len);
extern const char *
libarch_get_system_register (arm64_reg_t reg);



static const char *A64_REGISTERS_GP_64[] = {
    "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
    "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "x30",
    "sp",  "xzr"
};

static const char *A64_REGISTERS_GP_32[] = {
    "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
    "w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
    "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
    "w24", "w25", "w26", "w27", "w28", "w29", "w30",
    "wsp",  "wzr"
};

static const char *A64_REGISTERS_FP_128[] = {
    "v0",  "v1",  "v2",  "v3",  "v4",  "v5",  "v6",  "v7",
    "v8",  "v9",  "v10", "v11", "v12", "v13", "v14", "v15",
    "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
    "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31",
};

/* add others */

static uint64_t A64_REGISTERS_GP_64_LEN = sizeof (A64_REGISTERS_GP_64) / sizeof (*A64_REGISTERS_GP_64);
static uint64_t A64_REGISTERS_GP_32_LEN = sizeof (A64_REGISTERS_GP_32) / sizeof (*A64_REGISTERS_GP_32);
static uint64_t A64_REGISTERS_FP_128_LEN = sizeof (A64_REGISTERS_FP_128) / sizeof (*A64_REGISTERS_FP_128);



#endif /* __libarch_register_h__ */