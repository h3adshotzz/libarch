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
    unsigned sf = select_bits ((*instr)->opcode, 31, 31);
    unsigned op = select_bits ((*instr)->opcode, 30, 30);
    unsigned S = select_bits ((*instr)->opcode, 29, 29);
    unsigned o2 = select_bits ((*instr)->opcode, 22, 22);
    unsigned uimm6 = select_bits ((*instr)->opcode, 16, 21);
    unsigned op3 = select_bits ((*instr)->opcode, 14, 15);
    unsigned uimm4 = select_bits ((*instr)->opcode, 10, 13);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, sf);
    libarch_instruction_add_field (instr, op);
    libarch_instruction_add_field (instr, S);
    libarch_instruction_add_field (instr, o2);
    libarch_instruction_add_field (instr, uimm6);
    libarch_instruction_add_field (instr, op3);
    libarch_instruction_add_field (instr, uimm4);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rd);

    /* Add common operands */
    libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
    libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
    libarch_instruction_add_operand_immediate (instr, uimm6, ARM64_IMMEDIATE_TYPE_ULONG);
    libarch_instruction_add_operand_immediate (instr, uimm4, ARM64_IMMEDIATE_TYPE_ULONG);

    /* ADDG */
    if (sf == 1 && op == 0 && S == 0 && o2 == 0) {
        (*instr)->type = ARM64_INSTRUCTION_ADDG;

    /* SUBG */
    } else if (sf == 1 && op == 1 && S == 0 && o2 == 0) {
        (*instr)->type = ARM64_INSTRUCTION_SUBG;

    } else {
        return LIBARCH_DECODE_STATUS_SOFT_FAIL;
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
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
    unsigned sf = select_bits ((*instr)->opcode, 31, 31);
    unsigned opc = select_bits ((*instr)->opcode, 29, 30);
    unsigned hw = select_bits ((*instr)->opcode, 21, 22);
    unsigned imm16 = select_bits ((*instr)->opcode, 5, 20);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, sf);
    libarch_instruction_add_field (instr, opc);
    libarch_instruction_add_field (instr, hw);
    libarch_instruction_add_field (instr, imm16);
    libarch_instruction_add_field (instr, Rd);

    /* Calculate shift */
    uint64_t shift = hw << 4;

    /* Determine instruction size */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sf == 0 && (hw >> 1) == 0) _SET_32 (size, regs, len);
    else _SET_64 (size, regs, len);

    if (opc == 0) {

        /* Add common operands */
        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

        /* MOV (inverted wide immediate) */
        if (!(is_zero (imm16) && hw != 0)) {
            (*instr)->type = ARM64_INSTRUCTION_MOV;

            /* Calculate immediate value */
            int imm_type = (size == 64) ? ARM64_IMMEDIATE_TYPE_LONG : ARM64_IMMEDIATE_TYPE_INT;
            long imm = ~((long) imm16 << shift);

            libarch_instruction_add_operand_immediate (instr, 
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *) &imm : *(int *) &imm,
                imm_type);

        /* MOVN */
        } else {
            (*instr)->type = ARM64_INSTRUCTION_MOVN;
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imm16, ARM64_IMMEDIATE_TYPE_UINT);

            /* Add the left-shift if present */
            if (shift) libarch_instruction_add_operand_shift (instr, shift, ARM64_SHIFT_TYPE_LSL);
        }
    } else if (opc == 2) {

        /* Add common operands */
        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

        /* MOV (wide immediate) */
        if (!is_zero (imm16) && hw != 0b00) {
            (*instr)->type = ARM64_INSTRUCTION_MOV;

            /* Calculate immediate value */
            int imm_type = (size == 64) ? ARM64_IMMEDIATE_TYPE_LONG : ARM64_IMMEDIATE_TYPE_INT;
            long imm = ~((long) imm16 << shift);

            libarch_instruction_add_operand_immediate (instr, 
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *) &imm : *(int *) &imm,
                imm_type);

        /* MOVZ */
        } else {
            (*instr)->type = ARM64_INSTRUCTION_MOVZ;
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imm16, ARM64_IMMEDIATE_TYPE_UINT);

            /* Add the left-shift if present */
            if (shift) libarch_instruction_add_operand_shift (instr, shift, ARM64_SHIFT_TYPE_LSL);
        }
    } else if (opc == 3) {

        /* MOVK */
        (*instr)->type = ARM64_INSTRUCTION_MOVK;

        /* Add operands */
        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imm16, ARM64_IMMEDIATE_TYPE_UINT);

        /* Add the left-shift if present */
        if (shift) libarch_instruction_add_operand_shift (instr, shift, ARM64_SHIFT_TYPE_LSL);
    } 
    
    return ((*instr)->type) ? LIBARCH_DECODE_STATUS_SUCCESS : LIBARCH_DECODE_STATUS_SOFT_FAIL;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_bitfield (instruction_t **instr)
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

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sf == 1) _SET_64 (size, regs, len);
    else _SET_32 (size, regs, len);

    if (opc == 0) {

        /* Add common operands */
        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

        /* ADR */
        if ((sf == 0 && imms == 0b011111) || (sf == 1 && imms == 0b111111)) {
            (*instr)->type = ARM64_INSTRUCTION_ADR;
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &immr, ARM64_IMMEDIATE_TYPE_UINT);

        /* SXTB / SXTH / SXTW */
        } else if (immr == 0 && (imms == 0b000111 || imms == 0b001111 || imms == 0b011111)) {

            /* Work out which one it is */
            if (imms == 0b000111) (*instr)->type = ARM64_INSTRUCTION_SXTB;
            else if (imms == 0b001111) (*instr)->type = ARM64_INSTRUCTION_SXTH;
            else if (imms == 0b011111) (*instr)->type = ARM64_INSTRUCTION_SXTW;
        
        /* SBFX / SBFIZ */
        } else if (imms < immr || BFXPreferred (sf, (opc >> 1), imms, immr)) {

            unsigned lsb, width;

            /* Choose the correct SBF_ instruction */
            if (BFXPreferred (sf, (opc >> 1), imms, immr)) {
                (*instr)->type = ARM64_INSTRUCTION_SBFX;
                lsb = immr;
                width = imms + 1 - lsb;
            
            } else {
                (*instr)->type = ARM64_INSTRUCTION_SBFIZ;
                lsb = (size - immr) & (size - 1);
                width = imms + 1;
            }

            /* Add operands */
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &lsb, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &width, ARM64_IMMEDIATE_TYPE_UINT);

        /* SBFM */
        } else {
            (*instr)->type = ARM64_INSTRUCTION_SBFM;

            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &immr, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imms, ARM64_IMMEDIATE_TYPE_UINT);
        }

    } else if (opc == 1) {

        /* Add common operand */
        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

        /* BFC */
        if (Rn == 0b11111 && (imms < immr)) {
            (*instr)->type = ARM64_INSTRUCTION_BFC;

            /* Calculate lsb and width */
            unsigned lsb = (size - immr) & (size - 1);
            unsigned width = imms + 1;

            /* Add operands */
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &lsb, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &width, ARM64_IMMEDIATE_TYPE_UINT);

        /* BFI / BFXIL */
        } else if ((Rn != 0b11111 && (imms < immr)) || (imms >= immr)) {

            /* Work out which one it is */
            if ((Rn != 0b11111 && (imms < immr))) (*instr)->type = ARM64_INSTRUCTION_BFI;
            else if (imms >= immr) (*instr)->type = ARM64_INSTRUCTION_BFXIL;
            else return LIBARCH_DECODE_STATUS_FAIL;

            /* Calculate lsb and width */
            unsigned lsb = immr;
            unsigned width = imms + 1 - lsb;

            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &lsb, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &width, ARM64_IMMEDIATE_TYPE_UINT);

        /* BFM */
        } else {
            (*instr)->type = ARM64_INSTRUCTION_BFM;

            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &immr, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imms, ARM64_IMMEDIATE_TYPE_UINT);
        }

    } else if (opc == 2) {

        /* Add common operands */
        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);

        /* LSL / LSR (immediate) */
        if (((imms != 0b11111) && imms + 1 == immr) || imms == 0b11111) {

            unsigned imm;

            if (imms == 0b11111) {
                (*instr)->type = ARM64_INSTRUCTION_LSR;
                imm = immr;
            } else {
                (*instr)->type = ARM64_INSTRUCTION_LSL;
                imm = (size - 1) - imms;
            }
            libarch_instruction_add_operand_immediate (instr, imm, ARM64_IMMEDIATE_TYPE_UINT);

        /* UBFX / UBFIZ */
        } else if (imms < immr || BFXPreferred (sf, (opc >> 1), imms, immr)) {
            if (BFXPreferred (sf, (opc >> 1), imms, immr)) (*instr)->type = ARM64_INSTRUCTION_UBFX;
            else (*instr)->type = ARM64_INSTRUCTION_UBFIZ;

            /* Calculate lsb and width */
            unsigned lsb = (size - immr) & (size - 1);
            unsigned width = imms + 1;

            /* Add operands */
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &lsb, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &width, ARM64_IMMEDIATE_TYPE_UINT);
           
        /* UXTB / UXTH */
        } else if (immr == 0 && (imms == 0b000111 || imms == 0b001111)) {
            if (imms == 0b000111) (*instr)->type = ARM64_INSTRUCTION_UXTB;
            else (*instr)->type = ARM64_INSTRUCTION_UXTH;

        /* UBFM */
        } else {
            (*instr)->type = ARM64_INSTRUCTION_UBFM;

            /* Add Operands */
            libarch_instruction_add_operand_immediate (instr, *(unsigned int*) &immr, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int*) &imms, ARM64_IMMEDIATE_TYPE_UINT);
        }
    } else {
        return LIBARCH_DECODE_STATUS_SOFT_FAIL;
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_extract (instruction_t **instr)
{
    unsigned sf = select_bits ((*instr)->opcode, 31, 31);
    unsigned op21 = select_bits ((*instr)->opcode, 29, 30);
    unsigned N = select_bits ((*instr)->opcode, 22, 22);
    unsigned o0 = select_bits ((*instr)->opcode, 21, 21);

    unsigned Rm = select_bits ((*instr)->opcode, 16, 20);
    unsigned imms = select_bits ((*instr)->opcode, 10, 15);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, sf);
    libarch_instruction_add_field (instr, op21);
    libarch_instruction_add_field (instr, N);
    libarch_instruction_add_field (instr, o0);
    libarch_instruction_add_field (instr, Rm);
    libarch_instruction_add_field (instr, imms);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rd);

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sf == 1 && N == 1) _SET_64 (size, regs, len);
    else _SET_32 (size, regs, len);

    /* Common operands */
    libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
    libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);

    /* ROR (immediate ) */
    if (Rn == Rm) {
        (*instr)->type = ARM64_INSTRUCTION_ROR;
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imms, ARM64_IMMEDIATE_TYPE_UINT);

    /* EXTR */
    } else {
        (*instr)->type == ARM64_INSTRUCTION_EXTR;
        libarch_instruction_add_operand_register (instr, Rm, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imms, ARM64_IMMEDIATE_TYPE_UINT);
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
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