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

#define CHECK_BIT(a, bit)       ((a & (1<<bit)) != 0)

static char * _get_instruction_mnemonic (arm64_instr_t mnemonic)
{
    switch (mnemonic) {
        case ARM64_INSTRUCTION_MOV:
            return "MOV";
        case ARM64_INSTRUCTION_ADD:
            return "ADD";
        case ARM64_INSTRUCTION_ADDS:
            return "ADDS";
        case ARM64_INSTRUCTION_SUB:
            return "SUB";
        case ARM64_INSTRUCTION_SUBS:
            return "SUBS";
        default:
            return "unk";
    }
}

///////////////////////////////////////////////////////////////////

static libarch_return_t
decode_pc_relative_addressing (instruction_t **instr)
{
    /**
     *  ** Data Processing - Immediate: PC-Relative Addressing **
     * 
     *  PC-Relative Addressing instructions: ADR and ADRP.
     * 
     *  - op[31]
     *    Instruction type: 0 - ADR, 1 - ADRP.
     * 
     *  - immlo[29:30]
     *    Immediate value low-bytes.
     * 
     *  - immhi[5:23]
     *    Immediate value high-bytes.
     * 
     *  - Rd[0:4]
     *    Destination Register.
     * 
     */
    unsigned op = select_bits ((*instr)->opcode, 31, 31);
    unsigned immlo = select_bits ((*instr)->opcode, 29, 30);
    unsigned immhi = select_bits ((*instr)->opcode, 5, 23);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, op);
    libarch_instruction_add_field (instr, immlo);
    libarch_instruction_add_field (instr, immhi);
    libarch_instruction_add_field (instr, Rd);

    uint64_t imm = 0;
    if (op == 0) {
        (*instr)->type = ARM64_INSTRUCTION_ADR;

        imm = ((immhi << 2) | immlo);
        imm = sign_extend (imm, 21);
        imm += (*instr)->addr;

        mstrappend (&(*instr)->parsed,
            "adr    %s, #0x%llx",
            libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
            imm);
        
    } else if (op == 1) {
        (*instr)->type = ARM64_INSTRUCTION_ADRP;

        imm = ((immhi << 2) | immlo) << 12;
        imm = sign_extend (imm, 32);
        imm += ((*instr)->addr & ~0xfff);

        mstrappend (&(*instr)->parsed,
            "adrp   %s, #0x%llx",
            libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
            imm);
    }

    /* Add operands */
    libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);
    libarch_instruction_add_operand_immediate (instr, imm, ARM64_IMMEDIATE_TYPE_ULONG);

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
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
    // ...

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sf == 1) _SET_64 (size, regs, len);
    else _SET_32 (size, regs, len);

    /* ADD */
    if (op == 0 && S == 0) {
        /* MOV Alias */
        if (sh == 0 && imm12 == 0 && (Rd == 0b11111 || Rn == 0b11111)) {
            (*instr)->type = ARM64_INSTRUCTION_MOV;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);

            mstrappend (&(*instr)->parsed,
                "mov    %s, %s",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len));

        } else {
            /* ADD (immediate) */
            (*instr)->type = ARM64_INSTRUCTION_ADD;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

            mstrappend (&(*instr)->parsed,
                "add    %s, %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                imm12);

            /* If there is also a shift, append it */
            if (sh) {
                libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);
                mstrappend (&(*instr)->parsed, ", lsl #12");
            }
        }

    /* ADDS/CMN */
    } else if (op == 0 && S == 1) {
        /* CMN Alias */
        if (Rd == 0b11111) {
            (*instr)->type = ARM64_INSTRUCTION_CMN;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

            mstrappend (&(*instr)->parsed,
                "cmn   %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                imm12);

            /* If there is also a shift, append it */
            if (sh) {
                libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);
                mstrappend (&(*instr)->parsed, ", lsl #12");
            }

        } else {
            /* ADDS (immediate) */
            (*instr)->type = ARM64_INSTRUCTION_ADDS;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

            mstrappend (&(*instr)->parsed,
                "adds   %s, %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                imm12);

            /* If there is also a shift, append it */
            if (sh) {
                libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);
                mstrappend (&(*instr)->parsed, ", lsl #12");
            }

        }

    } else if (op == 1 && S == 0) {
        (*instr)->type = ARM64_INSTRUCTION_SUB;

        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

        mstrappend (&(*instr)->parsed,
            "sub    %s, %s, #0x%x",
            libarch_get_general_register (Rd, regs, len),
            libarch_get_general_register (Rn, regs, len),
            imm12);

        /* If there is also a shift, append it */
        if (sh) {
            libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);
            mstrappend (&(*instr)->parsed, ", lsl #12");
        }
    } else if (op == 1 && S == 1) {
        /* CMP Alias */
        if (Rd == 0b11111) {
            (*instr)->type = ARM64_INSTRUCTION_CMP;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

            mstrappend (&(*instr)->parsed,
                "cmp   %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                imm12);

            /* If there is also a shift, append it */
            if (sh) {
                libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);
                mstrappend (&(*instr)->parsed, ", lsl #12");
            }
        } else {
            /* SUBS (immediate) */
            (*instr)->type = ARM64_INSTRUCTION_SUBS;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, imm12, ARM64_IMMEDIATE_TYPE_ULONG);

            mstrappend (&(*instr)->parsed,
                "subs   %s, %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                imm12);

            /* If there is also a shift, append it */
            if (sh) {
                libarch_instruction_add_operand_shift (instr, 12, ARM64_SHIFT_TYPE_LSL);
                mstrappend (&(*instr)->parsed, ", lsl #12");
            }
        }

    } else {
        printf ("error: sf: %d, op: %d, S: %d\n", sf, op, S);
        return LIBARCH_RETURN_FAILURE;
    }

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
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

    if (sf == 1 && op == 0 && S == 0 && o2 == 0) {
        /* ADDG */
        (*instr)->type = ARM64_INSTRUCTION_ADDG;

        libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, uimm6, ARM64_IMMEDIATE_TYPE_ULONG);
        libarch_instruction_add_operand_immediate (instr, uimm4, ARM64_IMMEDIATE_TYPE_ULONG);

        mstrappend (&(*instr)->parsed,
            "addg   %s, %s, #0x%x, #0x%x",
            libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
            libarch_get_general_register (Rn, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
            uimm6, uimm4);

    } else if (sf == 1 && op == 1 && S == 0 && o2 == 0) {
        /* SUBG */
        (*instr)->type = ARM64_INSTRUCTION_SUBG;

        libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, uimm6, ARM64_IMMEDIATE_TYPE_ULONG);
        libarch_instruction_add_operand_immediate (instr, uimm4, ARM64_IMMEDIATE_TYPE_ULONG);

        mstrappend (&(*instr)->parsed,
            "subg   %s, %s, #0x%x, #0x%x",
            libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
            libarch_get_general_register (Rn, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
            uimm6, uimm4);
    }

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
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

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sf == 0 && N == 0) _SET_32 (size, regs, len);
    else _SET_64 (size, regs, len);

    int imm_type = (sf == 1) ? ARM64_IMMEDIATE_TYPE_LONG : ARM64_IMMEDIATE_TYPE_INT; 

    unsigned long imm = 0;
    decode_bitmasks (N, imms, immr, 1, &imm);

    if (sf == 0 && N == 1) {
        // Unallocated
    } else if (opc == 0) {
        // AND
        (*instr)->type = ARM64_INSTRUCTION_AND;

        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, 
            (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm,
            imm_type);

        mstrappend (&(*instr)->parsed,
            "and    %s, %s, #0x%x",
            libarch_get_general_register (Rd, regs, len),
            libarch_get_general_register (Rn, regs, len),
            (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm);


    } else if (opc == 1) {
        // MOV (bitmask immediate)
        if (Rn == 0b11111 && !move_wide_preferred (sf, N, imms, immr)) {
            (*instr)->type = ARM64_INSTRUCTION_MOV;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, 
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm,
                imm_type);

            mstrappend (&(*instr)->parsed,
                "mov    %s, #0x%x",
                libarch_get_general_register (Rn, regs, len),
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm);
            
        } else {
            // ORR
            (*instr)->type = ARM64_INSTRUCTION_ORR;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, 
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm,
                imm_type);

            mstrappend (&(*instr)->parsed,
                "orr    %s, %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm);
        }
    } else if (opc == 2) {
        // EOR
        (*instr)->type = ARM64_INSTRUCTION_EOR;

        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, 
            (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm,
            imm_type);

        mstrappend (&(*instr)->parsed,
            "eor    %s, %s, #0x%x",
            libarch_get_general_register (Rd, regs, len),
            libarch_get_general_register (Rn, regs, len),
            (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm);

    } else if (opc == 3) {
        // TST
        if (Rd == 0b11111) {
            (*instr)->type = ARM64_INSTRUCTION_TST;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, 
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm,
                imm_type);

            mstrappend (&(*instr)->parsed,
                "tst    %s, #0x%x",
                libarch_get_general_register (Rn, regs, len),
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm);

        } else {
            // ANDS
            (*instr)->type = ARM64_INSTRUCTION_ANDS;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, 
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm,
                imm_type);

            mstrappend (&(*instr)->parsed,
                "ands   %s, %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm);
            }
    }
}

static libarch_return_t
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

    unsigned shift = hw << 4;

    /* Determine instruction size */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sf == 0 && (hw >> 1) == 0) _SET_32 (size, regs, len);
    else _SET_64 (size, regs, len);

    if (opc == 1) {
        // Unallocated
    } else if (opc == 0) {
        // MOV (inverted wide immediate)
        if (!(is_zero (imm16) && hw != 0)) {
            (*instr)->type = ARM64_INSTRUCTION_MOV;

            int imm_type = (size == 64) ? ARM64_IMMEDIATE_TYPE_LONG : ARM64_IMMEDIATE_TYPE_INT;
            long imm = ~((long) imm16 << shift);

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, 
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *) &imm : *(int *) &imm,
                imm_type);

            mstrappend (&(*instr)->parsed,
                "mov    %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm);

        } else {
            // MOVN
            (*instr)->type = ARM64_INSTRUCTION_MOVN;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imm16, ARM64_IMMEDIATE_TYPE_UINT);

            /* Extra if there is a shift */
            if (shift) {
                libarch_instruction_add_operand_shift (instr, shift, ARM64_SHIFT_TYPE_LSL);

                mstrappend (&(*instr)->parsed,
                    "movn   %s, #0x%x, lsl #%d",
                    libarch_get_general_register (Rd, regs, len),
                    *(unsigned int *) &imm16,
                    shift);
            } else {
                mstrappend (&(*instr)->parsed,
                    "movn   %s, #0x%x",
                    libarch_get_general_register (Rd, regs, len),
                    *(unsigned int *) &imm16);
            }

        }
    } else if (opc == 2) {
        // MOV (wide immediate)
        if (!is_zero (imm16) && hw != 0b00) {
            (*instr)->type = ARM64_INSTRUCTION_MOV;

            int imm_type = (size == 64) ? ARM64_IMMEDIATE_TYPE_LONG : ARM64_IMMEDIATE_TYPE_INT;
            long imm = ~((long) imm16 << shift);

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, 
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *) &imm : *(int *) &imm,
                imm_type);

            mstrappend (&(*instr)->parsed,
                "mov    %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm);

        } else {
            // MOVZ
            (*instr)->type = ARM64_INSTRUCTION_MOVZ;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imm16, ARM64_IMMEDIATE_TYPE_UINT);

            /* Extra if there is a shift */
            if (shift) {
                libarch_instruction_add_operand_shift (instr, shift, ARM64_SHIFT_TYPE_LSL);

                mstrappend (&(*instr)->parsed,
                    "movz   %s, #0x%x, lsl #%d",
                    libarch_get_general_register (Rd, regs, len),
                    *(unsigned int *) &imm16,
                    shift);
            } else {
                mstrappend (&(*instr)->parsed,
                    "movz   %s, #0x%x",
                    libarch_get_general_register (Rd, regs, len),
                    *(unsigned int *) &imm16);
            }
        }
    } else if (opc == 3) {
        // MOVK
        (*instr)->type = ARM64_INSTRUCTION_MOVK;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imm16, ARM64_IMMEDIATE_TYPE_UINT);

            /* Extra if there is a shift */
            if (shift) {
                libarch_instruction_add_operand_shift (instr, shift, ARM64_SHIFT_TYPE_LSL);

                mstrappend (&(*instr)->parsed,
                    "movk   %s, #0x%x, lsl #%d",
                    libarch_get_general_register (Rd, regs, len),
                    *(unsigned int *) &imm16,
                    shift);
            } else {
                mstrappend (&(*instr)->parsed,
                    "movk   %s, #0x%x",
                    libarch_get_general_register (Rd, regs, len),
                    *(unsigned int *) &imm16);
            }
    }
    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_bitfield (instruction_t **instr)
{
    unsigned sf = select_bits ((*instr)->opcode, 31, 31);
    unsigned opc = select_bits ((*instr)->opcode, 29, 30);
    unsigned N = select_bits ((*instr)->opcode, 22, 22);
    
    unsigned immr = select_bits ((*instr)->opcode, 16, 21);
    unsigned imms = select_bits ((*instr)->opcode, 10, 15);

    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sf == 1) _SET_64 (size, regs, len);
    else _SET_32 (size, regs, len);

    // SBFM
    if (opc == 0) {
        
        if ((sf == 0 && imms == 0b011111) || (sf == 1 && imms == 0b111111)) {
            (*instr)->type = ARM64_INSTRUCTION_ADR;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &immr, ARM64_IMMEDIATE_TYPE_UINT);

            mstrappend(&(*instr)->parsed,
                "asr    %s, %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                immr);

        } else if (imms < immr || BFXPreferred (sf, (opc >> 1), imms, immr)) {
            if (BFXPreferred (sf, (opc >> 1), imms, immr))
                (*instr)->type = ARM64_INSTRUCTION_SBFX;
            else
                (*instr)->type = ARM64_INSTRUCTION_SBFIZ;

            unsigned lsb = (size - immr) & (size - 1);
            unsigned width = imms + 1;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &lsb, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &width, ARM64_IMMEDIATE_TYPE_UINT);

            mstrappend(&(*instr)->parsed,
                "%s   %s, %s, #0x%x, #%d",
                A64_INSTRUCTIONS_STR[(*instr)->type],
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                lsb, width);

        } else if (immr == 0 && (imms == 0b000111 || imms == 0b001111 || imms == 0b011111)) {

            /**
             *  SXTB, SXTH and SXTW have the same format:
             * 
             *      SXTB|H|W    Rd, Rn
             */
            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);

            char *name;
            if (imms == 0b000111) (*instr)->type = ARM64_INSTRUCTION_SXTB;
            else if (imms == 0b001111) (*instr)->type = ARM64_INSTRUCTION_SXTH;
            else if (imms == 0b011111) (*instr)->type = ARM64_INSTRUCTION_SXTW;

            mstrappend(&(*instr)->parsed,
                "%s   %s, %s",
                A64_INSTRUCTIONS_STR[(*instr)->type],
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len));

        } else {
            (*instr)->type = ARM64_INSTRUCTION_SBFM;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, immr, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, imms, ARM64_IMMEDIATE_TYPE_UINT);

            mstrappend(&(*instr)->parsed,
                "sbfm   %s, %s, #0x%x, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                immr, imms);
        }

    } else if (opc == 0b01) {
        
        // BFC
        if (Rn == 0b11111 && (imms < immr)) {
            (*instr)->type = ARM64_INSTRUCTION_BFC;

            unsigned lsb = (size - immr) & (size - 1);
            unsigned width = imms + 1;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &lsb, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &width, ARM64_IMMEDIATE_TYPE_UINT);

            mstrappend(&(*instr)->parsed,
                "bfc    %s, #0x%x, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                immr, imms);

        } else if ((Rn != 0b11111 && (imms < immr)) || (imms >= immr)) {

            // BFI/BFXIL
            if ((Rn != 0b11111 && (imms < immr)))
                (*instr)->type = ARM64_INSTRUCTION_BFI;
            else if (imms >= immr)
                (*instr)->type = ARM64_INSTRUCTION_BFXIL;

            unsigned lsb = (size - immr) & (size - 1);
            unsigned width = imms + 1;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &lsb, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &width, ARM64_IMMEDIATE_TYPE_UINT);

            mstrappend(&(*instr)->parsed,
                "%s   %s, %s, #0x%x, #%d",
                A64_INSTRUCTIONS_STR[(*instr)->type],
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                lsb, width);
        } else {
            // BFM
            (*instr)->type = ARM64_INSTRUCTION_BFM;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, immr, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, imms, ARM64_IMMEDIATE_TYPE_UINT);

            mstrappend(&(*instr)->parsed,
                "bfm    %s, %s, #0x%x, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                immr, imms);
        }
    
    } else if (opc == 0b10) {

        // LSL/LSR (immediate)
        if ((imms != 0b11111) && imms + 1 == immr) {
            // LSL
            (*instr)->type = ARM64_INSTRUCTION_LSL;
            
            unsigned shift = (size - 1) - imms;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, shift, ARM64_IMMEDIATE_TYPE_UINT);

            mstrappend(&(*instr)->parsed,
                "%s    %s, %s, #0x%x",
                A64_INSTRUCTIONS_STR[(*instr)->type],
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                shift);

        } else if (imms == 0b11111) {
            // LSR
            (*instr)->type = ARM64_INSTRUCTION_LSR;
            
            unsigned shift = (size - 1) - imms;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, shift, ARM64_IMMEDIATE_TYPE_UINT);

            mstrappend(&(*instr)->parsed,
                "%s    %s, %s, #0x%x",
                A64_INSTRUCTIONS_STR[(*instr)->type],
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                shift);

        } else if (imms < immr || BFXPreferred (sf, (opc >> 1), imms, immr)) {
            if (BFXPreferred (sf, (opc >> 1), imms, immr))
                (*instr)->type = ARM64_INSTRUCTION_UBFX;
            else
                (*instr)->type = ARM64_INSTRUCTION_UBFIZ;

            unsigned lsb = (size - immr) & (size - 1);
            unsigned width = imms + 1;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &lsb, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &width, ARM64_IMMEDIATE_TYPE_UINT);

            mstrappend(&(*instr)->parsed,
                "%s   %s, %s, #0x%x, #%d",
                A64_INSTRUCTIONS_STR[(*instr)->type],
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                lsb, width);

        } else if (immr == 0 && (imms == 0b000111 || imms == 0b001111)) {

            if (imms == 0b000111) (*instr)->type = ARM64_INSTRUCTION_UXTB;
            else (*instr)->type = ARM64_INSTRUCTION_UXTH;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);

            mstrappend(&(*instr)->parsed,
                "%s   %s, %s",
                A64_INSTRUCTIONS_STR[(*instr)->type],
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len));
        } else {
            // UBFM
            (*instr)->type = ARM64_INSTRUCTION_UBFM;

            libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int*) &immr, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int*) &imms, ARM64_IMMEDIATE_TYPE_UINT);

            mstrappend(&(*instr)->parsed,
                "ubfm   %s, %s, #0x%x, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                immr, imms);
        }
        
    }

    
    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
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

    printf ("extract\n");

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sf == 1 && N == 1) _SET_64 (size, regs, len);
    else _SET_32 (size, regs, len);

    // ROR (immediate)
    if (Rn == Rm) {
        (*instr)->type = ARM64_INSTRUCTION_ROR;

        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imms, ARM64_IMMEDIATE_TYPE_UINT);

        mstrappend(&(*instr)->parsed,
                "ror    %s, %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                imms);

    } else {
        // EXTR
        (*instr)->type == ARM64_INSTRUCTION_EXTR;

        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rm, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imms, ARM64_IMMEDIATE_TYPE_UINT);

        mstrappend(&(*instr)->parsed,
                "extr   %s, %s, %s, #0x%x",
                libarch_get_general_register (Rd, regs, len),
                libarch_get_general_register (Rn, regs, len),
                libarch_get_general_register (Rm, regs, len),
                imms);
    }
}

libarch_return_t
disass_data_processing_instruction (instruction_t *instr)
{
    unsigned op0 = select_bits (instr->opcode, 23, 25);

    if ((op0 >> 1) == 0) {
        decode_pc_relative_addressing (&instr);
    } else if (op0 == 2) {
        decode_add_subtract_immediate (&instr);
    } else if (op0 == 3) {
        decode_add_subtract_immediate_tags (&instr);
    } else if (op0 == 4) {
        decode_logical_immediate (&instr);
    } else if (op0 == 5) {
        decode_move_wide_immediate (&instr);
    } else if (op0 == 6) {
        decode_bitfield (&instr);
    } else if (op0 == 7) {
        decode_extract (&instr);
    }
    return LIBARCH_RETURN_SUCCESS;
}
