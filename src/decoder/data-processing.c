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

#include "decoder/data-processing.h"
#include "utils.h"


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_pc_relative_addressing (instruction_t **instr)
{
    unsigned op = select_bits ((*instr)->opcode, 31, 31);
    unsigned immlo = select_bits ((*instr)->opcode, 29, 30);
    unsigned immhi = select_bits ((*instr)->opcode, 5, 23);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, op);
    libarch_instruction_add_field (instr, immlo);
    libarch_instruction_add_field (instr, immhi);
    libarch_instruction_add_field (instr, Rd);

    /* Calculate immediate value */
    uint64_t imm = 0;
    uint64_t imm_type = ARM64_IMMEDIATE_TYPE_ULONG;

    /* Determine the instruction type and add the register operands */
    if (op == 0) {
        (*instr)->type = ARM64_INSTRUCTION_ADR;

        imm = ((immhi << 2) | immlo);
        imm = sign_extend (imm, 21);
        imm += (*instr)->addr;

        imm_type |= ARM64_IMMEDIATE_FLAG_OUTPUT_DECIMAL;

    } else {
        (*instr)->type = ARM64_INSTRUCTION_ADRP;

        imm = ((immhi << 2) | immlo) << 12;
        imm = sign_extend (imm, 32);
        imm += (*instr)->addr;
    }

    /* Add operands */
    libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
    libarch_instruction_add_operand_immediate (instr, imm, imm_type);

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_add_subtract_immediate (instruction_t **instr)
{
    unsigned sf = select_bits ((*instr)->opcode, 31, 31);
    unsigned op = select_bits ((*instr)->opcode, 30, 30);
    unsigned S = select_bits ((*instr)->opcode, 29, 29);
    unsigned sh = select_bits ((*instr)->opcode, 22, 22);
    unsigned imm12 = select_bits ((*instr)->opcode, 10, 21);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, sf);
    libarch_instruction_add_field (instr, op);
    libarch_instruction_add_field (instr, S);
    libarch_instruction_add_field (instr, sh);
    libarch_instruction_add_field (instr, imm12);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rd);

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sf == 1) _SET_64 (size, regs, len);
    else _SET_32 (size, regs, len);

    /* Determine the instruction type and add the register operands */
    if (op == 0 && S == 0) {

        /* Common operands */
        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

        /* MOV */
        if (sh == 0 && imm12 == 0 && (Rd == 0b11111 || Rn == 0b11111)) {
            (*instr)->type = ARM64_INSTRUCTION_MOV;

        /* ADD (immediate) */
        } else {
            (*instr)->type = ARM64_INSTRUCTION_ADD;
            libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

            /* Add the left-shift if present */
            if (sh) libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);
        }

    } else if (op == 0 && S == 1) {

        /**
         * While there are common operands with CMD and ADDS (immediate), 
         * they're not in the same order. 
         */
        /* CMN */
        if (Rd == 0b11111) {
            (*instr)->type = ARM64_INSTRUCTION_CMN;

            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

            /* Add the left-shift if present */
            if (sh) libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);

        /* ADDS (immediate) */
        } else {
            (*instr)->type = ARM64_INSTRUCTION_ADDS;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

            /* Add the left-shift if present */
            if (sh) libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);
        }

    } else if (op == 1 && S == 0) {

        /* SUB */
        (*instr)->type = ARM64_INSTRUCTION_SUB;

        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
        libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

        /* Add the left-shift if present */
        if (sh) libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);

    } else if (op == 1 && S == 1) {

        /* CMP */
        if (Rd == 0b11111) {
            (*instr)->type = ARM64_INSTRUCTION_CMP;

            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

            /* Add the left-shift if present */
            if (sh) libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);

        /* SUBS (immediate) */
        } else {
            (*instr)->type = ARM64_INSTRUCTION_SUBS;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

            /* Add the left-shift if present */
            if (sh) libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);
        }

    } else {
        return LIBARCH_DECODE_STATUS_SOFT_FAIL;
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_add_subtract_immediate_tags (instruction_t **instr)
{

}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_logical_immediate (instruction_t **instr)
{
    unsigned sf = select_bits ((*instr)->opcode, 31, 31);
    unsigned opc = select_bits ((*instr)->opcode, 29, 30);
    unsigned N = select_bits ((*instr)->opcode, 22, 22);
    unsigned immr = select_bits ((*instr)->opcode, 16, 21);
    unsigned imms = select_bits ((*instr)->opcode, 10, 15);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, sf);
    libarch_instruction_add_field (instr, opc);
    libarch_instruction_add_field (instr, N);
    libarch_instruction_add_field (instr, immr);
    libarch_instruction_add_field (instr, imms);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rd);

    /* Unallocated */
    if (sf == 0 && N == 1) return LIBARCH_DECODE_STATUS_SOFT_FAIL;

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sf == 0 && N == 0) _SET_32 (size, regs, len);
    else _SET_64 (size, regs, len);

    /* Work out the immediate type and value */
    int imm_type = (size == 64) ? ARM64_IMMEDIATE_TYPE_LONG : ARM64_IMMEDIATE_TYPE_INT; 
    unsigned long imm = 0;
    decode_bitmasks (N, imms, immr, 1, &imm);

    /* Everything apart from `tst` has an Rd register as the first operand */
    if (!(opc == 3 && Rd == 0b11111))
        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

    /* Determine the instruction type and add the register operands */
    if (opc == 0) {
        (*instr)->type = ARM64_INSTRUCTION_AND;
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

    } else if (opc == 1) {

        if (Rn == 0b11111 && !move_wide_preferred (sf, N, imms, immr)) {
            (*instr)->type = ARM64_INSTRUCTION_MOV;
        } else {
            (*instr)->type = ARM64_INSTRUCTION_ORR;
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
        }

    } else if (opc == 2) {
        (*instr)->type = ARM64_INSTRUCTION_EOR;
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

    } else if (opc == 3) {

        if (Rd == 0b11111) {
            (*instr)->type = ARM64_INSTRUCTION_TST;
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
        } else {
            (*instr)->type = ARM64_INSTRUCTION_ANDS;
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
        }

    } else {
        return LIBARCH_DECODE_STATUS_SOFT_FAIL;
    }

    /* Add the immediate operand */
    libarch_instruction_add_operand_immediate (instr, 
        (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm,
         imm_type);

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_move_wide_immediate (instruction_t **instr)
{

}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_bitfield (instruction_t **instr)
{

}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_extract (instruction_t **instr)
{

}




///////////////////////////////////////////////////////////////////////////////

LIBARCH_API
decode_status_t
disass_data_processing_instruction (instruction_t *instr)
{
    unsigned op0 = select_bits (instr->opcode, 23, 25);

    if ((op0 >> 1) == 0) {
        if (decode_pc_relative_addressing (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_PC_RELATIVE_ADDRESSING;
    } else if (op0 == 2) {
        if (decode_add_subtract_immediate (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_ADD_SUBTRACT_IMMEDIATE;
    } else if (op0 == 3) {
        if (decode_add_subtract_immediate_tags (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_ADD_SUBTRACT_IMMEDIATE_TAGS;
    } else if (op0 == 4) {
        if (decode_logical_immediate (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_LOGICAL_IMMEDIATE;
    } else if (op0 == 5) {
        if (decode_move_wide_immediate (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_MOVE_WIDE_IMMEDIATE;
    } else if (op0 == 6) {
        if (decode_bitfield (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_BITFIELD;
    } else if (op0 == 7) {
        if (decode_extract (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_EXTRACT;
    }

    return (instr->subgroup != ARM64_DECODE_SUBGROUP_UNKNOWN) ? LIBARCH_DECODE_STATUS_SUCCESS : LIBARCH_DECODE_STATUS_SOFT_FAIL;
}