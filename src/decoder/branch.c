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

#include "decoder/branch.h"
#include "utils.h"


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_conditional_branch (instruction_t **instr)
{
    unsigned o1 = select_bits ((*instr)->opcode, 24, 24);
    unsigned o0 = select_bits ((*instr)->opcode, 4, 4);
    unsigned imm19 = select_bits ((*instr)->opcode, 5, 23);
    unsigned cond = select_bits ((*instr)->opcode, 0, 3);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, o1);
    libarch_instruction_add_field (instr, o0);
    libarch_instruction_add_field (instr, imm19);
    libarch_instruction_add_field (instr, cond);

    /* B.cond */
    (*instr)->type = ARM64_INSTRUCTION_B;
    (*instr)->cond = cond;

    uint64_t imm = sign_extend(imm19 << 2, 64) + (*instr)->addr;
    libarch_instruction_add_operand_immediate (instr, *(unsigned long *) &imm, ARM64_IMMEDIATE_TYPE_ULONG);
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_exception_generation (instruction_t **instr)
{
    unsigned opc = select_bits ((*instr)->opcode, 21, 23);
    unsigned imm16 = select_bits ((*instr)->opcode, 5, 20);
    unsigned op2 = select_bits ((*instr)->opcode, 2, 4);
    unsigned LL = select_bits ((*instr)->opcode, 0, 1);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, opc);
    libarch_instruction_add_field (instr, imm16);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, LL);

    /* Create an opcode table */
    arm64_reg_t opcode_table[3][4] = {
        { ARM64_INSTRUCTION_SVC, ARM64_INSTRUCTION_HVC, ARM64_INSTRUCTION_SMC, 0 },
        { ARM64_INSTRUCTION_BRK, ARM64_INSTRUCTION_HLT, 0, 0 },
        { 0, ARM64_INSTRUCTION_DCPS1, ARM64_INSTRUCTION_DCPS2, ARM64_INSTRUCTION_DCPS3 },
    };

    printf ("opc: %d, LL: %d\n", opc, LL);

    /* Work out the correct instruction */
    if (opc >= 0 && opc <= 2) {
        if (LL >= 1 && LL <= 3) (*instr)->type = opcode_table[opc][LL - 1];
        else (*instr)->type = opcode_table[1][opc - 1];
    } else if (opc == 5 && LL >= 1 && LL <= 3) (*instr)->type = opcode_table[2][LL];
    else return LIBARCH_DECODE_STATUS_SOFT_FAIL;

    /* Add the operand */
    libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &imm16, ARM64_IMMEDIATE_TYPE_UINT);

    return LIBARCH_DECODE_STATUS_SUCCESS;
}



LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_system_instruction_with_register (instruction_t **instr)
{
    unsigned CRm = select_bits ((*instr)->opcode, 8, 11);
    unsigned op2 = select_bits ((*instr)->opcode, 5, 7);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, CRm);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, Rt);

    /* Determine instruction */
    arm64_reg_t opcode_table[2] = { ARM64_INSTRUCTION_WFET, ARM64_INSTRUCTION_WFIT };
    (*instr)->type = opcode_table[op2];

    libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_hints (instruction_t **instr)
{
    unsigned CRm = select_bits ((*instr)->opcode, 8, 11);
    unsigned op2 = select_bits ((*instr)->opcode, 5, 7);
    unsigned Z = select_bits ((*instr)->opcode, 13, 13);
    unsigned D = select_bits ((*instr)->opcode, 10, 10);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, CRm);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, Z);
    libarch_instruction_add_field (instr, D);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rd);

    /**
     *  NOTE:   To try and cut down the amount of if-else blocks, I decided to go with this
     *          approach of creating an array of all the instructions to parse in this
     *          subgroup, along with the CRm, op2 and D/Z values, and register operands,
     *          and iterate through it to determine which one it is.
     * 
     *          The original can be found here, and should be discussed as an optimisation
     *          in the final project report:
     *          https://github.com/h3adshotzz/libarch/blob/f0218715a29b36ee91e12e55724d19c6fbb8b00b/src/decoder/branch.c
    */
    typedef struct {
        unsigned CRm;
        unsigned op2;
        unsigned xtra;
        arm64_instr_t type;
        int Rd; // -1
        int Rn; // -1
    } opcode;

    
    /* opcode table */
    opcode opcode_table[] = {
        /* CRm == 0 */
        { 0, 0, -1, ARM64_INSTRUCTION_NOP, -1, -1 },
        { 0, 1, -1, ARM64_INSTRUCTION_YIELD, -1, -1 },
        { 0, 2, -1, ARM64_INSTRUCTION_WFE, -1, -1 },
        { 0, 3, -1, ARM64_INSTRUCTION_WFI, -1, -1 },
        { 0, 4, -1, ARM64_INSTRUCTION_SEV, -1, -1 },
        { 0, 5, -1, ARM64_INSTRUCTION_SEVL, -1, -1 },
        { 0, 6, -1, ARM64_INSTRUCTION_DGH, -1, -1 },
        { 0, 7, 0, ARM64_INSTRUCTION_XPACI, Rd, -1 },
        { 0, 7, 1, ARM64_INSTRUCTION_XPACD, Rd, -1 },

        /* CRm == 1 */
        { 1, 0, 0, ARM64_INSTRUCTION_PACIZA, Rd, Rn },
        { 1, 0, 1, ARM64_INSTRUCTION_PACIA1716, -1, -1 },
        { 1, 1, 0, ARM64_INSTRUCTION_PACIZA, Rd, Rn },
        { 1, 1, 1, ARM64_INSTRUCTION_PACIB1716, -1, -1 },
        { 1, 4, 0, ARM64_INSTRUCTION_AUTIZA, Rd, Rn },
        { 1, 4, 1, ARM64_INSTRUCTION_AUTIA1716, -1, -1 },
        { 1, 6, 0, ARM64_INSTRUCTION_AUTIZB, Rd, Rn },
        { 1, 6, 1, ARM64_INSTRUCTION_AUTIB1716, -1, -1 },

        /* CRm == 2 */
        { 2, 0, -1, ARM64_INSTRUCTION_ESB, -1, -1 },
        { 2, 1, -1, ARM64_INSTRUCTION_PSB_CSYNC, -1, -1 },
        { 2, 2, -1, ARM64_INSTRUCTION_TSB_CSYNC, -1, -1 },
        { 2, 4, -1, ARM64_INSTRUCTION_CSDB, -1, -1 },

        /* CRm == 3 */
        { 3, 0, -1, ARM64_INSTRUCTION_PACIAZ, -1, -1 },
        { 3, 1, -1, ARM64_INSTRUCTION_PACIASP, -1, -1 },
        { 3, 2, -1, ARM64_INSTRUCTION_PACIBZ, -1, -1 },
        { 3, 3, -1, ARM64_INSTRUCTION_PACIBSP, -1, -1 },
        { 3, 4, -1, ARM64_INSTRUCTION_AUTIAZ, -1, -1 },
        { 3, 5, -1, ARM64_INSTRUCTION_AUTIASP, -1, -1 },
        { 3, 6, -1, ARM64_INSTRUCTION_AUTIAZ, -1, -1 },
        { 3, 7, -1, ARM64_INSTRUCTION_AUTIASP, -1, -1 },

    };

    /* Only CRm == 0 instructions check 'D' */
    unsigned xtra = (CRm == 0) ? D : Z;

    /* Determine which one instruction it is */
    for (int i = 0; i < sizeof (opcode_table) / sizeof (opcode); i++) {
        if (opcode_table[i].CRm == CRm && opcode_table[i].op2 == op2 &&
        (opcode_table[i].xtra == -1 || opcode_table[i].xtra == xtra)) {
            (*instr)->type = opcode_table[i].type;

            if (opcode_table[i].Rd != -1)
                libarch_instruction_add_operand_register (instr, Rd, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

            if (opcode_table[i].Rn != -1)
                libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
        } 
    }

    /* BTI is annoying and is completely different to the others */
    if (CRm == 4 && (op2 & ~6) == 0) {
        (*instr)->type = ARM64_INSTRUCTION_BTI;

        const char *targets[] = { "", "c", "j", "jc" };
        libarch_instruction_add_operand_target (instr, targets[op2 >> 1]);
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_barriers (instruction_t **instr)
{
    unsigned CRm = select_bits ((*instr)->opcode, 8, 11);
    unsigned op2 = select_bits ((*instr)->opcode, 5, 7);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, CRm);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, Rt);

    /* If Rt doesn't equal 0x1f, is unallocated */
    if (Rt != 0b11111) return LIBARCH_DECODE_STATUS_SOFT_FAIL;

    /* CLREX */
    if (op2 == 2 && Rt == 0b11111) {
        (*instr)->type = ARM64_INSTRUCTION_CLREX;

        if (CRm < 15)
            libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &CRm, ARM64_IMMEDIATE_TYPE_UINT);

    /* DSB / DMB / ISB / SB / TCOMMIT */
    } else {

        /* DSB */
        if (op2 == 4) {
            (*instr)->type = ARM64_INSTRUCTION_DSB;
            libarch_instruction_add_operand_extra (instr, ARM64_OPERAND_TYPE_MEMORY_BARRIER, CRm);
        } else if (op2 == 5) {
            (*instr)->type = ARM64_INSTRUCTION_DMB;
        } else if (op2 == 6) {
            (*instr)->type = ARM64_INSTRUCTION_ISB;
        } else if (op2 == 7) {
            (*instr)->type = ARM64_INSTRUCTION_SB;
        } else if (op2 == 3) {
            (*instr)->type = ARM64_INSTRUCTION_TCOMMIT;
        }
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_pstate (instruction_t **instr)
{
    unsigned op1 = select_bits ((*instr)->opcode, 16, 18);
    unsigned CRm = select_bits ((*instr)->opcode, 8, 11);
    unsigned op2 = select_bits ((*instr)->opcode, 5, 7);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, op1);
    libarch_instruction_add_field (instr, CRm);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, Rt);

    unsigned imm = CRm;
    unsigned imm_01 = select_bits (CRm, 0, 1);

    /* CFINV */
    if (op1 == 0 && op2 == 0) {
        (*instr)->type = ARM64_INSTRUCTION_CFINV;
        (*instr)->parsed = "cfinv";
    
    /* XAFLAG */
    } else if (op1 == 0 && op2 == 1) {
        (*instr)->type = ARM64_INSTRUCTION_XAFLAG;
        (*instr)->parsed = "xaflag";

    /* AXFLAG */
    } else if (op1 == 0 && op2 == 2) {
        (*instr)->type = ARM64_INSTRUCTION_AXFLAG;
        (*instr)->parsed = "axflag";

    /* MSR (Immediate) */
    } else {
        (*instr)->type = ARM64_INSTRUCTION_MSR;

        /* PSTATE value table */
        int pstate_table[][5] = {
            { 0, 5, -1, ARM64_PSTATE_SPSEL, imm },
            { 3, 6, -1, ARM64_PSTATE_DAIFSET, imm },
            { 3, 7, -1, ARM64_PSTATE_DAIFCLR, imm },
            { 0, 3, -1, ARM64_PSTATE_UAO, imm },
            { 0, 4, -1, ARM64_PSTATE_PAN, imm },
            { 1, 0, 0, ARM64_PSTATE_ALLINT, imm_01 },
            { 3, 1, -1, ARM64_PSTATE_SSBS, imm },
            { 3, 2, -1, ARM64_PSTATE_DIT, imm },
            { 3, 3, 1, ARM64_PSTATE_SVCRSM, imm_01 },
            { 3, 3, 2, ARM64_PSTATE_SVCRSM, imm_01 },
            { 3, 3, 3, ARM64_PSTATE_SVCRSM, imm_01 },
            { 3, 4, -1, ARM64_PSTATE_TC0, imm },
        };

        for (int i = 0; i < sizeof (pstate_table) / sizeof (pstate_table[0]); i++) {
            if ((pstate_table[i][0] == op1 && pstate_table[i][1] == op2) &&
                (pstate_table[i][2] == -1 || pstate_table[i][2] == (CRm >> 1))) {
                libarch_instruction_add_operand_extra (instr, ARM64_OPERAND_TYPE_PSTATE, pstate_table[i][3]);
                libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &pstate_table[i][4], ARM64_IMMEDIATE_TYPE_UINT);
            }
        }
    }
    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_system_instruction (instruction_t **instr)
{
    unsigned L = select_bits ((*instr)->opcode, 21, 21);
    unsigned op1 = select_bits ((*instr)->opcode, 16, 18);
    unsigned CRn = select_bits ((*instr)->opcode, 12, 15);
    unsigned CRm = select_bits ((*instr)->opcode, 8, 11);
    unsigned op2 = select_bits ((*instr)->opcode, 5, 7);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, L);
    libarch_instruction_add_field (instr, op1);
    libarch_instruction_add_field (instr, CRn);
    libarch_instruction_add_field (instr, CRm);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, Rt);
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_system_register_move (instruction_t **instr)
{
    unsigned L = select_bits ((*instr)->opcode, 21, 21);
    unsigned o0 = select_bits ((*instr)->opcode, 19, 19);
    unsigned op1 = select_bits ((*instr)->opcode, 16, 18);
    unsigned CRn = select_bits ((*instr)->opcode, 12, 15);
    unsigned CRm = select_bits ((*instr)->opcode, 8, 11);
    unsigned op2 = select_bits ((*instr)->opcode, 5, 7);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, L);
    libarch_instruction_add_field (instr, o0);
    libarch_instruction_add_field (instr, op1);
    libarch_instruction_add_field (instr, CRn);
    libarch_instruction_add_field (instr, CRm);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, Rt);

    /* Calculate system register */
    unsigned sysreg = ((((((2 + o0) << 14) | (op1 << 11)) | (CRn << 7)) | (CRm << 3)) | op2);

    /* MSR (register) */
    if (L == 0) {
        (*instr)->type = ARM64_INSTRUCTION_MSR;

    /* MRS */
    } else {
        (*instr)->type = ARM64_INSTRUCTION_MRS;
        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
    }

    /* If the sysreg is recognised, then add the register as ARM64_REGISTER_TYPE_SYSTEM */
    if (libarch_get_system_register (sysreg)) {
        libarch_instruction_add_operand_register (instr, sysreg, 64, ARM64_REGISTER_TYPE_SYSTEM, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
    } else {
        /* S<op0>_<op1>_<Cn>_<Cm>_<op2> */
        libarch_instruction_add_operand_immediate (instr, (o0 == 0) ? 2 : 3, ARM64_IMMEDIATE_TYPE_SYSS);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &op1, ARM64_IMMEDIATE_TYPE_UINT);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &CRn, ARM64_IMMEDIATE_TYPE_SYSC);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &CRm, ARM64_IMMEDIATE_TYPE_SYSC);
        libarch_instruction_add_operand_immediate (instr, *(unsigned int *) &op2, ARM64_IMMEDIATE_TYPE_UINT);
    }

    /* For MSR, the Rt register comes after the system registers */
    if (L == 0)
        libarch_instruction_add_operand_register (instr, Rt, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_unconditional_branch_register (instruction_t **instr)
{
    unsigned opc = select_bits ((*instr)->opcode, 21, 24);
    unsigned op2 = select_bits ((*instr)->opcode, 16, 20);
    unsigned op3 = select_bits ((*instr)->opcode, 10, 15);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned op4 = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, opc);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, op3);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, op4);

    // Special bits for these instructions
    unsigned Z = select_bits ((*instr)->opcode, 24, 24);
    unsigned M = select_bits ((*instr)->opcode, 10, 10);
    unsigned Rm = op4;

    typedef struct {
        /* Instruction bits */
        int opc, op2, op3, Rn, op4, Rm;
        int Z, M;

        arm64_instr_t type;

        /* Register flags */
        int use_rn;
        int use_rm;
    } opcode;

    opcode opcode_table[] = {
        { 0, 0x1f, 0, -1, 0, -1, -1, -1, ARM64_INSTRUCTION_BR, 1, 0 },
        { 0, 0x1f, 2, -1, 0x1f, 0x1f, 0, 0, ARM64_INSTRUCTION_BRAAZ, 1, 0 },
        { 8, 0x1f, 2, -1, 0x1f, 0, 1, 0, ARM64_INSTRUCTION_BRAA, 1, 1 },
        { 0, 0x1f, 3, -1, 0x1f, 0x1f, 0, 1, ARM64_INSTRUCTION_BRABZ, 1, 0 },
        { 8, 0x1f, 3, -1, 0x1f, -1, 1, 1, ARM64_INSTRUCTION_BRAB, 1, 1 },

        { 1, 0x1f, 0, -1, 0, -1, -1, -1, ARM64_INSTRUCTION_BLR, 1, 0 },
        { 1, 0x1f, 2, -1, 0x1f, 0x1f, 0, 0, ARM64_INSTRUCTION_BLRAAZ, 1, 0 },
        { 1, 0x1f, 2, -1, 0x1f, -1, 1, 0, ARM64_INSTRUCTION_BLRAA, 1, 1 },
        { 1, 0x1f, 3, -1, 0x1f, 0x1f, 0, 1, ARM64_INSTRUCTION_BLRABZ, 1, 0 },
        { 1, 0x1f, 3, -1, 0x1f, -1, 1, 1, ARM64_INSTRUCTION_BLRAB, 1, 1 },

        { 2, 0x1f, 0, -1, 0, -1, -1, -1, ARM64_INSTRUCTION_RET, 1, 0 },
        { 2, 0x1f, 2, 0x1f, 0x1f, -1, -1, 0, ARM64_INSTRUCTION_RETAA, 0, 0 },
        { 2, 0x1f, 3, 0x1f, 0x1f, -1, -1, 1, ARM64_INSTRUCTION_RETAB, 0, 0 },

        { 4, 0x1f, 0, 0x1f, 0, -1, -1, -1, ARM64_INSTRUCTION_ERET, 0, 0 },
        { 4, 0x1f, 2, 0x1f, 0x1f, -1, -1, 0, ARM64_INSTRUCTION_ERETAA, 0, 0 },
        { 4, 0x1f, 3, 0x1f, 0x1f, -1, -1, 1, ARM64_INSTRUCTION_ERETAB, 0, 0 },

        { 5, 0x1f, 0, 0x1f, 0, -1, -1, -1, ARM64_INSTRUCTION_DRPS, 0, 0 },
    };

    for (int i = 0; i < sizeof (opcode_table) / sizeof (opcode_table[0]); i++) {
        if (opcode_table[i].opc == opc && opcode_table[i].op2 == op2 && opcode_table[i].op3 == op3 &&
            (opcode_table[i].Rn == -1 || opcode_table[i].Rn == Rn) &&
            opcode_table[i].op4 == 0 &&
            (opcode_table[i].Rm == -1 || opcode_table[i].Rm == Rm) &&
            (opcode_table[i].Z == -1 || opcode_table[i].Z == Z) &&
            (opcode_table[i].M == -1 || opcode_table[i].M == M)) {
            // blah
            (*instr)->type = opcode_table[i].type;

            printf ("use_rn: %d, use_rm: %d\n", opcode_table[i].use_rn, opcode_table[i].use_rm);

            /* Special check for 'ret' instruction */
            if (opcode_table[i].type == ARM64_INSTRUCTION_RET && Rn == 30) break;

            if (opcode_table[i].use_rn)
                libarch_instruction_add_operand_register (instr, Rn, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

            if (opcode_table[i].use_rm)
                libarch_instruction_add_operand_register (instr, Rm, 64, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);

            break;
        }
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_unconditional_branch_immediate (instruction_t **instr)
{
    unsigned op = select_bits ((*instr)->opcode, 31, 31);
    unsigned imm26 = select_bits ((*instr)->opcode, 0, 25);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, op);
    libarch_instruction_add_field (instr, imm26);

    /* B / BL */
    if (op == 0) (*instr)->type = ARM64_INSTRUCTION_B;
    else (*instr)->type = ARM64_INSTRUCTION_BL;

    /* Extend the pc-relative immediate value */
    long label = (signed) sign_extend (imm26 << 2, 28) + (*instr)->addr;
    libarch_instruction_add_operand_immediate (instr, *(long *) &label, ARM64_IMMEDIATE_TYPE_LONG);

    return LIBARCH_RETURN_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_compare_and_branch_immediate (instruction_t **instr)
{
    unsigned sf = select_bits ((*instr)->opcode, 31, 31);
    unsigned op = select_bits ((*instr)->opcode, 24, 24);
    unsigned imm19 = select_bits ((*instr)->opcode, 5, 23);
    unsigned Rt = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, sf);
    libarch_instruction_add_field (instr, op);
    libarch_instruction_add_field (instr, imm19);
    libarch_instruction_add_field (instr, Rt);

    /* Determine instruction size, and register width */
    uint32_t len;
    unsigned size;
    const char **regs;

    if (sf == 1) _SET_64 (size, regs, len);
    else _SET_32 (size, regs, len);

    /* Extend the pc-relative immediate value */
    long label = (signed) sign_extend (imm19 << 2, 21) + (*instr)->addr;

    /* CBZ / CBNZ */
    if (op == 0) (*instr)->type = ARM64_INSTRUCTION_CBZ;
    else (*instr)->type = ARM64_INSTRUCTION_CBNZ;

    libarch_instruction_add_operand_register (instr, Rt, size, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_NONE);
    libarch_instruction_add_operand_immediate (instr, *(long *) &label, ARM64_IMMEDIATE_TYPE_LONG);

    return LIBARCH_RETURN_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_test_and_branch_immediate (instruction_t **instr)
{

}




///////////////////////////////////////////////////////////////////////////////

LIBARCH_API
decode_status_t
disass_branch_exception_sys_instruction (instruction_t *instr)
{
    unsigned op0 = select_bits (instr->opcode, 29, 31);
    unsigned op1 = select_bits (instr->opcode, 12, 25);
    unsigned op2 = select_bits (instr->opcode, 0, 4);

    if (op0 == 2 && (op1 >> 13) == 0) {
        if (decode_conditional_branch (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_CONDITIONAL_BRANCH;

    } else if (op0 == 6 && (op1 >> 12) == 0) {
        if (decode_exception_generation (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_EXCEPTION_GENERATION;

    } else if (op0 == 6 && op1 == 0b01000000110001) {
        if (decode_system_instruction_with_register (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_SYS_INSTRUCTION_WITH_REGISTER;

    } else if (op0 == 6 && op1 == 0b01000000110010 && op2 == 0b11111) {
        if (decode_hints (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_HINTS;

    } else if (op0 == 6 && op1 == 0b01000000110011) {
        if (decode_barriers (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_BARRIERS;

    } else if (op0 == 6 && ((op1 >> 7) & ~4) == 0x21) {
        if (decode_system_instruction (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_SYSTEM_INSTRUCTION;

    } else if (op0 == 6 && (op1 & ~0x70) == 0x1004) {
        if (decode_pstate (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_PSTATE;

    } else if (op0 == 6 && (((op1 >> 8) & ~2) == 0x11)) {
        if (decode_system_register_move (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_SYSTEM_REGISTER_MOVE;

    } else if (op0 == 6 && ((op1 >> 13) == 1)) {
        if (decode_unconditional_branch_register (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_UNCONDITIONAL_BRANCH_REGISTER;

    } else if ((op0 & ~4) == 0) {
        if (decode_unconditional_branch_immediate (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_UNCONDITIONAL_BRANCH_IMMEDIATE;

    } else if ((op0 & ~4) == 1) {
        if ((op1 >> 13) == 0) 
            if (decode_compare_and_branch_immediate (&instr))
                instr->subgroup = ARM64_DECODE_SUBGROUP_COMPARE_AND_BRANCH_IMMEDIATE;
        else
            if (decode_test_and_branch_immediate (&instr))
                instr->subgroup = ARM64_DECODE_SUBGROUP_TEST_AND_BRANCH_IMMEDIATE;
    } else {
        // HINT
        instr->type = ARM64_INSTRUCTION_HINT;
    }
    return LIBARCH_DECODE_STATUS_SUCCESS;
}