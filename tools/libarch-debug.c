//===----------------------------------------------------------------------===//
//
//                         === The LIBARCH Project ===
//
//  This  document  is the property of "Is This On?" It is considered to be
//  confidential and proprietary and may not be, in any form, reproduced or
//  transmitted, in whole or in part, without express permission of Is This
//  On?.
//
//  Copyright (C) 2022, Harry Moulton - Is This On? Holdings Ltd
//
//  Harry Moulton <me@h3adsh0tzz.com>
//
//===----------------------------------------------------------------------===//

#define DARK_WHITE      "\x1b[38;5;251m"
#define DARK_GREY       "\x1b[38;5;243m"
#define RED             "\x1b[38;5;88m"
#define BLUE            "\x1b[38;5;32m"
#define RESET           "\x1b[0m"
#define BOLD            "\x1b[1m"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <libarch.h>
#include <libarch-version.h>

#include <arm64/arm64-instructions.h>
#include <arm64/arm64-registers.h>
#include <arm64/arm64-misc.h>
#include <arm64/arm64-tlbi-ops.h>

#include <instruction.h>
#include <register.h>
#include <utils.h>


void instruction_debug (instruction_t *instr, int show_fields)
{
    printf ("Parsed:            %s\n",          instr->parsed);
    printf ("Opcode:            0x%08x\n",      instr->opcode);
    printf ("Decode Group:      %d\n",          instr->group);
    printf ("Instruction Type:  %s (%d)\n",     A64_INSTRUCTIONS_STR[instr->type], instr->type);
    printf ("Address:           0x%016x\n",     instr->addr);
    base10 (instr->opcode, 29);

    printf ("Operands:          %d\n", instr->operands_len);
    for (int i = 0; i < instr->operands_len; i++) {
        operand_t *op = &instr->operands[i];
        
        printf ("\t[%d]: type:          ", i);
        if (op->op_type == ARM64_OPERAND_TYPE_REGISTER) {
            printf ("REGISTER (%d)\n", op->op_type);
            printf ("\t[%d]: reg:           %d\n", i, op->reg);
            printf ("\t[%d]: reg_size:      %d\n", i, op->reg_size);
            printf ("\t[%d]: reg_type:      %d\n", i, op->reg_type);
        } else if (op->op_type == ARM64_OPERAND_TYPE_IMMEDIATE) {
            printf ("IMMEDIATE (%d)\n", op->op_type);
            printf ("\t[%d]: imm_type:      %d\n", i, op->imm_type);
            printf ("\t[%d]: imm_bits:      %d\n", i, op->imm_bits);
        } else if (op->op_type == ARM64_OPERAND_TYPE_SHIFT) {
            printf ("SHIFT (%d)\n", op->op_type);
            printf ("\t[%d]: shift_type:    %d\n", i, op->shift_type);
            printf ("\t[%d]: shift:         %d\n", i, op->shift);
        } else if (op->op_type == ARM64_OPERAND_TYPE_TARGET) {
            printf ("TARGET (%d)\n", op->op_type);
            printf ("\t[%d]: target:        %s\n", i, op->target);
        } else if (op->op_type == ARM64_OPERAND_TYPE_PSTATE) {
            printf ("PSTATE (%d)\n", op->op_type);
            printf ("\t[%d]: pstate:        %d\n", i, op->extra);
        } else if (op->op_type == ARM64_OPERAND_TYPE_AT_NAME) {
            printf ("AT NAME (%d)\n", op->op_type);
            printf ("\t[%d]: pstate:        %d\n", i, op->extra);
        } else if (op->op_type == ARM64_OPERAND_TYPE_TLBI_OP) {
            printf ("TLBI (%d)\n", op->op_type);
            printf ("\t[%d]: pstate:        %d\n", i, op->extra);
        }
    }

    if (show_fields) {
        printf ("Fields:            %d\n", instr->fields_len);
        for (int i = 0; i < instr->fields_len; i++) {
            int f = instr->fields[i];
            printf ("\t[%d]: field:         %d\n", i, f);
        }
    }
    printf ("\n");
}

void create_string (instruction_t *instr)
{
    /* Handle Mnemonic */
    char *mnemonic = A64_INSTRUCTIONS_STR[instr->type];
    if (instr->cond != -1) printf ("%s.%s\t", mnemonic, A64_CONDITIONS_STR[instr->cond]);
    else if (instr->spec != -1) printf ("%s.%s\t", mnemonic, A64_VEC_SPECIFIER_STR[instr->spec]);
    else printf ("%s\t", mnemonic);


    /* Handle Operands */
    for (int i = 0; i < instr->operands_len; i++) {
        operand_t *op = &instr->operands[i];

        /* Register */
        if (op->op_type == ARM64_OPERAND_TYPE_REGISTER) {
            char *reg;

            if (op->reg_type == ARM64_REGISTER_TYPE_GENERAL) {
                reg = libarch_get_general_register (op->reg,
                    (op->reg_size == 64) ? A64_REGISTERS_GP_64 : A64_REGISTERS_GP_32,
                    (op->reg_size == 64) ? A64_REGISTERS_GP_64_LEN : A64_REGISTERS_GP_32_LEN);
            } else if (op->reg_type == ARM64_REGISTER_TYPE_SYSTEM) {
                reg = libarch_get_system_register (op->reg);
            } else if (op->reg_type == ARM64_REGISTER_TYPE_FLOATING_POINT) {
                if (op->reg_size == 128) reg = libarch_get_general_register (op->reg, A64_REGISTERS_FP_128, A64_REGISTERS_FP_128_LEN); 
            } // ... other types


            // Print the register
            if (op->reg_prefix && !op->reg_suffix)
                printf ("%c%s", op->reg_prefix, reg);
            else if (!op->reg_prefix && op->reg_suffix)
                printf ("%s%c", reg, op->reg_suffix);
            else if (op->reg_prefix && op->reg_suffix)
                printf ("%c%s%c", op->reg_prefix, reg, op->reg_suffix);
            else
                printf ("%s", reg);
            goto check_comma;
        }

        /* Immediate */
        if (op->op_type == ARM64_OPERAND_TYPE_IMMEDIATE) {
            if (op->imm_type == ARM64_IMMEDIATE_TYPE_SYSC)
                printf ("c%d", op->imm_bits);
            else if (op->imm_type == ARM64_IMMEDIATE_TYPE_SYSS)
                printf ("s%d", op->imm_bits);
            else if (instr->type == ARM64_INSTRUCTION_SYS || instr->type == ARM64_INSTRUCTION_SYSL)
                printf ("%d", op->imm_bits);
            else {
                if ((op->imm_type & ARM64_IMMEDIATE_FLAG_OUTPUT_DECIMAL) == ARM64_IMMEDIATE_FLAG_OUTPUT_DECIMAL)
                    printf ("#%d", op->imm_bits);
                else if (op->imm_type == ARM64_IMMEDIATE_TYPE_LONG || op->imm_type == ARM64_IMMEDIATE_TYPE_ULONG)
                    printf ("0x%llx", op->imm_bits);
                else
                    printf ("0x%x", op->imm_bits);
            }
            
            goto check_comma;
        }

        /* Shift */
        if (op->op_type == ARM64_OPERAND_TYPE_SHIFT) {
            char *shift;

            if (op->shift_type == ARM64_SHIFT_TYPE_LSL) shift = "lsl";
            else if (op->shift_type == ARM64_SHIFT_TYPE_LSR) shift = "lsr";
            else if (op->shift_type == ARM64_SHIFT_TYPE_ASR) shift = "asr";
            else if (op->shift_type == ARM64_SHIFT_TYPE_ROR) shift = "ror";
            else if (op->shift_type == ARM64_SHIFT_TYPE_ROR) shift = "ror";
            else if (op->shift_type == ARM64_SHIFT_TYPE_MSL) shift = "msl";
            else continue;

            printf ("%s #%d", shift, op->shift);
            goto check_comma;
        }

        /* Target */
        if (op->op_type == ARM64_OPERAND_TYPE_TARGET) {
            if (op->target) printf ("%s", op->target);
            goto check_comma;
        }

        /* PSTATE */
        if (op->op_type == ARM64_OPERAND_TYPE_PSTATE) {
            printf ("%s", A64_PSTATE_STR[op->extra]);
        }

        /* Address Translate Name */
        if (op->op_type == ARM64_OPERAND_TYPE_AT_NAME) {
            printf ("%s", A64_AT_NAMES_STR[op->extra]);
        }

        /* TLBI Ops */
        if (op->op_type == ARM64_OPERAND_TYPE_TLBI_OP) {
            printf ("%s", A64_TLBI_OPS_STR[op->extra]);
        }

        /* Prefetch Operation */
        if (op->op_type == ARM64_OPERAND_TYPE_PRFOP) {
            printf ("%s", A64_PRFOP_STR[op->extra]);
        }

check_comma:
        if (i < instr->operands_len - 1) printf (", ");
    }
    printf ("\n");
}

void disassemble (uint32_t *data, uint32_t len, uint64_t base, int dbg)
{
    printf ("base: 0x%llx\n", base);
    for (int i = 0; i < len; i++) {
        //if (data[i] == NULL) continue;
        instruction_t *in = libarch_instruction_create (data[i], base);

        libarch_disass (&in);

        if (dbg) {
            instruction_debug (in, 1);
            create_string (in);
        }
        else printf ("0x%016llx  %08x\t%s\n", in->addr, in->opcode, in->parsed);

        base += 4;
    }
}

#define SWAP_INT(a)     ( ((a) << 24) | \
                        (((a) << 8) & 0x00ff0000) | \
                        (((a) >> 8) & 0x0000ff00) | \
                        ((unsigned int)(a) >> 24) )

int main (int argc, char *argv[])
{
    printf (BLUE "\n    LIBARCH Version %s: %s; root:%s/%s_%s %s\n\n" RESET,
            LIBARCH_BUILD_VERSION, __TIMESTAMP__, LIBARCH_SOURCE_VERSION, LIBARCH_BUILD_TYPE, BUILD_ARCH_CAP, BUILD_ARCH);

    if (argc == 2) {
        uint32_t input = SWAP_INT(strtol(argv[1], NULL, 16));
        uint32_t *opcode[] = { input, NULL };
        disassemble (opcode, 1, 0xfffffff00813e72c, 1);
    } else {
        printf ("disassembling %s\n", argv[1]);

        int fd = open (argv[1], O_RDONLY);
        struct stat st;
        fstat (fd, &st);
        size_t size = st.st_size;
        unsigned char *data = mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        close (fd);
        if (data == MAP_FAILED) {
            printf ("shit\n");
            return 1;
        }

        uint64_t base = 0xfffffff007b20000;
        for (int i = 0; i < atoi(argv[2]); i++) {

            uint32_t opcode = *(uint32_t *) (data + (i * 4));
            instruction_t *in = libarch_instruction_create (opcode, base);
            libarch_disass (&in);

            printf ("0x%016llx  %08x\t", in->addr, SWAP_INT(in->opcode));
            create_string (in);
            base += 4;

            /*//ins_data[i] = *(uint32_t *) data;
            instruction_t *in = libarch_instruction_create (*(uint32_t *) data, base);
            libarch_disass (&in);

            //printf ("0x%016llx  %08x\t%s\n", in->addr, in->opcode, in->parsed);
            printf ("0x%016llx  %08x\t", in->addr, in->opcode);
            create_string (in);

            data += sizeof (uint32_t);
            base += 4;*/
        }

        //uint64_t base = 0; //0x0000000100002d04;
        //disassemble (ins_data, atoi(argv[2]), base, atoi(argv[3]));
    }



    return 0;
}
