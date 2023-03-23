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
        if ((opc == 1 || opc == 2) && ((LL == 3 || LL == 1) || (LL == 2 || LL == 3))) LIBARCH_RETURN_VOID;
        if (opc == 3 && (LL == 1 || LL == 2 || LL == 3)) LIBARCH_RETURN_VOID;
        if (opc == 5 || opc == 6 || opc == 7) LIBARCH_RETURN_VOID;
        if (opc == 5 && LL == 0) LIBARCH_RETURN_VOID;
    }
    if (op2) return LIBARCH_RETURN_VOID;

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
        printf ("system instruction w/ register\n");

    } else if (op0 == 6 && op1 == 0b01000000110010 && op2 == 0b11111) {
        printf ("hints\n");

    } else if (op0 == 6 && op1 == 0b01000000110011) {
        printf ("barriers\n");

    } else if (op0 == 6 && (op1 & ~0x70) == 0x1004) {
        printf ("pstate\n");

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
    }
    return LIBARCH_RETURN_SUCCESS;
}