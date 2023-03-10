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

    /* Add operands */
    libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);

    unsigned imm = 0;
    if (op == 0) {
        (*instr)->type = ARM64_INSTRUCTION_ADR;

        imm = ((immhi << 2) | immlo);
        imm = sign_extend (imm, 21);
        imm += (*instr)->addr;
        libarch_instruction_add_operand_immediate (instr, imm, ARM64_IMMEDIATE_TYPE_ULONG);

        mstrappend (&(*instr)->parsed,
            "adr    %s, 0x%x",
            libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
            imm);
        
    } else if (op == 1) {
        (*instr)->type = ARM64_INSTRUCTION_ADRP;

        imm = ((immhi << 2) | immlo) << 12;
        imm = sign_extend (imm, 32);
        imm = ((*instr)->addr & 0xfff);
        libarch_instruction_add_operand_immediate (instr, imm, ARM64_IMMEDIATE_TYPE_ULONG);

        mstrappend (&(*instr)->parsed,
            "adrp   %s, 0x%x",
            libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
            imm);
    }

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_add_subtract_immediate (instruction_t **instr)
{
    unsigned sf = select_bits ((*instr)->opcode, 31, 31); //select_bits ((*instr)->opcode, 30, 31);
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
    unsigned size = (sf == 1) ? 64 : 32;
    const char **regs = (sf == 1) ? A64_REGISTERS_GP_64 : A64_REGISTERS_GP_32;
    uint32_t len = (sf == 1) ? A64_REGISTERS_GP_64_LEN : A64_REGISTERS_GP_32_LEN;

    /* MOV/ADD */
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
    unsigned size = (sf == 1) ? 64 : 32;
    const char **regs = (sf == 1) ? A64_REGISTERS_GP_64 : A64_REGISTERS_GP_32;
    uint32_t len = (sf == 1) ? A64_REGISTERS_GP_64_LEN : A64_REGISTERS_GP_32_LEN;
    int imm_type = (sf == 1) ? ARM64_IMMEDIATE_TYPE_LONG : ARM64_IMMEDIATE_TYPE_INT; 

    unsigned long imm = 0;
    decode_bit_masks (N, imms, immr, 1, &imm);

    if (sf == 0 && N == 1) {
        // Unallocated
    } else if (opc == 0) {
        // AND
        (*instr)->type = ARM64_INSTRUCTION_AND;

        libarch_instruction_add_operand_register (instr, Rd, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_register (instr, Rn, size, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, 
            (imm_type == ARM64_IMMEDIATE_TYPE_LONG) ? *(long *)&imm : *(int *)&imm);


    } else if (opc == 1) {
        // ORR - 32-bit
    } else if (opc == 2) {
        // EOR - 32-bit
    } else if (opc == 3) {
        // ANDS - 32-bit
    }
}

libarch_return_t
disass_data_processing_instruction (instruction_t *instr)
{
    /**
     *  ** Data Processing - Immediate: Encoding **
     * 
     *  Data Processing Immediate instructions have a single `op0`, 3 bits wide,
     *  located from bit 23:25. This determines the type of immediate operation.
     * 
     */
    unsigned op0 = select_bits (instr->opcode, 23, 25);

    if ((op0 >> 1) == 0) {
        decode_pc_relative_addressing (&instr);
    } else if (op0 == 2) {
        decode_add_subtract_immediate (&instr);
    } else if (op0 == 3) {
        decode_add_subtract_immediate_tags (&instr);
    }
    /*else if (op0 == 3) printf ("add/subtract immediate w/ tags\n");
    else if (op0 == 4) printf ("logical immediate\n");
    else if (op0 == 5) printf ("move wide\n");
    else if (op0 == 6) printf ("bitfield\n");
    else if (op0 == 7) printf ("extract\n");*/

    return LIBARCH_RETURN_SUCCESS;
}
