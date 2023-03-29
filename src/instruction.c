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

#include <string.h>

#include "instruction.h"
#include "decoder/data-processing.h"
#include "decoder/load-and-store.h"
#include "decoder/branch.h"


instruction_t *
libarch_instruction_create (uint32_t opcode, uint64_t addr)
{
    instruction_t *instr = calloc (1, sizeof (instruction_t));
    instr->opcode = opcode;
    instr->addr = addr;

    /* default extra values */
    instr->cond = -1;
    instr->spec = -1;

    return instr;
}

libarch_return_t
libarch_instruction_add_operand_immediate (instruction_t **instr, uint64_t bits, uint8_t type)
{
    /* Alloc/Realloc operands array */
    if ((*instr)->operands_len == 0) {
        (*instr)->operands = malloc (sizeof (operand_t) * ++(*instr)->operands_len);
    } else {
        operand_t *new = (*instr)->operands = realloc ((*instr)->operands, sizeof (operand_t) * ++(*instr)->operands_len);
        (*instr)->operands = new;
    }

    /* Add the new operand */
    (*instr)->operands[(*instr)->operands_len - 1].op_type = ARM64_OPERAND_TYPE_IMMEDIATE;
    (*instr)->operands[(*instr)->operands_len - 1].imm_bits = bits;
    (*instr)->operands[(*instr)->operands_len - 1].imm_type = type;

    return LIBARCH_RETURN_SUCCESS;
}

libarch_return_t
libarch_instruction_add_operand_shift (instruction_t **instr, uint32_t shift, uint8_t type)
{
    /* Alloc/Realloc operands array */
    if ((*instr)->operands_len == 0) {
        (*instr)->operands = malloc (sizeof (operand_t) * ++(*instr)->operands_len);
    } else {
        operand_t *new = (*instr)->operands = realloc ((*instr)->operands, sizeof (operand_t) * ++(*instr)->operands_len);
        (*instr)->operands = new;
    }

    /* Add the new operand */
    (*instr)->operands[(*instr)->operands_len - 1].op_type = ARM64_OPERAND_TYPE_SHIFT;
    (*instr)->operands[(*instr)->operands_len - 1].shift = shift;
    (*instr)->operands[(*instr)->operands_len - 1].shift_type = type;

    return LIBARCH_RETURN_SUCCESS;
}

libarch_return_t
libarch_instruction_add_operand_register (instruction_t **instr, arm64_reg_t a64reg, uint8_t size, uint8_t type, uint32_t opts)
{
    /* Alloc/Realloc operands array */
    if ((*instr)->operands_len == 0) {
        (*instr)->operands = malloc (sizeof (operand_t) * ++(*instr)->operands_len);
    } else {
        operand_t *new = (*instr)->operands = realloc ((*instr)->operands, sizeof (operand_t) * ++(*instr)->operands_len);
        (*instr)->operands = new;
    }

    if (opts == ARM64_REGISTER_OPERAND_PREFER_ZERO && a64reg == ARM64_REG_SP && (size == 64 || size == 32))
        a64reg = (size == 64) ? ARM64_REG_XZR : ARM64_32_REG_WZR;

    /* Add the new operand */
    (*instr)->operands[(*instr)->operands_len - 1].op_type = ARM64_OPERAND_TYPE_REGISTER;
    (*instr)->operands[(*instr)->operands_len - 1].reg = a64reg;
    (*instr)->operands[(*instr)->operands_len - 1].reg_size = size;
    (*instr)->operands[(*instr)->operands_len - 1].reg_type = type;

    return LIBARCH_RETURN_SUCCESS;
}

libarch_return_t
libarch_instruction_add_operand_register_with_fix (instruction_t **instr, arm64_reg_t a64reg, uint8_t size, uint8_t type, char prefix, char suffix)
{
    /* Alloc/Realloc operands array */
    if ((*instr)->operands_len == 0) {
        (*instr)->operands = malloc (sizeof (operand_t) * ++(*instr)->operands_len);
    } else {
        operand_t *new = (*instr)->operands = realloc ((*instr)->operands, sizeof (operand_t) * ++(*instr)->operands_len);
        (*instr)->operands = new;
    }

    /* Add the new operand */
    (*instr)->operands[(*instr)->operands_len - 1].op_type = ARM64_OPERAND_TYPE_REGISTER;
    (*instr)->operands[(*instr)->operands_len - 1].reg = a64reg;
    (*instr)->operands[(*instr)->operands_len - 1].reg_size = size;
    (*instr)->operands[(*instr)->operands_len - 1].reg_type = type;

    /* Register prefix/suffix, e.g. [x12] has a prefix '[' and suffix ']' */
    (*instr)->operands[(*instr)->operands_len - 1].reg_prefix = prefix;
    (*instr)->operands[(*instr)->operands_len - 1].reg_suffix = suffix;

    return LIBARCH_RETURN_SUCCESS;
}

libarch_return_t
libarch_instruction_add_operand_target (instruction_t **instr, char *target)
{
    /* Alloc/Realloc operands array */
    if ((*instr)->operands_len == 0) {
        (*instr)->operands = malloc (sizeof (operand_t) * ++(*instr)->operands_len);
    } else {
        operand_t *new = (*instr)->operands = realloc ((*instr)->operands, sizeof (operand_t) * ++(*instr)->operands_len);
        (*instr)->operands = new;
    }

    /* Add the new operand */
    (*instr)->operands[(*instr)->operands_len - 1].op_type = ARM64_OPERAND_TYPE_TARGET;
    (*instr)->operands[(*instr)->operands_len - 1].target = strdup (target);

    return LIBARCH_RETURN_SUCCESS;
}

libarch_return_t
libarch_instruction_add_operand_extra (instruction_t **instr, int type, int val)
{
    /* Alloc/Realloc operands array */
    if ((*instr)->operands_len == 0) {
        (*instr)->operands = malloc (sizeof (operand_t) * ++(*instr)->operands_len);
    } else {
        operand_t *new = (*instr)->operands = realloc ((*instr)->operands, sizeof (operand_t) * ++(*instr)->operands_len);
        (*instr)->operands = new;
    }

    /* Add the new operand */
    (*instr)->operands[(*instr)->operands_len - 1].op_type = type;
    (*instr)->operands[(*instr)->operands_len - 1].extra = val;

    return LIBARCH_RETURN_SUCCESS;
}

libarch_return_t
libarch_instruction_add_field (instruction_t **instr, int field)
{
    /* Alloc/Realloc fields array */
    if ((*instr)->fields_len == 0) {
        (*instr)->fields = malloc (sizeof (int) * ++(*instr)->fields_len);
    } else {
        int *new = (*instr)->fields = realloc ((*instr)->fields, sizeof (int) * ++(*instr)->fields_len);
        (*instr)->fields = new;
    }

    /* Add the new field */
    (*instr)->fields[(*instr)->fields_len - 1] = field;

    return LIBARCH_RETURN_SUCCESS;
}


/* move to libarch.c */
libarch_return_t
libarch_disass (instruction_t **instr)
{
    /**
     *  ** AArch64 Instruction Set Encoding **
     * 
     *  Every instruction has some common bits to identify the Decode Group,
     *  these being op0 and op1.
     * 
     *   - op0[31]
     *      Not really relevant here. Section C4.1 of the Arm Reference Manual
     *      only really shows that bit 31 is `0` if the Decode Group is "Reserved".
     * 
     *   - op1[25:28]
     *      This is the important bit. These four bits tell us the decode group,
     *      essentially the type of instruction we're dealing with.
     * 
     */
    unsigned op0 = select_bits ((*instr)->opcode, 31, 31);
    unsigned op1 = select_bits ((*instr)->opcode, 25, 28);

    if (op0 == 0 && op1 == 0) {
        // Reserved
        (*instr)->group = ARM64_DECODE_GROUP_RESERVED;
    } else if (op0 == 1 && op1 == 0) {
        // SME
    } else if (op1 == 2) {
        // SVE
    } else if ((op1 >> 1) == 4) {
        // Data Processing - Immediate
        (*instr)->group = ARM64_DECODE_GROUP_DATA_PROCESS_IMMEDIATE;
        disass_data_processing_instruction (*instr);
    } else if ((op1 >> 1) == 5) {
        // Branch, Exception, System Register
        (*instr)->group = ARM64_DECODE_GROUP_BRANCH_EXCEPTION_SYSREG;
        disass_branch_exception_sys_instruction (*instr);
    } else if ((op1 & ~10) == 4) {
        // Load and Store
        (*instr)->group = ARM64_DECODE_GROUP_LOAD_AND_STORE;
        disass_load_and_store_instruction (*instr);
    } else if ((op1 & ~8) == 5) {
        // Data Processing - Register
        (*instr)->group = ARM64_DECODE_GROUP_DATA_PROCESS_REGISTER;
    } else if ((op1 & ~8) == 7) {
        // Data Processing - Floating
        (*instr)->group = ARM64_DECODE_GROUP_DATA_PROCESS_FLOATING;
    } else {
        // Unknown
        (*instr)->group = ARM64_DECODE_GROUP_UNKNOWN;
    }

    return LIBARCH_RETURN_SUCCESS;
}