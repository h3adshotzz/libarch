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

#include <instruction.h>
#include <register.h>
#include <utils.h>


void check_op0 (unsigned int opcode)
{
    unsigned op0 = select_bits (opcode, 25, 28);
    printf ("checking 0x%llx: ", opcode);

    if (op0 == 0) printf ("reserved\n");
    else if (op0 > 0 && op0 <= 3) printf ("unknown\n");
    else if ((op0 >> 1) == 4) printf ("data processing immediate\n");
    else if ((op0 >> 1) == 5) printf ("branch, exception, system\n");
    else if ((op0 & ~10) == 4) printf ("load and store\n");
    else if ((op0 & ~8) == 5) printf ("data processing register\n");
    else if ((op0 & ~8) == 7) printf ("data processing floating\n");
}


void instruction_debug (instruction_t *instr, int show_fields)
{
    printf ("Parsed:            %s\n", instr->parsed);
    printf ("Opcode:            0x%08x\n",      instr->opcode);
    printf ("Decode Group:      %d\n",          instr->group);
    printf ("Instruction Type:  %d\n",          instr->type);
    printf ("Address:           0x%016x\n",     instr->addr);

    printf ("Operands:          %d\n", instr->operands_len);
    for (int i = 0; i < instr->operands_len; i++) {
        operand_t *op = &instr->operands[i];
        
        printf ("\t[%d]: type:          %d\n", i, op->op_type);
        if (op->op_type == ARM64_OPERAND_TYPE_REGISTER) {
            printf ("\t[%d]: reg:           %d\n", i, op->reg);
            printf ("\t[%d]: reg_size:      %d\n", i, op->reg_size);
            printf ("\t[%d]: reg_type:      %d\n", i, op->reg_type);
        } else if (op->op_type == ARM64_OPERAND_TYPE_IMMEDIATE) {
            printf ("\t[%d]: imm_type:      %d\n", i, op->imm_type);
            printf ("\t[%d]: imm_bits:      %d\n", i, op->imm_bits);
        } else if (op->op_type == ARM64_OPERAND_TYPE_SHIFT) {
            printf ("\t[%d]: shift_type:    %d\n", i, op->shift_type);
            printf ("\t[%d]: shift:         %d\n", i, op->shift);
        }
    }

    if (show_fields) {
        printf ("Fields:            %d\n", instr->fields_len);
        for (int i = 0; i < instr->fields_len; i++) {
            int f = &instr->fields[i];
            printf ("\t[%d]: field:         %d\n", i, f);
        }
    }
}

int main (int argc, char *argv[])
{
    printf (BLUE "\n    LIBARCH Version %s: %s; root:%s/%s_%s %s\n\n" RESET,
            LIBARCH_BUILD_VERSION, __TIMESTAMP__, LIBARCH_SOURCE_VERSION, LIBARCH_BUILD_TYPE, BUILD_ARCH_CAP, BUILD_ARCH);

    // 41880091 41880011
    instruction_t *test = libarch_instruction_create (0x91008041, 0);
    instruction_t *test2 = libarch_instruction_create (0x11008041, 0);
    libarch_disass (&test);
    libarch_disass (&test2);

    
    printf ("%d: %s\n", test->group, test->parsed);
    printf ("%d: %s\n\n", test2->group, test2->parsed);


    /* create the file descriptor */
    int fd = open (argv[1], O_RDONLY);

    /* calc the file size */
    struct stat st;
    fstat (fd, &st);
    size_t size = st.st_size;

    /* mmap the file */
    unsigned char *data = mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close (fd);

    /* verify the map was sucessful */
    if (data == MAP_FAILED) {
        printf ("shit\n");
        return 1;
    }
    uint32_t aligned_size = size / sizeof (uint32_t);
    printf ("aligned: %d\n", aligned_size);

    uint32_t *ins_data = malloc (aligned_size);
    for (int i = 0; i < aligned_size; i++) {
        ins_data[i] = *(uint32_t *) data;
        data += sizeof (uint32_t);
    }

    uint64_t base = 0x0;
    for (int i = 0; i < atoi(argv[2]); i++) {
        instruction_t *in = libarch_instruction_create (ins_data[i], 0);
        in->addr = base;
        libarch_disass (&in);

        printf ("0x%016x  %08x\t%s\n", in->addr, in->opcode, in->parsed);

        //instruction_debug (in, 0);
        //printf ("\n");

        base += sizeof (uint32_t);
    }

    return 0;
}