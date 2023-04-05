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

#include "decoder/data-processing-register.h"

LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_data_processing_2_source (instruction_t **instr)
{
    unsigned sf = select_bits ((*instr)->opcode, 31, 31);
    unsigned S = select_bits ((*instr)->opcode, 29, 29);
    unsigned Rm = select_bits ((*instr)->opcode, 16, 20);
    unsigned op = select_bits ((*instr)->opcode, 10, 15);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, sf);
    libarch_instruction_add_field (instr, S);
    libarch_instruction_add_field (instr, Rm);
    libarch_instruction_add_field (instr, op);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rd);

    typedef struct { unsigned sf, S, opcode; arm64_instr_t type; } opcode;

    opcode opcode_table[] = {
        { 0, 0, 2, ARM64_INSTRUCTION_UDIV },
        { 0, 0, 3, ARM64_INSTRUCTION_SDIV }, 
        { 0, 0, 8, ARM64_INSTRUCTION_LSLV },
        { 0, 0, 9, ARM64_INSTRUCTION_LSRV },
        { 0, 0, 10, ARM64_INSTRUCTION_ASRV },
        { 0, 0, 11, ARM64_INSTRUCTION_RORV },

        { 1, 0, 0, ARM64_INSTRUCTION_SUBP },
        { 1, 0, 2, ARM64_INSTRUCTION_UDIV },
        { 1, 0, 3, ARM64_INSTRUCTION_SDIV },
        { 1, 0, 4, ARM64_INSTRUCTION_IRG },
        { 1, 0, 5, ARM64_INSTRUCTION_GMI },
        { 1, 0, 8, ARM64_INSTRUCTION_LSLV },
        { 1, 0, 9, ARM64_INSTRUCTION_LSRV },
        { 1, 0, 10, ARM64_INSTRUCTION_ASRV },
        { 1, 0, 11, ARM64_INSTRUCTION_RORV },
        { 1, 0, 12, ARM64_INSTRUCTION_PACGA },
        { 1, 1, 0, ARM64_INSTRUCTION_SUBPS },
    };
    int table_size = sizeof (opcode_table) / sizeof (opcode);

    for (int i = 0; i < table_size; i++) {
        if (opcode_table[i].sf == sf && opcode_table[i].S == S && opcode_table[i].opcode == op) {
            (*instr)->type = opcode_table[i].type;

            libarch_instruction_add_operand_register (instr, Rd, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
            libarch_instruction_add_operand_register (instr, Rn, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
            libarch_instruction_add_operand_register (instr, Rm, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
        }
    }

    return LIBARCH_DECODE_STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

LIBARCH_API
decode_status_t
disass_data_processing_register_instruction (instruction_t *instr)
{
    unsigned op0 = select_bits (instr->opcode, 30, 30);
    unsigned op1 = select_bits (instr->opcode, 28, 28);
    unsigned op2 = select_bits (instr->opcode, 21, 24);
    unsigned op3 = select_bits (instr->opcode, 10, 15);

    if (op0 == 0 && op1 == 1 && op2 == 6) {
        decode_data_processing_2_source (&instr);
        printf ("data processing 2 source\n");

    } else if (op0 == 1 && op1 == 1 && op2 == 6) {
        printf ("data processing 1 source\n");

    } else if (op1 == 0 && (op2 >> 3) == 0) {
        printf ("logical (shifted register)\n");
    }

    printf ("data processing register\n");
    return LIBARCH_DECODE_STATUS_SUCCESS;
}