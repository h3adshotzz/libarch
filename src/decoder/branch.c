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

}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_hints (instruction_t **instr)
{

}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_barriers (instruction_t **instr)
{

}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_pstate (instruction_t **instr)
{

}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_system_instruction (instruction_t **instr)
{

}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_system_register_move (instruction_t **instr)
{

}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_unconditional_branch_register (instruction_t **instr)
{

}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_unconditional_branch_immediate (instruction_t **instr)
{

}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_compare_and_branch_immediate (instruction_t **instr)
{

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

    } else if (op0 == 6 && (op1 & ~0x70) == 0x1004) {
        if (decode_pstate (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_PSTATE;

    } else if (op0 == 6 && ((op1 >> 7) & ~4) == 0x21) {
        if (decode_system_instruction (&instr))
            instr->subgroup = ARM64_DECODE_SUBGROUP_SYSTEM_INSTRUCTION;

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