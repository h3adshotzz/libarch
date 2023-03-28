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

static libarch_return_t
decode_compare_and_swap_pair (instruction_t **instr)
{
    unsigned sz = select_bits ((*instr)->opcode, 30, 30);
    unsigned L = select_bits ((*instr)->opcode, 22, 22);
    unsigned Rs = select_bits ((*instr)->opcode, 16, 20);
    unsigned o0 = select_bits ((*instr)->opcode, 15, 15);
    unsigned Rt2 = select_bits ((*instr)->opcode, 10, 14);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Unallocated */
    if (Rt2 != 0b11111) return LIBARCH_RETURN_VOID;

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sz == 1) _SET_64 (size, regs, len);
    else _SET_32 (size, regs, len);

    // CASP 
    if (L == 0 && o0 == 0) 
        (*instr)->type = ARM64_INSTRUCTION_CASP;
    // CASPA
    else if (L == 1 && o0 == 0)
        (*instr)->type = ARM64_INSTRUCTION_CASPA;
    // CASPAL
    else if (L == 1 && o0 == 1)
        (*instr)->type = ARM64_INSTRUCTION_CASPAL;
    // CASPL
    else if (L == 0 && o0 == 1)
        (*instr)->type = ARM64_INSTRUCTION_CASPL;

    /* Add the operands */
    libarch_instruction_add_operand_register (instr, Rs, size, ARM64_REGISTER_TYPE_GENERAL);
    libarch_instruction_add_operand_register (instr, Rs + 1, size, ARM64_REGISTER_TYPE_GENERAL);
    libarch_instruction_add_operand_register (instr, Rt, size, ARM64_REGISTER_TYPE_GENERAL);
    libarch_instruction_add_operand_register (instr, Rt + 1, size, ARM64_REGISTER_TYPE_GENERAL);

    libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
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
    arm64_vec_specifier_t spec = 0;
    if (size == 0 && Q == 0) spec = ARM64_VEC_ARRANGEMENT_8B; 
    else if (size == 0 && Q == 1) spec = ARM64_VEC_ARRANGEMENT_16B;
    else if (size == 1 && Q == 0) spec = ARM64_VEC_ARRANGEMENT_4H;
    else if (size == 1 && Q == 1) spec = ARM64_VEC_ARRANGEMENT_8H;
    else if (size == 2 && Q == 0) spec = ARM64_VEC_ARRANGEMENT_2S;
    else if (size == 2 && Q == 1) spec = ARM64_VEC_ARRANGEMENT_4S;
    else if (size == 3 && Q == 1) spec = ARM64_VEC_ARRANGEMENT_2D;
    
    (*instr)->spec = spec;

    /* Set the register operands */
    if (reg_count == 1) {
        libarch_instruction_add_operand_register_with_fix (instr, Rt, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, '{', '}');
    } else {
        for (int i = Rt; i < Rt + reg_count; i++) {
            if (i == Rt) libarch_instruction_add_operand_register_with_fix (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, '{', NULL);
            else if (i == Rt + reg_count - 1) libarch_instruction_add_operand_register_with_fix (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, NULL, '}');
            else libarch_instruction_add_operand_register (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT);
        }
    }

    libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');  

    /* Is the instruction post-indexed? */
    if (op2 == 1) {
        if (Rm != 0b11111) libarch_instruction_add_operand_register (instr, Rm, 64, ARM64_REGISTER_TYPE_GENERAL);
        else {
            int imm = 0;

            if (reg_count == 1) imm = (Q == 0) ? 8 : 16;
            else if (reg_count == 2) imm = (Q == 0) ? 16 : 32;
            else if (reg_count == 3) imm = (Q == 0) ? 24 : 48;
            else if (reg_count == 4) imm = (Q == 0) ? 32 : 64;

            libarch_instruction_add_operand_immediate (instr, imm, ARM64_IMMEDIATE_TYPE_INT | ARM64_IMMEDIATE_FLAG_OUTPUT_DECIMAL);
        }
    }

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
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

    unsigned elem = (((opcode & 1) << 1) | R) + 1;
    int index = 0;

    int replicate = 0;
    arm64_vec_specifier_t spec = 0;
    switch ((opcode >> 1))
    {
        case 3: replicate = 1; break;
        case 0:
            index = (Q << 3) | (S << 2) | size;
            spec = ARM64_VEC_ARRANGEMENT_B;
            break;

        case 1:
            index = (Q << 2) | (S << 1) | (size >> 1);
            spec = ARM64_VEC_ARRANGEMENT_H;
            break;
        
        case 2:
            if ((size & 1) == 0) {
                index = (Q << 1) | S;
                spec = ARM64_VEC_ARRANGEMENT_S;
            } else {
                index = Q;
                spec = ARM64_VEC_ARRANGEMENT_D;
            }
        default: LIBARCH_RETURN_VOID;
    }

    if (replicate) (*instr)->type = (ARM64_INSTRUCTION_LD1R - 1) + ((elem * 2) - 1);
    else if (L == 0) (*instr)->type = (ARM64_INSTRUCTION_ST1 - 1) + elem;
    else if (L == 1) (*instr)->type = (ARM64_INSTRUCTION_LD1 - 1) + ((elem * 2) - 1);

    /* Set the register operands */
    for (int i = Rt; i < Rt + elem; i++) {
        if (i == Rt) libarch_instruction_add_operand_register_with_fix (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, '{', NULL);
        else if (i == Rt + elem - 1) libarch_instruction_add_operand_register_with_fix (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT, NULL, '}');
        else libarch_instruction_add_operand_register (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT);

        if (replicate) {
            if (size == 0) spec = (Q == 0) ? ARM64_VEC_ARRANGEMENT_8B : ARM64_VEC_ARRANGEMENT_16B;
            else if (size == 1) spec = (Q == 0) ? ARM64_VEC_ARRANGEMENT_4H : ARM64_VEC_ARRANGEMENT_8H;
            else if (size == 2) spec = (Q == 0) ? ARM64_VEC_ARRANGEMENT_2S : ARM64_VEC_ARRANGEMENT_4S;
            else if (size == 3) spec = (Q == 0) ? ARM64_VEC_ARRANGEMENT_D : ARM64_VEC_ARRANGEMENT_2D;
        }
        (*instr)->spec = spec;
    }

    /* Add the index */
    if (!replicate)
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &index, ARM64_IMMEDIATE_TYPE_UINT);

    /* Add base register */
    libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL);

    /* Is the instruction post-indexed? */
    if (op2 == 3) {
        if (Rm != 0b11111) libarch_instruction_add_operand_register (instr, Rm, 64, ARM64_REGISTER_TYPE_GENERAL);
        else libarch_instruction_add_operand_immediate (instr, (Q == 0) ? 32 : 64, ARM64_IMMEDIATE_TYPE_INT);
    }

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_load_store_memory_tags (instruction_t **instr)
{
    unsigned opc = select_bits ((*instr)->opcode, 22, 23);
    unsigned imm9 = select_bits ((*instr)->opcode, 12, 20);
    unsigned op2 = select_bits ((*instr)->opcode, 10, 11);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Unallocated */
    if (opc == 2 && imm9 != 0 && op2 == 0) return LIBARCH_RETURN_VOID;
    if (opc == 3 && imm9 != 0 && op2 == 0) return LIBARCH_RETURN_VOID;

    unsigned int simm = sign_extend (imm9, 9) << 4;

    // STG
    if (opc == 0 && (op2 >= 1 && op2 <= 3)) {
        (*instr)->type = ARM64_INSTRUCTION_STG;

        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

        if (simm)
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &simm, ARM64_IMMEDIATE_TYPE_UINT);

    // STZGM
    } else if (opc == 0 && imm9 == 0 && op2 == 0) {
        (*instr)->type = ARM64_INSTRUCTION_STZGM;
        
        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    // LDG
    } else if (opc == 1 && op2 == 0) {
        (*instr)->type = ARM64_INSTRUCTION_LDG;

        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &simm, ARM64_IMMEDIATE_TYPE_UINT);

    // STZG
    } else if (opc == 1 && (op2 >= 1 && op2 <= 3)) {
        (*instr)->type = ARM64_INSTRUCTION_STZG;

        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');
        
        if (simm)
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &simm, ARM64_IMMEDIATE_TYPE_UINT);

    // ST2G
    } else if (opc == 2 && (op2 >= 1 && op2 <= 3)) {
        (*instr)->type = ARM64_INSTRUCTION_ST2G;

        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');
        
        if (simm)
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &simm, ARM64_IMMEDIATE_TYPE_UINT);

    // STGM
    } else if (opc == 2 && imm9 == 0 && op2 == 0) {
        (*instr)->type = ARM64_INSTRUCTION_STGM;

        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    // STZ2G
    } else if (opc == 3 && (op2 >= 1 && op2 <= 3)) {
        (*instr)->type = ARM64_INSTRUCTION_STZ2G;

        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');
        
        if (simm)
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &simm, ARM64_IMMEDIATE_TYPE_UINT);

    // LDGM
    } else if (opc == 2 && imm9 == 0 && op2 == 0) {
        (*instr)->type = ARM64_INSTRUCTION_LDGM;

        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');
    }

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_load_store_exclusive_pair (instruction_t **instr)
{
    unsigned sz = select_bits ((*instr)->opcode, 30, 30);
    unsigned L = select_bits ((*instr)->opcode, 22, 22);
    unsigned Rs = select_bits ((*instr)->opcode, 16, 20);
    unsigned o0 = select_bits ((*instr)->opcode, 15, 15);
    unsigned Rt2 = select_bits ((*instr)->opcode, 10, 14);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sz == 1) _SET_64 (size, regs, len);
    else _SET_32 (size, regs, len);

    // STXP / STLXP
    if (L == 0) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_STXP;
        else (*instr)->type = ARM64_INSTRUCTION_STLXP;

        // The first register is always 32-bit
        libarch_instruction_add_operand_register (instr, Rs, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rt, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rt2, size, ARM64_REGISTER_TYPE_GENERAL);

    // LDXP / LDAXP
    } else if (L == 1 && o0 == 0) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_LDXP;
        else (*instr)->type = ARM64_INSTRUCTION_LDAXP;

        libarch_instruction_add_operand_register (instr, Rt, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rt2, size, ARM64_REGISTER_TYPE_GENERAL);
    }

    // The base register is always 64-bit.
    libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_load_store_exclusive_register (instruction_t **instr)
{
    unsigned size = select_bits ((*instr)->opcode, 30, 31);
    unsigned L = select_bits ((*instr)->opcode, 22, 22);
    unsigned Rs = select_bits ((*instr)->opcode, 16, 20);
    unsigned o0 = select_bits ((*instr)->opcode, 15, 15);
    unsigned Rt2 = select_bits ((*instr)->opcode, 10, 14);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    // STXRB / STLXRB
    if (size == 0 && L == 0) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_STXRB;
        else (*instr)->type = ARM64_INSTRUCTION_STLXRB;

        // The first two registers are always 32-bit
        libarch_instruction_add_operand_register (instr, Rs, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    // LDXRB / LDAXRB
    } else if (size == 0 && L == 1) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_LDXRB;
        else (*instr)->type = ARM64_INSTRUCTION_LDAXRB;

        // The first register is always 32-bit
        libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    // STXRH / STLXRH
    } else if (size == 1 && L == 0) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_STXRH;
        else (*instr)->type = ARM64_INSTRUCTION_STLXRH;

        // The first two registers are always 32-bit
        libarch_instruction_add_operand_register (instr, Rs, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    // LDXRH / LDAXRH
    } else if (size == 1 && L == 1) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_LDXRH;
        else (*instr)->type = ARM64_INSTRUCTION_LDAXRH;

        // The first register is always 32-bit
        libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    // STXR / STXLR
    } else if ((size == 2 || size == 3) && L == 0) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_STXR;
        else (*instr)->type = ARM64_INSTRUCTION_STLXR;

        // The first register is always 32-bit
        libarch_instruction_add_operand_register (instr, Rs, 32, ARM64_REGISTER_TYPE_GENERAL);

        if (size == 2) libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_GENERAL);
        else libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);

        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    // LDXR / LDAXR
    } else if ((size == 2 || size == 3) && L == 1) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_LDXR;
        else (*instr)->type = ARM64_INSTRUCTION_LDAXR;

        // The first register is always 32-bit
        libarch_instruction_add_operand_register (instr, Rs, 32, ARM64_REGISTER_TYPE_GENERAL);

        if (size == 2) libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_GENERAL);
        else libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);

        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    }

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_load_store_ordered (instruction_t **instr)
{
    unsigned size = select_bits ((*instr)->opcode, 30, 31);
    unsigned L = select_bits ((*instr)->opcode, 22, 22);
    unsigned Rs = select_bits ((*instr)->opcode, 16, 20);
    unsigned o0 = select_bits ((*instr)->opcode, 15, 15);
    unsigned Rt2 = select_bits ((*instr)->opcode, 10, 14);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    // STLLRB / STLRB
    if (size == 0 && L == 0) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_STLLRB;
        else (*instr)->type = ARM64_INSTRUCTION_STLRB;

        // First register is 32-bit
        libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

        
    // LDLARB / LDARB
    } else if (size == 0 && L == 1) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_LDLARB;
        else (*instr)->type = ARM64_INSTRUCTION_LDARB;

        // First register is 32-bit
        libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    // STLLRH / STLRH
    } else if (size == 1 && L == 0) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_STLLRH;
        else (*instr)->type = ARM64_INSTRUCTION_STLRH;

        // First register is 32-bit
        libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    // LDLARH / LDARH
    } else if (size == 1 && L == 1) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_LDLARH;
        else (*instr)->type = ARM64_INSTRUCTION_LDARH;

        // First register is 32-bit
        libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    // STLLR / STLR
    } else if ((size == 2 || size == 3) && L == 0) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_STLLR;
        else (*instr)->type = ARM64_INSTRUCTION_STLR;

        // First register is 32-bit
        libarch_instruction_add_operand_register (instr, Rt, (size == 2) ? 32 : 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    // LDLAR / LDAR
    } else if ((size == 2 || size == 3) && L == 1) {
        if (o0 == 0) (*instr)->type = ARM64_INSTRUCTION_LDLAR;
        else (*instr)->type = ARM64_INSTRUCTION_LDAR;

        // First register is 32-bit
        libarch_instruction_add_operand_register (instr, Rt, (size == 2) ? 32 : 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register_with_fix (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, '[', ']');

    }
    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_load_register_literal (instruction_t **instr)
{
    unsigned opc = select_bits ((*instr)->opcode, 30, 31);
    unsigned V = select_bits ((*instr)->opcode, 26, 26);
    unsigned imm19 = select_bits ((*instr)->opcode, 5, 23);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Extend the pc-relative immediate value */
    long label = (signed) sign_extend (imm19 << 2, 64) + (*instr)->addr;

    // LDR (literal) - 32-bit
    if (opc == 0 && V == 0) {
        (*instr)->type = ARM64_INSTRUCTION_LDR;

        libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, *(long *) &label, ARM64_IMMEDIATE_TYPE_LONG);

    // LDR (literal, SIMD & FP) - 32-bit
    } else if (opc == 0 && V == 1) {
        (*instr)->type = ARM64_INSTRUCTION_LDR;

        libarch_instruction_add_operand_register (instr, Rt, 32, ARM64_REGISTER_TYPE_FLOATING_POINT);
        libarch_instruction_add_operand_immediate (instr, *(long *) &label, ARM64_IMMEDIATE_TYPE_LONG);

    // LDR (literal) - 64-bit
    } else if (opc == 1 && V == 0) {
        (*instr)->type = ARM64_INSTRUCTION_LDR;

        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, *(long *) &label, ARM64_IMMEDIATE_TYPE_LONG);

    // LDR (literal, SIMD & FP) - 64-bit
    } else if (opc == 1 && V == 1) {
        (*instr)->type = ARM64_INSTRUCTION_LDR;

        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_FLOATING_POINT);
        libarch_instruction_add_operand_immediate (instr, *(long *) &label, ARM64_IMMEDIATE_TYPE_LONG);

    // LDRSW (literal)
    } else if (opc == 2 && V == 0) {
        (*instr)->type = ARM64_INSTRUCTION_LDRSW;

        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, *(long *) &label, ARM64_IMMEDIATE_TYPE_LONG);

    // LDR (literal, SIMD & FP) - 128-bit
    } else if (opc == 2 && V == 1) {
        (*instr)->type = ARM64_INSTRUCTION_LDR;

        libarch_instruction_add_operand_register (instr, Rt, 128, ARM64_REGISTER_TYPE_FLOATING_POINT);
        libarch_instruction_add_operand_immediate (instr, *(long *) &label, ARM64_IMMEDIATE_TYPE_LONG);

    // PRFM (literal)
    } else if (opc == 3 && V == 0) {
        (*instr)->type = ARM64_INSTRUCTION_PRFM;

    }
}



libarch_return_t
disass_load_and_store_instruction (instruction_t *instr)
{
    unsigned op0 = select_bits (instr->opcode, 28, 31);
    unsigned op1 = select_bits (instr->opcode, 26, 26);
    unsigned op2 = select_bits (instr->opcode, 23, 24);
    unsigned op3 = select_bits (instr->opcode, 16, 21);
    unsigned op4 = select_bits (instr->opcode, 10, 11);

    if (op0 == 0 && op1 == 0 && op2 == 0 && (op3 >> 5) == 1) {
        printf ("Compare and swap pair\n");
        decode_compare_and_swap_pair (&instr);

    } else if (((op0 == 4 || op0 == 0) && op1 == 1 && op2 == 0 && op3 == 0) ||
               ((op0 == 4 || op0 == 0) && op1 == 1 && op2 == 1 && (op3 >> 5) == 0)) {
        printf ("Advanced SIMD load/store multiple structures\n");
        decode_advanced_simd_load_store_multiple_structures (&instr);

    } else if (((op0 == 4 || op0 == 0) && op1 == 1 && op2 == 2 && (op3 == 0 || (op3 >> 5) == 1)) ||
               ((op0 == 4 || op0 == 0) && op1 == 1 == op2 == 3)) {
        printf ("Advanced SIMD load/store single structure\n");
        decode_advanced_simd_load_store_single_structure (&instr);

    } else if (op0 == 13 && op1 == 0 && (op2 == 2 || op2 == 3) && (op3 >> 5) == 1) {
        printf ("Load/store memory tags\n");
        decode_load_store_memory_tags (&instr);

    } else if (((op0 >> 2) == 2 || (op0 >> 2) == 3) && op1 == 0 && op2 == 0 && (op3 >> 5) == 1) {
        printf ("Load/store exclusive pair\n");
        decode_load_store_exclusive_pair (&instr);

    } else if (((op0 >> 2) >= 0 || (op0 >> 2) <= 3) && op1 == 0 && op2 == 0 && (op3 >> 5) == 0) {
        printf ("Load/store exclusive register\n");
        decode_load_store_exclusive_register (&instr);

    } else if (((op0 >> 2) >= 0 || (op0 >> 2) <= 3) && op1 == 0 && op2 == 1 && (op3 >> 5) == 0) {
        printf ("Load/store ordered\n");
        decode_load_store_ordered (&instr);

    } else if (((op0 >> 2) >= 0 || (op0 >> 2) <= 3) && op1 == 0 && op2 == 1 && (op3 >> 5) == 1) {
        printf ("Compare and swap\n");
        // Not Implemented Yet

    } else if (((op0 >> 2) >= 1 || (op0 >> 2) <= 3) && op1 == 0 && (op2 == 2 || op2 == 3) && (op3 >> 5) == 0 && op4 == 0) {
        printf ("LDAPR/STLR (Unscaled immediate)\n");
        // Not Implemented Yet

    } else if (((op0 >> 2) >= 1 || (op0 >> 2) <= 3) && (op2 == 0 || op2 == 1)) {
        printf ("Load Register (literal)\n");
        decode_load_register_literal (&instr);

    } else {
        printf ("not implemented\n");
    }


    return LIBARCH_RETURN_SUCCESS;
}