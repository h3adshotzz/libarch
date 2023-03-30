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

#include "decoder/load-and-store.h"

LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_compare_and_swap_pair (instruction_t **instr)
{
    /**
     *  NOTE:   These instructions belong to the FEAT_LSE instruction set
     *          extension, and these aren't supported on Apple platforms.
     * 
     *          For now, these wont be implemented. See more on page 549
     *          of the ARM reference manual.
    */
    return LIBARCH_DECODE_STATUS_SOFT_FAIL;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_advanced_simd_load_store_multiple_structures (instruction_t **instr)
{
    unsigned Q = select_bits ((*instr)->opcode, 30, 30);
    unsigned op2 = select_bits ((*instr)->opcode, 23, 24);
    unsigned L = select_bits ((*instr)->opcode, 22, 22);
    unsigned Rm = select_bits ((*instr)->opcode, 16, 20);
    unsigned opcode = select_bits ((*instr)->opcode, 12, 15);
    unsigned size = select_bits ((*instr)->opcode, 10, 11);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, Q);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, L);
    libarch_instruction_add_field (instr, Rm);
    libarch_instruction_add_field (instr, opcode);
    libarch_instruction_add_field (instr, size);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rt);

    /**
     *  STx and LDx instruction encodings are the same. L determines whether its an ST or LD
     * 
     *  op2 = 1     L = 0   Rm != 11111     opcode = 0      -   ST4 (multiple structures, register offset)
     *  op2 = 1     L = 0   Rm == 11111     opcode = 0      -   ST4 (multiple structures, imm offset)
     *  op2 = 0     L = 0                   opcode = 0      -   ST4 (multiple structures, no offset)
     * 
     *  op2 = 1     L = 0   Rm != 11111     opcode = 2      -   ST1 (multiple structures, 4 register, register offset)
     *  op2 = 1     L = 0   Rm != 11111     opcode = 6      -   ST1 (multiple structures, 3 register, register offset)
     *  op2 = 1     L = 0   Rm != 11111     opcode = 10     -   ST1 (multiple structures, 2 register, register offset)
     *  op2 = 1     L = 0   Rm != 11111     opcode = 7      -   ST1 (multiple structures, 1 register, register offset)
     *  op2 = 1     L = 0   Rm == 11111     opcode = 2      -   ST1 (multiple structures, 4 register, imm offset)
     *  op2 = 1     L = 0   Rm == 11111     opcode = 6      -   ST1 (multiple structures, 3 register, imm offset)
     *  op2 = 1     L = 0   Rm == 11111     opcode = 10     -   ST1 (multiple structures, 2 register, imm offset)
     *  op2 = 1     L = 0   Rm == 11111     opcode = 7      -   ST1 (multiple structures, 1 register, imm offset)
     *  op2 = 0     L = 0                   opcode = 2      -   ST1 (multiple structures, 4 register)
     *  op2 = 0     L = 0                   opcode = 6      -   ST1 (multiple structures, 3 register)
     *  op2 = 0     L = 0                   opcode = 10     -   ST1 (multiple structures, 2 register)
     *  op2 = 0     L = 0                   opcode = 7      -   ST1 (multiple structures, 1 register)
     * 
     *  op2 = 1     L = 0   Rm != 11111     opcode = 4      -   ST3 (multiple structures, register offset)
     *  op2 = 1     L = 0   Rm == 11111     opcode = 4      -   ST3 (multiple structures, imm offset)
     *  op2 = 0     L = 0                   opcode = 4      -   ST3 (multiple structures, no offset)
     * 
     *  op2 = 1     L = 0   Rm != 11111     opcode = 8      -   ST2 (multiple structures, register offset)
     *  op2 = 1     L = 0   Rm == 11111     opcode = 8      -   ST2 (multiple structures, imm offset)
     *  op2 = 0     L = 0                   opcode = 8      -   ST2 (multiple structures, no offset)
     * 
     *  op2 = 1     
     * 
    */

    /**
     *  The value of `opcode` determines how many register operands each instruction
     *  of this type has, whether it's an STx or LDx. 
     */
    int reg_count, elem;
    switch (opcode)
    {
        /* 4 registers */
        case 0:     reg_count = 4; elem = 4; break;
        case 2:     reg_count = 4; elem = 1; break;

        /* 3 registers */
        case 4:     reg_count = 3; elem = 3; break;
        case 6:     reg_count = 3; elem = 1; break;

        /* 2 registers */
        case 8:     reg_count = 2; elem = 2; break;
        case 10:    reg_count = 2; elem = 1; break;

        /* 1 register */
        case 7:     reg_count = 1; elem = 1; break;

        default: return LIBARCH_RETURN_VOID;
    }

    /* Work out the register type */
    if (L == 0) (*instr)->type = (ARM64_INSTRUCTION_ST1 - 1) + elem;
    else (*instr)->type = (ARM64_INSTRUCTION_LD1 - 1) + ((elem * 2) - 1);

    /* Work out the arrangement specifier */
    int vec_table[][3] = {
        { 0, 0, ARM64_VEC_ARRANGEMENT_8B },
        { 0, 1, ARM64_VEC_ARRANGEMENT_16B },
        { 1, 0, ARM64_VEC_ARRANGEMENT_4H },
        { 1, 1, ARM64_VEC_ARRANGEMENT_8H },
        { 2, 0, ARM64_VEC_ARRANGEMENT_2S },
        { 2, 1, ARM64_VEC_ARRANGEMENT_4S },
        { 3, 1, ARM64_VEC_ARRANGEMENT_2D },
    };
    for (int i = 0; i < sizeof (vec_table) / sizeof (vec_table[0]); i++) {
        if (vec_table[i][0] == size && vec_table[i][1] == Q)
            (*instr)->spec = vec_table[i][2];
    }

    /* Set the register operands */
    if (reg_count == 1) {
        libarch_instruction_add_operand_register_with_fix (instr, Rt, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, '{', '}');
    } else {
        for (int i = Rt; i < Rt + reg_count; i++) {
            if (i == Rt) libarch_instruction_add_operand_register_with_fix (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, '{', NULL);
            else if (i == Rt + reg_count - 1) libarch_instruction_add_operand_register_with_fix (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, NULL, '}');
            else libarch_instruction_add_operand_register (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_REGISTER_OPERAND_OPT_NONE);
        }
    }

    libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');  

    /* Is the instruction post-indexed? */
    if (op2 == 1) {
        if (Rm != 0b11111) libarch_instruction_add_operand_register (instr, Rm, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
        else {
            int imm = 0;

            /* Choose the correct immediate value */
            int imm_table[] = {8, 16, 24, 32};
            int imm_table_q[] = {16, 32, 48, 64};

            if (reg_count >= 1 && reg_count <= 4)
                imm = (Q == 0) ? imm_table[reg_count - 1] : imm_table_q[reg_count - 1];

            libarch_instruction_add_operand_immediate (instr, imm, ARM64_IMMEDIATE_TYPE_INT | ARM64_IMMEDIATE_FLAG_OUTPUT_DECIMAL);
        }
    }
    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_advanced_simd_load_store_single_structure (instruction_t **instr)
{
    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_load_store_memory_tags (instruction_t **instr)
{
    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_load_store_exclusive_pair (instruction_t **instr)
{
    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_load_store_exclusive_register (instruction_t **instr)
{
    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_load_store_ordered (instruction_t **instr)
{
    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_load_register_literal (instruction_t **instr)
{
    return LIBARCH_DECODE_STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

LIBARCH_API
decode_status_t
disass_load_and_store_instruction (instruction_t *instr)
{
    unsigned op0 = select_bits (instr->opcode, 28, 31);
    unsigned op1 = select_bits (instr->opcode, 26, 26);
    unsigned op2 = select_bits (instr->opcode, 23, 24);
    unsigned op3 = select_bits (instr->opcode, 16, 21);
    unsigned op4 = select_bits (instr->opcode, 10, 11);

    if (op0 == 0 && op1 == 0 && op2 == 0 && (op3 >> 5) == 1) {
        if (decode_compare_and_swap_pair (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_COMPARE_AND_SWAP_PAIR;

    } else if (((op0 == 4 || op0 == 0) && op1 == 1 && op2 == 0 && op3 == 0) ||
               ((op0 == 4 || op0 == 0) && op1 == 1 && op2 == 1 && (op3 >> 5) == 0)) {
        if (decode_advanced_simd_load_store_multiple_structures (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_ADVANCED_SIMD_LOAD_STORE_MULT_STRUCT;

    } else if (((op0 == 4 || op0 == 0) && op1 == 1 && op2 == 2 && (op3 == 0 || (op3 >> 5) == 1)) ||
               ((op0 == 4 || op0 == 0) && op1 == 1 == op2 == 3)) {
        if (decode_advanced_simd_load_store_single_structure (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_ADVANCED_SIMD_LOAD_STORE_SINGLE_STRUCT;

    } else if (op0 == 13 && op1 == 0 && (op2 == 2 || op2 == 3) && (op3 >> 5) == 1) {
        if (decode_load_store_memory_tags (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_LOAD_STORE_MEMORY_TAGS;

    } else if (((op0 >> 2) >= 0 || (op0 >> 2) <= 3) && op1 == 0 && op2 == 1 && (op3 >> 5) == 0) {
        if (decode_load_store_ordered (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_LOAD_STORE_ORDERED;

    } else if ((op0 & ~12) == 0 && op1 == 0 && (op2 >> 1) == 0) {
        if ((op3 >> 5) == 1) {
            if (decode_load_store_exclusive_pair (&instr))
                instr->subgroup = ARM64_DECODE_SUBGROUP_LOAD_STORE_EXCL_PAIR;
        } else {
            if (decode_load_store_exclusive_register (&instr))
                instr->subgroup = ARM64_DECODE_SUBGROUP_LOAD_STORE_EXCL_REGISTER;
        }
    } else if (((op0 >> 2) >= 1 && (op0 >> 2) <= 3) && (op2 == 0 || op2 == 1)) {
        if (decode_load_register_literal (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_LOAD_REGISTER_LITERAL;

    } else if (((op0 >> 2) >= 0 || (op0 >> 2) <= 3) && op1 == 0 && op2 == 1 && (op3 >> 5) == 1) {
        printf ("Compare and swap\n");
        // Not Implemented Yet

    } else if (((op0 >> 2) >= 1 || (op0 >> 2) <= 3) && op1 == 0 && (op2 == 2 || op2 == 3) && (op3 >> 5) == 0 && op4 == 0) {
        printf ("LDAPR/STLR (Unscaled immediate)\n");
        // Not Implemented Yet

    } else {
        printf ("not implemented\n");
    }

    return LIBARCH_RETURN_SUCCESS;
}