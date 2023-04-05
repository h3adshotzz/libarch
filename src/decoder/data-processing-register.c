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


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_data_processing_1_source (instruction_t **instr)
{
    unsigned sf = select_bits ((*instr)->opcode, 31, 31);
    unsigned S = select_bits ((*instr)->opcode, 29, 29);
    unsigned op2 = select_bits ((*instr)->opcode, 16, 20);
    unsigned Z = select_bits ((*instr)->opcode, 13, 13);
    unsigned op = select_bits ((*instr)->opcode, 10, 15);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */
    libarch_instruction_add_field (instr, sf);
    libarch_instruction_add_field (instr, S);
    libarch_instruction_add_field (instr, op2);
    libarch_instruction_add_field (instr, Z);
    libarch_instruction_add_field (instr, op);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rd);

    typedef struct opcode { unsigned sf, S, op2, op; arm64_instr_t type; } opcode;
    opcode opcode_table[] = {
        { 0, 0, 0, 0, ARM64_INSTRUCTION_RBIT },
        { 0, 0, 0, 1, ARM64_INSTRUCTION_REV16 },
        { 0, 0, 0, 2, ARM64_INSTRUCTION_REV },
        { 0, 0, 0, 4, ARM64_INSTRUCTION_CLZ },
        { 0, 0, 0, 5, ARM64_INSTRUCTION_CLS },

        { 1, 0, 0, 0, ARM64_INSTRUCTION_RBIT },
        { 1, 0, 0, 1, ARM64_INSTRUCTION_REV16 },
        { 1, 0, 0, 2, ARM64_INSTRUCTION_REV32 },
        { 1, 0, 0, 3, ARM64_INSTRUCTION_REV32 },
        { 1, 0, 0, 4, ARM64_INSTRUCTION_CLZ },
        { 1, 0, 0, 5, ARM64_INSTRUCTION_CLS },

        { 1, 0, 1, 0, ARM64_INSTRUCTION_PACIA },
        { 1, 0, 1, 1, ARM64_INSTRUCTION_PACIB },
        { 1, 0, 1, 2, ARM64_INSTRUCTION_PACDA },
        { 1, 0, 1, 3, ARM64_INSTRUCTION_PACDB },

        { 1, 0, 1, 4, ARM64_INSTRUCTION_AUTIA },
        { 1, 0, 1, 5, ARM64_INSTRUCTION_AUTIB },
        { 1, 0, 1, 6, ARM64_INSTRUCTION_AUTDA },
        { 1, 0, 1, 7, ARM64_INSTRUCTION_AUTDB },

        /* leave these out for now */
        // PACIZA, PACIZB, PACDZA, PACDZB, AUTIZA, AUTIZB, AUTDZA, AUTDZB, XPACI, XPACD

    };
    int table_size = sizeof (opcode_table) / sizeof (opcode);

    for (int i = 0; i < table_size; i++) {
        if (opcode_table[i].sf == sf && opcode_table[i].S == S && opcode_table[i].op2 == op2 && opcode_table[i].op == op) {
            (*instr)->type = opcode_table[i].type;

            libarch_instruction_add_operand_register (instr, Rn, (sf == 1) ? 64 : 32, opcode_table[i].type, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);

            if (opcode_table[i].type == ARM64_INSTRUCTION_PACIA) {
                if (Z == 1 && Rn == 0x1f) {
                    (*instr)->type = ARM64_INSTRUCTION_PACIZA;
                } else{
                    libarch_instruction_add_operand_register (instr, Rn, (sf == 1) ? 64 : 32, opcode_table[i].type, ARM64_REGISTER_OPERAND_OPT_NONE);
                }
            } else {
                libarch_instruction_add_operand_register (instr, Rn, (sf == 1) ? 64 : 32, opcode_table[i].type, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
            }
            break;
        }
    }
    return LIBARCH_DECODE_STATUS_SUCCESS;
}


LIBARCH_PRIVATE LIBARCH_API
decode_status_t
decode_logical_shift_register (instruction_t **instr)
{
    unsigned sf = select_bits ((*instr)->opcode, 31, 31);
    unsigned opc = select_bits ((*instr)->opcode, 29, 30);
    unsigned shift = select_bits ((*instr)->opcode, 22, 23);
    unsigned N = select_bits ((*instr)->opcode, 21, 21);
    unsigned Rm = select_bits ((*instr)->opcode, 16, 20);
    unsigned imm6 = select_bits ((*instr)->opcode, 10, 15);
    unsigned Rn = select_bits ((*instr)->opcode, 5, 9);
    unsigned Rd = select_bits ((*instr)->opcode, 0, 4);

    /* Add fields in left-right order */    
    libarch_instruction_add_field (instr, sf);
    libarch_instruction_add_field (instr, opc);
    libarch_instruction_add_field (instr, shift);
    libarch_instruction_add_field (instr, N);
    libarch_instruction_add_field (instr, Rm);
    libarch_instruction_add_field (instr, imm6);
    libarch_instruction_add_field (instr, Rn);
    libarch_instruction_add_field (instr, Rd);

    typedef struct { unsigned sf, opc, N; int width; arm64_instr_t type } opcode;
    opcode opcode_table[] = {
        { 0, 0, 0, 32, ARM64_INSTRUCTION_AND },
        { 0, 0, 1, 32, ARM64_INSTRUCTION_BIC },
        { 0, 1, 0, 32, ARM64_INSTRUCTION_ORR },
        { 0, 1, 1, 32, ARM64_INSTRUCTION_ORN },
        { 0, 2, 0, 32, ARM64_INSTRUCTION_EOR },
        { 0, 2, 1, 32, ARM64_INSTRUCTION_EON },
        { 0, 3, 0, 32, ARM64_INSTRUCTION_ANDS },
        { 0, 3, 1, 32, ARM64_INSTRUCTION_BICS },

        { 1, 0, 0, 64, ARM64_INSTRUCTION_AND },
        { 1, 0, 1, 64, ARM64_INSTRUCTION_BIC },
        { 1, 1, 0, 64, ARM64_INSTRUCTION_ORR },
        { 1, 1, 1, 64, ARM64_INSTRUCTION_ORN },
        { 1, 2, 0, 64, ARM64_INSTRUCTION_EOR },
        { 1, 2, 1, 64, ARM64_INSTRUCTION_EON },
        { 1, 3, 0, 64, ARM64_INSTRUCTION_ANDS },
        { 1, 3, 1, 64, ARM64_INSTRUCTION_BICS },
    };
    int table_size = sizeof (opcode_table) / sizeof (opcode);

    for (int i = 0; i < table_size; i++) {
        if (opcode_table[i].sf == sf && opcode_table[i].opc == opc && opcode_table[i].N == N) {
            int shift_table[] = { ARM64_SHIFT_TYPE_LSL, ARM64_SHIFT_TYPE_LSR, ARM64_SHIFT_TYPE_ASR, ARM64_SHIFT_TYPE_ROR };

            /* Check for the MOV alias */
            if ((shift == 0 && imm6 == 0 && Rn == 0x1f) && opcode_table[i].type == ARM64_INSTRUCTION_ORR) {
                (*instr)->type = ARM64_INSTRUCTION_MOV;

                libarch_instruction_add_operand_register (instr, Rd, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
                libarch_instruction_add_operand_register (instr, Rm, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);

            /* Check for the MVN alias */
            } else if (Rn == 0x1f && opcode_table[i].type == ARM64_INSTRUCTION_ORN) {
                (*instr)->type = ARM64_INSTRUCTION_MVN;

                libarch_instruction_add_operand_register (instr, Rd, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
                libarch_instruction_add_operand_register (instr, Rm, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
                libarch_instruction_add_operand_shift (instr, *(unsigned int *) &imm6, shift_table[shift]);

            /* Check for the TST alias */
            } else if (Rd == 0x1f && opcode_table[i].type == ARM64_INSTRUCTION_ANDS) {
                (*instr)->type = ARM64_INSTRUCTION_TST;

                libarch_instruction_add_operand_register (instr, Rn, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
                libarch_instruction_add_operand_register (instr, Rm, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
                libarch_instruction_add_operand_shift (instr, *(unsigned int *) &imm6, shift_table[shift]);

            /* The rest of the opcode_table instructions */
            } else {
                (*instr)->type = opcode_table[i].type;

                libarch_instruction_add_operand_register (instr, Rd, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
                libarch_instruction_add_operand_register (instr, Rn, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
                libarch_instruction_add_operand_register (instr, Rm, (sf == 1) ? 64 : 32, ARM64_REGISTER_TYPE_GENERAL, ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO);
                libarch_instruction_add_operand_shift (instr, *(unsigned int *) &imm6, shift_table[shift]);
            }
            break;
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
        decode_data_processing_1_source (&instr);
        printf ("data processing 1 source\n");

    } else if (op1 == 0 && (op2 >> 3) == 0) {
        decode_logical_shift_register (&instr);
        printf ("logical (shifted register)\n");
    }

    printf ("data processing register\n");
    return LIBARCH_DECODE_STATUS_SUCCESS;
}