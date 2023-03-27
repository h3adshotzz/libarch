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

    if (size == 64)
        libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL);

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
    for (int i = Rt; i < Rt + reg_count; i++)
        libarch_instruction_add_operand_register (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT);

    libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL);  

    /* Is the instruction post-indexed? */
    if (op2 == 1) {
        if (Rm != 0b11111) libarch_instruction_add_operand_register (instr, Rm, 64, ARM64_REGISTER_TYPE_GENERAL);
        else libarch_instruction_add_operand_immediate (instr, (Q == 0) ? 32 : 64, ARM64_IMMEDIATE_TYPE_INT);
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

    /**
     * 
     *  op2 = 2     L = 0   R = 0   opcode == 000                           -   ST1 8-bit
     *  op2 = 2     L = 0   R = 0   opcode == 010           size == x0      -   ST1 16-bit
     *  op2 = 2     L = 0   R = 0   opcode == 100           size == 00      -   ST1 32-bit 
     *  op2 = 2     L = 0   R = 0   opcode == 100   S == 0  size == 01      -   ST1 64-bit
     *  op2 = 3     
     * 
     */

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
        libarch_instruction_add_operand_register (instr, i, 128, ARM64_REGISTER_TYPE_FLOATING_POINT);

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

    /* Is the instruction post-indexed? */
    if (op2 == 3) {
        if (Rm != 0b11111) libarch_instruction_add_operand_register (instr, Rm, 64, ARM64_REGISTER_TYPE_GENERAL);
        else libarch_instruction_add_operand_immediate (instr, (Q == 0) ? 32 : 64, ARM64_IMMEDIATE_TYPE_INT);
    }

    return LIBARCH_RETURN_SUCCESS;
}


libarch_return_t
disass_load_and_store_instruction (instruction_t *instr)
{
    unsigned op0 = select_bits (instr->opcode, 28, 31);
    unsigned op1 = select_bits (instr->opcode, 26, 26);
    unsigned op2 = select_bits (instr->opcode, 23, 24);
    unsigned op3 = select_bits (instr->opcode, 16, 21);
    unsigned op4 = select_bits (instr->opcode, 10, 11);

    if (op0 == 0 || op0 == 4) {
        if (op1 == 0 && op2 == 0 && (op3 >> 5) == 1)
            decode_compare_and_swap_pair (&instr);
        else if (op1 == 1 && (op2 == 0 || op2 == 1) && (op3 == 0 || (op3 >> 5) == 0)) {
            if (op2 == 0) printf ("Advanced SIMD load/store multiple structures\n");
            else printf ("Advanced SIMD load/store multiple structures (post-indexed)\n");

            decode_advanced_simd_load_store_multiple_structures (&instr);

        } else if (op1 == 1 && (op2 == 2 || op2 == 3)) {
            if (op2 == 2) printf ("Advanced SIMD load/store single structure\n");
            else printf ("Advanced SIMD load/store siingle structure (post-indexed)\n");

            decode_advanced_simd_load_store_single_structure (&instr);
        }

    } else if (op0 == 13) {
        if (op1 == 0 && (op2 >> 1) == 1 && (op3 >> 5) == 1)
            printf ("Load/store memory tags\n");
        else
            printf ("unallocated load-store instruction\n");

    } else if (op0 == 8 || op0 == 12) {
        if (op1 == 0 && op2 == 0 && (op3 >> 5) == 1)
            printf ("Load/store exclusive pair\n");
        else
            printf ("unallocated load-store instruction\n");
    } else {
        printf ("*not implemented*\n");
    }
    return LIBARCH_RETURN_SUCCESS;
}