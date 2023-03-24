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

#include "arm64/arm64-conditions.h"
#include "decoder/branch.h"

static libarch_return_t
decode_conditional_branch (instruction_t **instr)
{
    unsigned o1 = select_bits ((*instr)->opcode, 24, 24);
    unsigned o0 = select_bits ((*instr)->opcode, 4, 4);

    unsigned imm19 = select_bits ((*instr)->opcode, 5, 23);
    unsigned cond = select_bits ((*instr)->opcode, 0, 3);

    /* Unallocated */
    if (o0 != 0 && o1 != 0)
        return LIBARCH_RETURN_VOID;

    // B.cond
    (*instr)->type = ARM64_INSTRUCTION_B;
    (*instr)->cond = cond;

    uint64_t imm = sign_extend(imm19 << 2, 64) + (*instr)->addr;
    libarch_instruction_add_operand_immediate (instr, *(unsigned long *) &imm, ARM64_IMMEDIATE_TYPE_ULONG);

    mstrappend (&(*instr)->parsed,
        "b.%s   0x%lx",
        A64_CONDITIONS_STR[cond],
        imm);
}

static libarch_return_t
decode_exception_generation (instruction_t **instr)
{
    unsigned opc = select_bits ((*instr)->opcode, 21, 23);
    unsigned imm16 = select_bits ((*instr)->opcode, 5, 20);
    unsigned op2 = select_bits ((*instr)->opcode, 2, 4);
    unsigned LL = select_bits ((*instr)->opcode, 0, 1);

    /**
     *  Check for "Unallocated" opcodes that don't need to be
     *  parsed.
     */
    if (op2 == 1 || (op2 >> 1) == 1 || (op2 >> 2) == 1)
        return LIBARCH_RETURN_VOID;

    if (op2 == 0) {
        if (opc == 0 && LL == 0) return LIBARCH_RETURN_VOID;
        if ((opc == 1 || opc == 2) && ((LL == 3 || LL == 1) || (LL == 2 || LL == 3)))
            return LIBARCH_RETURN_VOID;
        if (opc == 3 && (LL >= 1 && LL <= 3))
            return LIBARCH_RETURN_VOID;
        if (opc >= 5 && opc <= 7)
            return LIBARCH_RETURN_VOID;
        if (opc == 5 && LL == 0)
            return LIBARCH_RETURN_VOID;
    }

    if (op2 != 0) return LIBARCH_RETURN_VOID;

    libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imm16, ARM64_IMMEDIATE_TYPE_UINT);

    if (opc == 0) {
        if (LL == 1) (*instr)->type = ARM64_INSTRUCTION_SVC;
        else if (LL == 2) (*instr)->type = ARM64_INSTRUCTION_HVC;
        else if (LL == 3) (*instr)->type = ARM64_INSTRUCTION_SMC;
    } else if (LL == 0) {
        if (opc == 1) (*instr)->type = ARM64_INSTRUCTION_BRK;
        else if (opc == 2) (*instr)->type = ARM64_INSTRUCTION_HLT;
    } else if (opc == 5) {
        if (LL == 1) (*instr)->type == ARM64_INSTRUCTION_DCPS1;
        else if (LL == 2) (*instr)->type == ARM64_INSTRUCTION_DCPS2;
        else if (LL == 3) (*instr)->type == ARM64_INSTRUCTION_DCPS3;
    }

    mstrappend (&(*instr)->parsed,
        "%s    #0x%x",
        A64_INSTRUCTIONS_STR[(*instr)->type],
        imm16);


    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_system_instruction_with_register (instruction_t **instr)
{
    unsigned CRm = select_bits ((*instr)->opcode, 8, 11);
    unsigned op2 = select_bits ((*instr)->opcode, 5, 7);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Unallocated */
    if (CRm != 0 || (CRm == 0 && ((op2 >> 1) == 1 || (op2 >> 2) == 1)))
        return LIBARCH_RETURN_VOID;

    libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);

    // WFET
    if (op2 == 0)  (*instr)->type = ARM64_INSTRUCTION_WFET;
    // WFIT
    else if (op2 == 1) (*instr)->type = ARM64_INSTRUCTION_WFIT;

    mstrappend (&(*instr)->parsed,
        "%s   %s",
        A64_INSTRUCTIONS_STR[(*instr)->type],
        libarch_get_general_register (Rt, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN));

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_hints (instruction_t **instr)
{
    unsigned CRm = select_bits ((*instr)->opcode, 8, 11);
    unsigned op2 = select_bits ((*instr)->opcode, 5, 7);
    unsigned Z = select_bits ((*instr)->opcode, 13, 13);
    unsigned D = select_bits ((*instr)->opcode, 10, 10);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);
    printf ("yup\n");

    if (CRm == 4 && (op2 & ~6) == 0) {
        (*instr)->type = ARM64_INSTRUCTION_BTI;

        unsigned ind = op2 >> 1;
        const char *targets[] = {"", "c", "j", "jc"};
        mstrappend (&(*instr)->parsed, "bti %s", targets[ind]);

    // NOP / YIELD / WFE / WFI / SEV / SEVL / DGH
    } else if (CRm == 0) {
        if (op2 == 0) {
            (*instr)->type = ARM64_INSTRUCTION_NOP;
            (*instr)->parsed = "nop";
        } else if (op2 == 1) {
            (*instr)->type = ARM64_INSTRUCTION_YIELD;
            (*instr)->parsed = "yield";
        } else if (op2 == 2) {
            (*instr)->type = ARM64_INSTRUCTION_WFE;
            (*instr)->parsed = "wfe";
        } else if (op2 == 3) {
            (*instr)->type = ARM64_INSTRUCTION_WFI;
            (*instr)->parsed = "wfi";
        } else if (op2 == 4) {
            (*instr)->type = ARM64_INSTRUCTION_SEV;
            (*instr)->parsed = "sev";
        } else if (op2 == 5) {
            (*instr)->type = ARM64_INSTRUCTION_SEVL;
            (*instr)->parsed = "sevl";
        } else if (op2 == 6) {
            (*instr)->type = ARM64_INSTRUCTION_DGH;
            (*instr)->parsed = "dgh";
        } else if (op2 == 7) {
            if (D == 0) (*instr)->type = ARM64_INSTRUCTION_XPACI;
            else if (D == 1) (*instr)->type = ARM64_INSTRUCTION_XPACD;

            libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);

            mstrappend (&(*instr)->parsed,
                "%s  %s",
                A64_INSTRUCTIONS_STR[(*instr)->type],
                libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN));

        } else if (op2 == 7 && ((*instr)->opcode == 0xd50320ff)) {
            (*instr)->type = ARM64_INSTRUCTION_XPACLRI;
            (*instr)->parsed = "xpaclri";
        }
    
    } else if (CRm == 1) {
        // PACIA / PACIZA / PACIA1716
        if (op2 == 0) {
            // PACIA
            if (Z == 0) {
                (*instr)->type = ARM64_INSTRUCTION_PACIA;

                libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);
                libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL);

                mstrappend (&(*instr)->parsed,
                    "pacia  %s, %s",
                    libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
                    libarch_get_general_register (Rn, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN));

            // PACIZA
            } else if (Z == 1 && Rn == 0b11111) {
                (*instr)->type = ARM64_INSTRUCTION_PACIZA;

                libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);

                mstrappend (&(*instr)->parsed,
                    "paciza %s",
                    libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN));
            
            // PACIA1716
            } else {
                (*instr)->type = ARM64_INSTRUCTION_PACIA1716;
                (*instr)->parsed = "pacia1716";
            }

        // PACIB / PACIZB / PACIB1716
        } else if (op2 == 2) {
            // PACIB
            if (Z == 0) {
                (*instr)->type = ARM64_INSTRUCTION_PACIB;

                libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);
                libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL);

                mstrappend (&(*instr)->parsed,
                    "pacib  %s, %s",
                    libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
                    libarch_get_general_register (Rn, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN));

            // PACIZB
            } else if (Z == 1 && Rn == 0b11111) {
                (*instr)->type = ARM64_INSTRUCTION_PACIZA;

                libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);

                mstrappend (&(*instr)->parsed,
                    "pacizb %s",
                    libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN));
            
            // PACIB1716
            } else {
                (*instr)->type = ARM64_INSTRUCTION_PACIB1716;
                (*instr)->parsed = "pacib1716";
            }

        // AUTIA / AUTIZA / AUTIA1716
        } else if (op2 == 4) {
            // AUTIA
            if (Z == 0) {
                (*instr)->type = ARM64_INSTRUCTION_AUTIA;

                libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);
                libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL);

                mstrappend (&(*instr)->parsed,
                    "autia  %s, %s",
                    libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
                    libarch_get_general_register (Rn, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN));

            // AUTIZA
            } else if (Z == 1 && Rn == 0b11111) {
                (*instr)->type = ARM64_INSTRUCTION_AUTIZA;

                libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);

                mstrappend (&(*instr)->parsed,
                    "autiza %s",
                    libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN));

            // AUTIA1716
            } else {
                (*instr)->type = ARM64_INSTRUCTION_AUTIA1716;
                (*instr)->parsed = "autia1716";
            }

        // AUTIB / AUTIZB / AUTIB1716
        } else if (op2 == 6) {
            // AUTIB
            if (Z == 0) {
                (*instr)->type = ARM64_INSTRUCTION_AUTIB;

                libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);
                libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL);

                mstrappend (&(*instr)->parsed,
                    "autib  %s, %s",
                    libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN),
                    libarch_get_general_register (Rn, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN));

            // AUTIZB
            } else if (Z == 1 && Rn == 0b11111) {
                (*instr)->type = ARM64_INSTRUCTION_AUTIZB;

                libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL);

                mstrappend (&(*instr)->parsed,
                    "autizb %s",
                    libarch_get_general_register (Rd, A64_REGISTERS_GP_64, A64_REGISTERS_GP_64_LEN));

            // AUTIB1716
            } else {
                (*instr)->type = ARM64_INSTRUCTION_AUTIB1716;
                (*instr)->parsed = "autib1716";
            }
        }
    } else if (CRm == 2) {
        // ESB
        if (op2 == 0) {
            (*instr)->type = ARM64_INSTRUCTION_ESB;
            (*instr)->parsed = "esb";

        // PSB CSYNC
        } else if (op2 == 1) {
            (*instr)->type = ARM64_INSTRUCTION_PSB_CSYNC;
            (*instr)->parsed = "psb csync";

        // TSB CSYNC
        } else if (op2 == 2) {
            (*instr)->type = ARM64_INSTRUCTION_TSB_CSYNC;
            (*instr)->parsed = "tsb csync";

        // CSDB
        } else if (op2 == 4) {
            (*instr)->type = ARM64_INSTRUCTION_CSDB;
            (*instr)->parsed = "csdb";
        }
    } else if (CRm == 3) {
        // PACIASP
        if (op2 == 1) {
            (*instr)->type = ARM64_INSTRUCTION_PACIASP;
            (*instr)->parsed = "paciasp";

        // PACIAZ
        } else if (op2 == 0) {
            (*instr)->type = ARM64_INSTRUCTION_PACIAZ;
            (*instr)->parsed = "paciaz";
        
        // PACIBSP
        } else if (op2 == 3) {
            (*instr)->type = ARM64_INSTRUCTION_PACIBSP;
            (*instr)->parsed = "pacibsp";
        
        // PACIBZ
        } else if (op2 == 2) {
            (*instr)->type = ARM64_INSTRUCTION_PACIBZ;
            (*instr)->parsed = "pacibz";

        // AUTIASP
        } else if (op2 == 5) {
            (*instr)->type = ARM64_INSTRUCTION_AUTIASP;
            (*instr)->parsed = "autiasp";

        // AUTIAZ
        } else if (op2 == 4) {
            (*instr)->type = ARM64_INSTRUCTION_AUTIAZ;
            (*instr)->parsed = "autiaz";

        // AUTIBSP
        } else if (op2 == 7) {
            (*instr)->type = ARM64_INSTRUCTION_AUTIASP;
            (*instr)->parsed = "autibsp";

        // AUTIBZ
        } else if (op2 == 6) {
            (*instr)->type = ARM64_INSTRUCTION_AUTIAZ;
            (*instr)->parsed = "autibz";
        }
    }

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_barriers (instruction_t **instr)
{
    unsigned CRm = select_bits ((*instr)->opcode, 8, 11);
    unsigned op2 = select_bits ((*instr)->opcode, 5, 7);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Unallocated */
    if (op2 == 0) return LIBARCH_RETURN_VOID;
    if ((op2 == 1 || op2 == 7) && Rt != 0b11111) return LIBARCH_RETURN_VOID;
    // if CRm == xx0x, op2 == 001, Rt = 11111
    // if CRm == xx11, op2 == 001, Rt = 11111
    if (CRm == 1 && op2 == 3) return LIBARCH_RETURN_VOID;
    if ((CRm >> 1) == 1 && op2 == 3) return LIBARCH_RETURN_VOID;
    if ((CRm >> 2) == 1 && op2 == 3) return LIBARCH_RETURN_VOID;
    if ((CRm >> 3) == 1 && op2 == 3) return LIBARCH_RETURN_VOID;

    // CLREX
    if (op2 == 2 && Rt == 0b11111) {
        (*instr)->type = ARM64_INSTRUCTION_CLREX;

        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &CRm, ARM64_IMMEDIATE_TYPE_UINT);
        mstrappend (&(*instr)->parsed, "clrex  #0x%x", *(unsigned int *) &CRm);

    // DSB
    } else if (Rt == 0b11111) {
        // DSB
        if (op2 == 4) (*instr)->type = ARM64_INSTRUCTION_DSB;
        else if (op2 == 5) (*instr)->type = ARM64_INSTRUCTION_DMB;
        else if (op2 == 6) (*instr)->type = ARM64_INSTRUCTION_ISB;
        else if (op2 == 7) (*instr)->type = ARM64_INSTRUCTION_SB;
        else if (op2 == 3) (*instr)->type = ARM64_INSTRUCTION_TCOMMIT;

        (*instr)->parsed = A64_INSTRUCTIONS_STR[(*instr)->type];
    }

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_pstate (instruction_t **instr)
{
    unsigned op1 = select_bits ((*instr)->opcode, 16, 18);
    unsigned CRm = select_bits ((*instr)->opcode, 8, 11);
    unsigned op2 = select_bits ((*instr)->opcode, 5, 7);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    unsigned char *pstate;

    /* Unallocated */
    if (Rt != 0b11111) return LIBARCH_RETURN_VOID;

    // CFINV / XAFLAG / AXFLAG
    if (op1 == 0) {

        // CFINV
        if (op2 == 0) {

        // XAFLAG
        } else if (op2 == 1) {

        // AXFLAG
        } else if (op2 == 2) {


        }

    } else {
        // MSR (Immediate)
        (*instr)->type = ARM64_INSTRUCTION_MSR;

        // Calculate pstate
        if (op1 == 0 && op2 == 5) pstate = "SPSel";
        else if (op1 == 3 && op2 == 6) pstate = "DAIFSet";
        else if (op1 == 3 && op2 == 7) pstate = "DAIFClr";
        else if (op1 == 0 && op2 == 3) pstate = "UAO";
        else if (op1 == 0 && op2 == 4) pstate = "PAN";
        else if (op1 == 1 && op2 == 0 && (CRm >> 1) == 0) pstate = "ALLINT";
        else if (op1 == 3 && op2 == 1) pstate = "SSBS";
        else if (op1 == 3 && op2 == 2) pstate = "DIT";
        else if (op1 == 3 && op2 == 3) {
            if ((CRm >> 1) == 1) pstate = "SVCRSM";
            else if ((CRm >> 1) == 2) pstate = "SVCRZA";
            else if ((CRm >> 1) == 3) pstate = "SVCRSMZA";
        }
        else if (op1 == 3 && op2 == 4) pstate = "TC0";

        mstrappend (&(*instr)->parsed,
            "msr    %s, #0x%x",
            pstate, CRm);

    }

    return LIBARCH_RETURN_SUCCESS;
}

libarch_return_t
disass_branch_exception_sys_instruction (instruction_t *instr)
{
    unsigned op0 = select_bits (instr->opcode, 29, 31);
    unsigned op1 = select_bits (instr->opcode, 12, 25);
    unsigned op2 = select_bits (instr->opcode, 0, 4);

    if (op0 == 2 && (op1 >> 13) == 0) {
        decode_conditional_branch (&instr);

    } else if (op0 == 6 && (op1 >> 12) == 0) {
        decode_exception_generation (&instr);

    } else if (op0 == 6 && op1 == 0b01000000110001) {
        decode_system_instruction_with_register (&instr);

    } else if (op0 == 6 && op1 == 0b01000000110010 && op2 == 0b11111) {
        decode_hints (&instr);

    } else if (op0 == 6 && op1 == 0b01000000110011) {
        decode_barriers (&instr);

    } else if (op0 == 6 && (op1 & ~0x70) == 0x1004) {
        printf ("pstate\n");
        decode_pstate (&instr);

    } else if (op0 == 6 && (op1 >> 7) == 0b0100100) {
        printf ("system instruction w/ result\n");

    } else if (op0 == 6 && (((op1 >> 8) & ~2) == 0x11)) {
        printf ("system instruction\n");

    } else if (op0 == 6 && (((op1 >> 8) & ~2) == 0x11)) {
        printf ("system register move\n");

    } else if (op0 == 6 && ((op1 >> 13) == 1)) {
        printf ("unconditional branch (register)\n");

    } else if ((op0 & ~4) == 0) {
        printf ("unconditional branch (immediate)\n");

    } else if ((op0 & ~4) == 1) {
        if ((op1 >> 13) == 0)
            printf ("compare and branch (immediate)\n");
        else
            printf ("test and branch (immediate)\n");
    } else {
        // HINT
        instr->type = ARM64_INSTRUCTION_HINT;
    }
    return LIBARCH_RETURN_SUCCESS;
}