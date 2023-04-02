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

            libarch_instruction_add_operand_immediate (instr, imm, ARM64_IMMEDIATE_TYPE_INT, ARM64_IMMEDIATE_OPERAND_OPT_PREFER_DECIMAL);
        }
    }
    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_advanced_simd_load_store_single_structure (instruction_t **instr)
{
    unsigned Q = select_bits ((*instr)->opcode, 30, 30);
    unsigned op2 = select_bits ((*instr)->opcode, 23, 24);
    unsigned L = select_bits ((*instr)->opcode, 22, 22);
    unsigned R = select_bits ((*instr)->opcode, 21, 21);
    unsigned Rm = select_bits ((*instr)->opcode, 16, 20);
    unsigned opcode = select_bits ((*instr)->opcode, 13, 15);
    unsigned S = select_bits ((*instr)->opcode, 12, 12);
    unsigned size = select_bits ((*instr)->opcode, 10, 11);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, Q);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, L);
    libarch_instruction_add_field (instr, R);
    libarch_instruction_add_field (instr, Rm);
    libarch_instruction_add_field (instr, opcode);
    libarch_instruction_add_field (instr, S);
    libarch_instruction_add_field (instr, size);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rt);

    unsigned elem = (((opcode & 1) << 1) | R) + 1;
    int index = 0;

    /* Determine the Vector Arrangement */
    int replicate = 0;
    switch ((opcode >> 1))
    {
        case 3: replicate = 1; break;
        case 0:
            index = (Q << 3) | (S << 2) | size;
            (*instr)->spec = ARM64_VEC_ARRANGEMENT_B;
            break;

        case 1:
            index = (Q << 2) | (S << 1) | (size >> 1);
            (*instr)->spec = ARM64_VEC_ARRANGEMENT_H;
            break;
        
        case 2:
            if ((size & 1) == 0) {
                index = (Q << 1) | S;
                (*instr)->spec = ARM64_VEC_ARRANGEMENT_S;
            } else {
                index = Q;
                (*instr)->spec = ARM64_VEC_ARRANGEMENT_D;
            }
        default: LIBARCH_RETURN_VOID;
    }

    /* Select correct instruction type */
    int reg_count = 0;
    if (replicate) {
        reg_count = ((elem * 2) - 1);
        (*instr)->type = (ARM64_INSTRUCTION_LD1R - 1) + reg_count;
    } else if (L == 0) {
        reg_count = elem;
        (*instr)->type = (ARM64_INSTRUCTION_ST1 - 1) + reg_count;
    } else if (L == 1) {
        reg_count = ((elem * 2) - 1);
        (*instr)->type = (ARM64_INSTRUCTION_LD1 - 1) + reg_count;
    }

    /* Set the register operands */
    if (reg_count == 1) {
        libarch_instruction_add_operand_register_with_fix (instr, Rt, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, '{', '}');
    } else {
        for (int i = 0; i < reg_count; i++) {
            if (i == 0) libarch_instruction_add_operand_register_with_fix (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, '{', NULL);
            else if (i == reg_count - 1) libarch_instruction_add_operand_register_with_fix (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, NULL, '}');
            else libarch_instruction_add_operand_register (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_REGISTER_OPERAND_OPT_NONE);
        }
    }

    /* Add the index */
    libarch_instruction_add_operand_immediate_with_fix (instr, *(unsigned int *) &index, ARM64_IMMEDIATE_TYPE_UINT, '[', ']');

    /* Add base register */
    libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

    /* Is the instruction post-indexed? */
    if (op2 == 3) {
        if (Rm != 0b11111) libarch_instruction_add_operand_register (instr, Rm, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
        else libarch_instruction_add_operand_immediate (instr, (Q == 0) ? 32 : 64, ARM64_IMMEDIATE_TYPE_INT, ARM64_IMMEDIATE_OPERAND_OPT_PREFER_DECIMAL);
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_load_store_memory_tags (instruction_t **instr)
{
    unsigned opc = select_bits ((*instr)->opcode, 22, 23);
    unsigned imm9 = select_bits ((*instr)->opcode, 12, 20);
    unsigned op2 = select_bits ((*instr)->opcode, 10, 11);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, opc);
    libarch_instruction_add_field (instr, imm9);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rt);

    uint32_t simm = sign_extend (imm9, 9) << 4;

    int opcode_table[][5] = {
        {0, 1, 0, ARM64_INSTRUCTION_STG, 1},
        {0, 2, 0, ARM64_INSTRUCTION_STG, 1},
        {0, 3, 0, ARM64_INSTRUCTION_STG, 1},
        {0, 0, 0, ARM64_INSTRUCTION_STZGM, 0},
        {1, 0, 0, ARM64_INSTRUCTION_LDG, 1},
        {1, 1, 0, ARM64_INSTRUCTION_STZG, 1},
        {1, 2, 0, ARM64_INSTRUCTION_STZG, 1},
        {1, 3, 0, ARM64_INSTRUCTION_STZG, 1},
        {2, 1, 0, ARM64_INSTRUCTION_ST2G, 1},
        {2, 2, 0, ARM64_INSTRUCTION_ST2G, 1},
        {2, 3, 0, ARM64_INSTRUCTION_ST2G, 1},
        {2, 0, 0, ARM64_INSTRUCTION_STGM, 0},
        {3, 1, 0, ARM64_INSTRUCTION_STZ2G, 1},
        {3, 2, 0, ARM64_INSTRUCTION_STZ2G, 1},
        {3, 3, 0, ARM64_INSTRUCTION_STZ2G, 1},
        {2, 0, 0, ARM64_INSTRUCTION_LDGM, 0}
    };

    for (int i = 0; i < sizeof (opcode_table) / sizeof (opcode_table[0]); i++) {
        if (opcode_table[i][0] == opc && opcode_table[i][1] == op2 && opcode_table[i][2] == imm9) {
            (*instr)->type = opcode_table[i][3];
            libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

            if (opcode_table[i][4] && simm)
                libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &simm, ARM64_IMMEDIATE_TYPE_UINT, ARM64_IMMEDIATE_OPERAND_OPT_NONE);
        }
    }


    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_load_store_exclusive_pair (instruction_t **instr)
{
    unsigned sz = select_bits ((*instr)->opcode, 30, 30);
    unsigned L = select_bits ((*instr)->opcode, 22, 22);
    unsigned Rs = select_bits ((*instr)->opcode, 16, 20);
    unsigned o0 = select_bits ((*instr)->opcode, 15, 15);
    unsigned Rt2 = select_bits ((*instr)->opcode, 10, 14);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, sz);
    libarch_instruction_add_field (instr, L);
    libarch_instruction_add_field (instr, Rs);
    libarch_instruction_add_field (instr, o0);
    libarch_instruction_add_field (instr, Rt2);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rt);

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sz == 1) _SET_64 (size, regs, len);
    else _SET_32 (size, regs, len);

    int opcode_table[][4] = {
        { 0, 0, ARM64_INSTRUCTION_STXP, 1 },
        { 0, 1, ARM64_INSTRUCTION_STLXP, 1 },
        { 1, 0, ARM64_INSTRUCTION_LDXP, 0  },
        { 1, 1, ARM64_INSTRUCTION_LDAXP, 0  },
    };

    for (int i = 0; i < sizeof (opcode_table) / sizeof (opcode_table[0]); i++) {
        if (opcode_table[i][0] == L && opcode_table[i][1] == o0) {
            (*instr)->type = opcode_table[i][2];

            /* The ST_ instructions have a 32-bit Rs register operand first */
            if (opcode_table[i][3])
                libarch_instruction_add_operand_register (instr, Rs, 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

            libarch_instruction_add_operand_register (instr, Rt, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_register (instr, Rt2, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
        }
    }

    /* Base register is always 64-bit */
    libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_load_store_exclusive_register (instruction_t **instr)
{
    unsigned size = select_bits ((*instr)->opcode, 30, 31);
    unsigned L = select_bits ((*instr)->opcode, 22, 22);
    unsigned Rs = select_bits ((*instr)->opcode, 16, 20);
    unsigned o0 = select_bits ((*instr)->opcode, 15, 15);
    unsigned Rt2 = select_bits ((*instr)->opcode, 10, 14);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, size);
    libarch_instruction_add_field (instr, L);
    libarch_instruction_add_field (instr, Rs);
    libarch_instruction_add_field (instr, o0);
    libarch_instruction_add_field (instr, Rt2);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rt);

    int opcode_table[][6] = {
        { 0, 0, 0, 1, 32, ARM64_INSTRUCTION_STXRB },
        { 0, 0, 1, 1, 32, ARM64_INSTRUCTION_STLXRB },

        { 0, 1, 0, -1, 32, ARM64_INSTRUCTION_LDXRB },
        { 0, 1, 1, -1, 32, ARM64_INSTRUCTION_LDAXRB },

        { 1, 0, 0, 1, 32, ARM64_INSTRUCTION_STXRH },
        { 1, 0, 1, 1, 32, ARM64_INSTRUCTION_STLXRH },

        { 1, 1, 0, -1, 32, ARM64_INSTRUCTION_LDXRH },
        { 1, 1, 1, -1, 32, ARM64_INSTRUCTION_LDAXRH },

        { 2, 0, 0, 1, 32, ARM64_INSTRUCTION_STXR },
        { 2, 0, 1, 1, 32, ARM64_INSTRUCTION_STLXR },
        { 3, 0, 0, 1, 64, ARM64_INSTRUCTION_STXR },
        { 3, 0, 1, 1, 64, ARM64_INSTRUCTION_STLXR },

        { 2, 1, 0, 1, 32, ARM64_INSTRUCTION_LDXR },
        { 2, 1, 1, 1, 32, ARM64_INSTRUCTION_LDAXR },
        { 3, 1, 0, 1, 64, ARM64_INSTRUCTION_LDXR },
        { 3, 1, 1, 1, 64, ARM64_INSTRUCTION_LDAXR },
    };

    for (int i = 0; i < sizeof (opcode_table) / sizeof (opcode_table[0]); i++) {
        if (opcode_table[i][0] == size && opcode_table[i][1] == L && opcode_table[i][2] == o0) {
            (*instr)->type = opcode_table[i][5];

            /* Does this instruction use the Rs register? */
            if (opcode_table[i][3])
                libarch_instruction_add_operand_register (instr, Rs, 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);

            libarch_instruction_add_operand_register (instr, Rt, opcode_table[i][4], ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');
        }
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_load_store_ordered (instruction_t **instr)
{
    unsigned size = select_bits ((*instr)->opcode, 30, 31);
    unsigned L = select_bits ((*instr)->opcode, 22, 22);
    unsigned Rs = select_bits ((*instr)->opcode, 16, 20);
    unsigned o0 = select_bits ((*instr)->opcode, 15, 15);
    unsigned Rt2 = select_bits ((*instr)->opcode, 10, 14);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, size);
    libarch_instruction_add_field (instr, L);
    libarch_instruction_add_field (instr, Rs);
    libarch_instruction_add_field (instr, o0);
    libarch_instruction_add_field (instr, Rt2);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rt);

    int opcode_table[][5] = {
        { 0, 0, 0, 32, ARM64_INSTRUCTION_STLLRB },
        { 0, 0, 1, 32, ARM64_INSTRUCTION_STLRB },

        { 0, 1, 0,  32, ARM64_INSTRUCTION_LDLARB },
        { 0, 1, 1,  32, ARM64_INSTRUCTION_LDARB },

        { 1, 0, 0, 32, ARM64_INSTRUCTION_STLLRH },
        { 1, 0, 1, 32, ARM64_INSTRUCTION_STLRH },

        { 1, 1, 0,  32, ARM64_INSTRUCTION_LDLARH },
        { 1, 1, 1,  32, ARM64_INSTRUCTION_LDARH },

        { 2, 0, 0, 32, ARM64_INSTRUCTION_STLLR },
        { 2, 0, 1, 32, ARM64_INSTRUCTION_STLR },
        { 3, 0, 0, 64, ARM64_INSTRUCTION_STLLR },
        { 3, 0, 1, 64, ARM64_INSTRUCTION_STLR },

        { 2, 1, 0, 32, ARM64_INSTRUCTION_LDLAR },
        { 2, 1, 1, 32, ARM64_INSTRUCTION_LDAR },
        { 3, 1, 0, 64, ARM64_INSTRUCTION_LDLAR },
        { 3, 1, 1, 64, ARM64_INSTRUCTION_LDAR },
    };

    for (int i = 0; i < sizeof (opcode_table) / sizeof (opcode_table[0]); i++) {
        if (opcode_table[i][0] == size && opcode_table[i][1] == L && opcode_table[i][2] == o0) {
            (*instr)->type = opcode_table[i][4];

            libarch_instruction_add_operand_register (instr, Rt, opcode_table[i][3], ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
            libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');
        }
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_load_register_literal (instruction_t **instr)
{
    unsigned opc = select_bits ((*instr)->opcode, 30, 31);
    unsigned V = select_bits ((*instr)->opcode, 26, 26);
    unsigned imm19 = select_bits ((*instr)->opcode, 5, 23);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, opc);
    libarch_instruction_add_field (instr, V);
    libarch_instruction_add_field (instr, imm19);
    libarch_instruction_add_field (instr, Rt);

    /* Extend the pc-relative immediate value */
    long label = (signed) sign_extend (imm19 << 2, 64) + (*instr)->addr;

    /* The PRFM (literal) instruction is handled differently to the others */
    if (opc == 3 && V == 0) {
        (*instr)->type = ARM64_INSTRUCTION_PRFM;

        unsigned type = select_bits (Rt, 3, 4);
        unsigned target = select_bits (Rt, 2, 1);
        unsigned policy = select_bits (Rt, 0, 0);

        /* Determine prefetch operation */
        int prefetch_op_table[][4] = {
            { 0, 0, 0, ARM64_PRFOP_PLDL1KEEP },
            { 0, 1, 0, ARM64_PRFOP_PLDL2KEEP },
            { 0, 2, 0, ARM64_PRFOP_PLDL3KEEP },
            { 0, 0, 1, ARM64_PRFOP_PLDL1STRM },
            { 0, 1, 1, ARM64_PRFOP_PLDL2STRM },
            { 0, 2, 1, ARM64_PRFOP_PLDL3STRM },
            { 1, 0, 0, ARM64_PRFOP_PLIL1KEEP },
            { 1, 1, 0, ARM64_PRFOP_PLIL2KEEP },
            { 1, 2, 0, ARM64_PRFOP_PLIL3KEEP },
            { 1, 0, 1, ARM64_PRFOP_PLIL1STRM },
            { 1, 1, 1, ARM64_PRFOP_PLIL2STRM },
            { 1, 2, 1, ARM64_PRFOP_PLIL3STRM },
            { 2, 0, 0, ARM64_PRFOP_PSTL1KEEP },
            { 2, 1, 0, ARM64_PRFOP_PSTL2KEEP },
            { 2, 2, 0, ARM64_PRFOP_PSTL3KEEP },
            { 2, 0, 1, ARM64_PRFOP_PSTL1STRM },
            { 2, 1, 1, ARM64_PRFOP_PSTL2STRM },
            { 2, 2, 1, ARM64_PRFOP_PSTL3STRM },
        };

        int prfop = -1;
        for (int i = 0; i < sizeof (prefetch_op_table) / sizeof (prefetch_op_table[0]); i++) {
            if (prefetch_op_table[i][0] == type && prefetch_op_table[i][1] == target && prefetch_op_table[i][2] == policy)
                prfop = prefetch_op_table[i][3];
        }

        /* If there was a prefetch op, add it as an extra operand */
        if (prfop >= 0) libarch_instruction_add_operand_extra (instr, ARM64_OPERAND_TYPE_PRFOP, prfop);
        else libarch_instruction_add_operand_immediate (instr, *(long *) &Rt, ARM64_IMMEDIATE_TYPE_UINT, ARM64_IMMEDIATE_OPERAND_OPT_NONE);

        libarch_instruction_add_operand_immediate (instr, *(long *) &label, ARM64_IMMEDIATE_TYPE_UINT, ARM64_IMMEDIATE_OPERAND_OPT_NONE);

        return LIBARCH_DECODE_STATUS_SUCCESS;
    }

    int opcode_table[][5] = {
        { 0, 0, 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDR },
        { 0, 1, 32, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDR },
        { 1, 0, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDR },
        { 1, 1, 64, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDR },

        { 2, 0, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDRSW },
        { 2, 1, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDR },
    };

    for (int i = 0; i < sizeof (opcode_table) / sizeof (opcode_table[0]); i++) {
        if (opcode_table[i][0] == opc && opcode_table[i][1] == V) {
            (*instr)->type = opcode_table[i][4];

            /* Add operands */
            libarch_instruction_add_operand_register (instr, Rt, opcode_table[i][2], opcode_table[i][3], ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_immediate (instr, *(long *) &label, ARM64_IMMEDIATE_TYPE_LONG, ARM64_IMMEDIATE_OPERAND_OPT_NONE);
        }
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
}

LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_load_store_register_pair (instruction_t **instr)
{
    unsigned opc = select_bits ((*instr)->opcode, 30, 31);
    unsigned V = select_bits ((*instr)->opcode, 26, 26);
    unsigned L = select_bits ((*instr)->opcode, 22, 22);
    unsigned imm7 = select_bits ((*instr)->opcode, 15, 21);
    unsigned Rt2 = select_bits ((*instr)->opcode, 10, 14);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    unsigned select = select_bits ((*instr)->opcode, 23, 25);

    // tmp
    libarch_instruction_add_field (instr, select);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, opc);
    libarch_instruction_add_field (instr, V);
    libarch_instruction_add_field (instr, L);
    libarch_instruction_add_field (instr, imm7);
    libarch_instruction_add_field (instr, Rt2);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rt);

    typedef struct {
        unsigned select, opc, V, L;
        int width;
        int simd_fp;
        arm64_instr_t type;
    } opcode;

    opcode opcode_table[] = {
        /* Load/Store no-allocate pair (offset) */
        { 0, 0, 0, 0, 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_STNP },
        { 0, 0, 0, 1, 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDNP },
        { 0, 0, 1, 0, 32, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_STNP },
        { 0, 0, 1, 1, 32, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDNP },
        { 0, 1, 1, 0, 64, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_STNP },
        { 0, 1, 1, 1, 64, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDNP },
        { 0, 2, 0, 0, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_STNP },
        { 0, 2, 0, 1, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDNP },

        /* Load/Store register pair (post-indexed) */
        { 1, 0, 0, 0, 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_STP },
        { 1, 0, 0, 1, 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDP },
        { 1, 0, 1, 0, 32, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_STP },
        { 1, 0, 1, 1, 32, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDP },
        { 1, 1, 1, 0, 64, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_STP },
        { 1, 1, 1, 1, 64, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDP },
        { 1, 2, 1, 0, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_STP },
        { 1, 2, 1, 1, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDP },
        { 1, 1, 0, 1, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDPSW },

        /* Load/Store register pair (offset) */
        { 2, 0, 0, 0, 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_STP },
        { 2, 0, 0, 1, 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDP },
        { 2, 0, 1, 0, 32, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_STP },
        { 2, 0, 1, 1, 32, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDP },
        { 2, 1, 0, 0, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_STP },
        { 2, 1, 0, 1, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDP },
        { 2, 1, 1, 0, 64, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_STP },
        { 2, 1, 1, 1, 64, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDP },
        { 2, 2, 1, 0, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_STP },
        { 2, 2, 1, 1, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDP },
        { 2, 1, 0, 1, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDPSW },

        /* Load/Store register pair (pre-indexed) */
        { 3, 0, 0, 0, 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_STP },
        { 3, 0, 0, 1, 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDP },
        { 3, 0, 1, 0, 32, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_STP },
        { 3, 0, 1, 1, 32, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDP },

        { 3, 2, 0, 0, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_STP },
        { 3, 2, 0, 1, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDP },
        { 3, 1, 1, 0, 64, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_STP },
        { 3, 1, 1, 1, 64, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDP },

        { 3, 2, 1, 0, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_STP },
        { 3, 2, 1, 1, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, ARM64_INSTRUCTION_LDP },
        { 3, 1, 0, 1, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_INSTRUCTION_LDPSW },

    };
    int opc_len = sizeof (opcode_table) / sizeof (opcode*);

    for (int i = 0; i < opc_len; i++) {
        if (opcode_table[i].select == select && opcode_table[i].opc == opc && opcode_table[i].V == V && opcode_table[i].L == L) {
            (*instr)->type = opcode_table[i].type;

            /* Common register operands */
            libarch_instruction_add_operand_register (instr, Rt, opcode_table[i].width, opcode_table[i].simd_fp, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
            libarch_instruction_add_operand_register (instr, Rt2, opcode_table[i].width, opcode_table[i].simd_fp, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);

            /* Fixup immediate value */
            unsigned int scale, imm;
            scale = (opcode_table[i].V == 0) ? 2 + (opc >> 1) : 2 + opc;
            imm = sign_extend (imm7, 7) << scale;

            /* Load/Store no-allocated pair (offset) */
            if (opcode_table[i].select == 0 || opcode_table[i].select == 2) {
                libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', (imm7 >= 1) ? NULL : ']');

                if (imm7) {
                    libarch_instruction_add_operand_immediate_with_fix (instr, *(int *) &imm, ARM64_IMMEDIATE_TYPE_INT, NULL, ']');
                    break;
                }

            /* Load/Store register pair (post-indexed) */
            } else if (opcode_table[i].select == 1) {
                libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');
                libarch_instruction_add_operand_immediate (instr, *(int *) &imm, ARM64_IMMEDIATE_TYPE_INT, ARM64_IMMEDIATE_OPERAND_OPT_NONE);
            
            /* Load/Store register pair (pre-indexed) */
            } else if (opcode_table[i].select == 3) {
                libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', NULL);
                libarch_instruction_add_operand_immediate_with_fix_extra (instr, *(int *) &imm, ARM64_IMMEDIATE_TYPE_INT, NULL, ']');
            }

            
        }
    }

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
    //printf ("load and store: op0: %d, op1: %d, op2: %d, op3: %d, op4: %d\n",
    //    op0, op1, op2, op3, op4);

    /**
     *  Load and Store Multiple structures
     *  Load and Store Single structures
     *  Load and Store Memory Tags
     *  Load and Store Exclusive
     *  LDAPR STLR
     *  Load and Store Literal
     *  Load and store Register Pair
     *  
     *  Load and Store Register
     *      Atomic memory
     *      Load and Store Register Offset 
     *      Load and Store PAC
     * 
     *  Load and Store Register
     */

    if ((op0 & ~4) == 0 && op1 == 1 && (op2 == 0 || op2 == 1) && (op3 & ~0x1f) == 0) {
        if (decode_advanced_simd_load_store_multiple_structures (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_ADVANCED_SIMD_LOAD_STORE_MULT_STRUCT;

    } else if ((op0 & ~4) == 0 && op1 == 1 && (op2 == 2 || op2 == 3)) {
        if (decode_advanced_simd_load_store_single_structure (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_ADVANCED_SIMD_LOAD_STORE_SINGLE_STRUCT;

    } else if (op0 == 13 && op1 == 0 && (op2 >> 1) == 1 && (op3 >> 5) == 1) {
        if (decode_load_store_memory_tags (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_LOAD_STORE_MEMORY_TAGS;

    } else if ((op0 & ~12) == 0 && op1 == 0 && op2 == 1 && (op3 >> 5) == 0) {
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

    } else if ((op0 & ~12) == 1 && (op2 >> 1) == 0) {
        if (decode_load_register_literal (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_LOAD_REGISTER_LITERAL;

    } else if ((op0 & ~12) == 2) {
        if (decode_load_store_register_pair (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_LOAD_REGISTER_PAIR;

    } else {
        /**
         *  The following instruction subgroups have not been implemented:
         *  - Compare and Swap
         *  - LDAPR/STLR (Unscaled Immediate)
         *  - Memory Copy and Memory Set
         */
        printf ("not implemented\n");
    }

    return LIBARCH_RETURN_SUCCESS;
}