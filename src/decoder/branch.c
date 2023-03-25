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

#include "arm64/arm64-misc.h"
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

        libarch_instruction_add_operand_target (instr, targets[ind]);

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

    unsigned imm = CRm;

    /* Unallocated */
    if (Rt != 0b11111) return LIBARCH_RETURN_VOID;

    // CFINV
    if (op1 == 0 && op2 == 0) {
        (*instr)->type = ARM64_INSTRUCTION_CFINV;
        (*instr)->parsed = "cfinv";
    
    // XAFLAG
    } else if (op1 == 0 && op2 == 1) {
        (*instr)->type = ARM64_INSTRUCTION_XAFLAG;
        (*instr)->parsed = "xaflag";

    // AXFLAG
    } else if (op1 == 0 && op2 == 2) {
        (*instr)->type = ARM64_INSTRUCTION_AXFLAG;
        (*instr)->parsed = "axflag";

    // MSR (Immediate)
    } else {
        // MSR (Immediate)
        (*instr)->type = ARM64_INSTRUCTION_MSR;
        arm64_pstate_t pstate;

        // Alias' SMSTART and SMSTOP are not implemented.

        // Calculate pstate
        if (op1 == 0 && op2 == 5) pstate = ARM64_PSTATE_SPSEL;
        else if (op1 == 3 && op2 == 6) pstate = ARM64_PSTATE_DAIFSET;
        else if (op1 == 3 && op2 == 7) pstate = ARM64_PSTATE_DAIFCLR;
        else if (op1 == 0 && op2 == 3) pstate = ARM64_PSTATE_UAO;
        else if (op1 == 0 && op2 == 4) pstate = ARM64_PSTATE_PAN;
        else if (op1 == 1 && op2 == 0 && (CRm >> 1) == 0) {
            pstate = ARM64_PSTATE_ALLINT;
            imm = select_bits (CRm, 0, 1);
        }
        else if (op1 == 3 && op2 == 1) pstate = ARM64_PSTATE_SSBS;
        else if (op1 == 3 && op2 == 2) pstate = ARM64_PSTATE_DIT;
        else if (op1 == 3 && op2 == 3) {
            if ((CRm >> 1) == 1) pstate = ARM64_PSTATE_SVCRSM;
            else if ((CRm >> 1) == 2) pstate = ARM64_PSTATE_SVCRZA;
            else if ((CRm >> 1) == 3) pstate = ARM64_PSTATE_SVCRSMZA;

            imm = select_bits (CRm, 0, 1);
        }
        else if (op1 == 3 && op2 == 4) pstate = ARM64_PSTATE_TC0;

        libarch_instruction_add_operand_pstate (instr, pstate);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imm, ARM64_IMMEDIATE_TYPE_UINT);

        mstrappend (&(*instr)->parsed,
            "msr    %s, #0x%x",
            A64_PSTATE_STR[pstate], imm);

    }

    return LIBARCH_RETURN_SUCCESS;
}

static libarch_return_t
decode_system_instruction (instruction_t **instr)
{
    unsigned L = select_bits ((*instr)->opcode, 21, 21);
    unsigned op1 = select_bits ((*instr)->opcode, 16, 18);
    unsigned CRn = select_bits ((*instr)->opcode, 12, 15);
    unsigned CRm = select_bits ((*instr)->opcode, 8, 11);
    unsigned op2 = select_bits ((*instr)->opcode, 5, 7);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    // SYS
    if (L == 0) {

        // AT
        if (CRn == 7 && (CRm == 8 || CRm == 9) && SysOp (op1, 0b0111, CRm, op2) == ARM64_SYSOP_AT) {
            (*instr)->type = ARM64_INSTRUCTION_AT;

            arm64_addr_trans_t at;
            if (op1 == 0 && select_bits (CRm, 0, 0) == 0) {
                if (op2 == 0) at = ARM64_AT_NAME_S1E1R;
                else if (op2 == 1) at = ARM64_AT_NAME_S1E1W;
                else if (op2 == 2) at = ARM64_AT_NAME_S1E0R;
                else if (op2 == 3) at = ARM64_AT_NAME_S1E0W;
            } else if (op1 == 0 && select_bits (CRm, 0, 0) == 1) {
                if (op2 == 0) at = ARM64_AT_NAME_S1E1RP;
                else if (op2 == 1) at = ARM64_AT_NAME_S1E1WP;
            } else if (op1 == 4 && select_bits (CRm, 0, 0) == 0) {
                if (op2 == 0) at = ARM64_AT_NAME_S1E2R;
                else if (op2 == 1) at = ARM64_AT_NAME_S1E2W;
                else if (op2 == 4) at = ARM64_AT_NAME_S12E1R;
                else if (op2 == 5) at = ARM64_AT_NAME_S12E1W;
                else if (op2 == 6) at = ARM64_AT_NAME_S12E0R;
                else if (op2 == 7) at = ARM64_AT_NAME_S12E0W;
            } else if (op1 == 6 && select_bits (CRm, 0, 0) == 0) {
                if (op2 == 0) at = ARM64_AT_NAME_S1E3R;
                else if (op2 == 1) at = ARM64_AT_NAME_S1E3W;
            }

            libarch_instruction_add_operand_at_name (instr, at);
            libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);

        // TLBI
        } else if ((CRn >> 1) == 4 && SysOp (op1, CRn, CRm, op2) == ARM64_SYSOP_TLBI) {
            (*instr)->type = ARM64_INSTRUCTION_TLBI;

            arm64_tlbi_op_t tlbi = get_tlbi (op1, CRn, CRm, op2);
            libarch_instruction_add_operand_tlbi_op (instr, tlbi);

            // Rt is optional
            if (Rt != 0b11111)
                libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);


        // SYS
        } else {
            (*instr)->type = ARM64_INSTRUCTION_SYS;

            /**
             *  The instructions CFP, CPP, DC, DVP and IC are left to default to
             *  'SYS'. They each have additional operands but aren't very common
             *  so to save the operand_t struct becoming a monster, just let them
             *  default to sys instructions.
            */

            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &op1, ARM64_IMMEDIATE_TYPE_UINT);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &CRn, ARM64_IMMEDIATE_TYPE_SYSC);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &CRm, ARM64_IMMEDIATE_TYPE_SYSC);
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &op2, ARM64_IMMEDIATE_TYPE_UINT);

            // Rt is optional
            if (Rt != 0b11111)
                libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        }

    // SYSL
    } else {
        (*instr)->type = ARM64_INSTRUCTION_SYSL;

        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &op1, ARM64_IMMEDIATE_TYPE_UINT);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &CRn, ARM64_IMMEDIATE_TYPE_SYSC);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &CRm, ARM64_IMMEDIATE_TYPE_SYSC);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &op2, ARM64_IMMEDIATE_TYPE_UINT);
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
        decode_pstate (&instr);

    } else if (op0 == 6 && ((op1 >> 7) & ~4) == 0x21) {
        printf ("system instruction: %d\n",
            decode_system_instruction (&instr));

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