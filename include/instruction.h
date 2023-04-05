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

#ifndef __LIBARCH_INSTRUCTION_H__
#define __LIBARCH_INSTRUCTION_H__

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "libarch.h"
#include "arm64/arm64-instructions.h"
#include "arm64/arm64-registers.h"
#include "arm64/arm64-common.h"

/* General "None" flag */
#define ARM64_NONE                              0

/******************************************************************************
*       Operands
*******************************************************************************/

/* Register type flags */
#define ARM64_REGISTER_TYPE_GENERAL             1
#define ARM64_REGISTER_TYPE_FLOATING_POINT      2
#define ARM64_REGISTER_TYPE_SYSTEM              3
#define ARM64_REGISTER_TYPE_ZERO                4

/* Shift type flags */
#define ARM64_SHIFT_TYPE_LSL                    4
#define ARM64_SHIFT_TYPE_LSR                    5
#define ARM64_SHIFT_TYPE_ASR                    6
#define ARM64_SHIFT_TYPE_ROR                    7
#define ARM64_SHIFT_TYPE_ROR                    8
#define ARM64_SHIFT_TYPE_MSL                    9

/* Immediate type flags */
#define ARM64_IMMEDIATE_TYPE_INT                10
#define ARM64_IMMEDIATE_TYPE_UINT               11
#define ARM64_IMMEDIATE_TYPE_LONG               12
#define ARM64_IMMEDIATE_TYPE_ULONG              13
#define ARM64_IMMEDIATE_TYPE_FLOAT              14
#define ARM64_IMMEDIATE_TYPE_SYSC               15
#define ARM64_IMMEDIATE_TYPE_SYSS               16

#define ARM64_IMMEDIATE_FLAG_OUTPUT_DECIMAL     0xf0000000

/* Operand type flags */
#define ARM64_OPERAND_TYPE_REGISTER             17
#define ARM64_OPERAND_TYPE_SHIFT                18
#define ARM64_OPERAND_TYPE_IMMEDIATE            19
#define ARM64_OPERAND_TYPE_TARGET               20
#define ARM64_OPERAND_TYPE_PSTATE               21
#define ARM64_OPERAND_TYPE_AT_NAME              22
#define ARM64_OPERAND_TYPE_TLBI_OP              23
#define ARM64_OPERAND_TYPE_PRFOP                24
#define ARM64_OPERAND_TYPE_MEMORY_BARRIER       25
#define ARM64_OPERAND_TYPE_INDEX_EXTEND         26

/* Operand Options */
#define ARM64_REGISTER_OPERAND_OPT_NONE             0
#define ARM64_REGISTER_OPERAND_OPT_PREFER_ZERO      1

#define ARM64_IMMEDIATE_OPERAND_OPT_NONE            0
#define ARM64_IMMEDIATE_OPERAND_OPT_PREFER_DECIMAL  2
#define ARM64_IMMEDIATE_OPERAND_OPT_PRE_INDEXED     0xf0000000


/**
 *  \brief  Operand Structure.
 * 
 *          This structure represents all types of operands for an instruction
 *          by having an `op_type` to determine which group of values to look
 *          at.
 * 
 *          * Registers *
 *          An arm64_reg_t value, which is just a typedef'd integer, holds the
 *          actual register number, with the size determining whether it's an
 *          X, W, V, etc. The type can be a General, Floating Point, System or
 *          Zero register.
 * 
 *          Some registers need a prefix and/or suffix, e.g. stlrb	w8, [x9],
 *          so these characters can be set. These are set to NULL by default
 *          so when printing instructions, verify the prefix and suffix.
 * 
 *          * Shift and Immediate *
 *          Shift's and Immediates are simple, they have a type and a value.
 * 
 *          * Extra *
 *          Some operands are neither a Register, Shift or Immediate, so they
 *          can be set in the `extra` field.
 * 
 */
typedef struct operand_t
{
    /* Operand type */
    uint8_t             op_type;

    /* op_type == ARM64_OPERAND_TYPE_REGISTER */
    arm64_reg_t         reg;
    uint8_t             reg_size;
    uint8_t             reg_type;

    /* op_type == ARM64_OPERAND_TYPE_SHIFT */
    uint32_t            shift;
    uint8_t             shift_type;

    /* op_type == ARM64_OPERAND_TYPE_IMMEDIATE */
    uint64_t            imm_bits; 
    uint8_t             imm_type;
    uint32_t            imm_opts;

    /* op_type == ARM64_OPERAND_TYPE_TARGET */
    char               *target;
    
    /**
     *  op_type == ARM64_OPERAND_TYPE_PSTATE
     *  op_type == ARM64_OPERAND_TYPE_AT_NAME
     *  op_type == ARM64_OPERAND_TYPE_TLBI_OP
    */
    int                 extra;
    int                 extra_val;

    /* Prefix and Suffix */
    char                prefix;
    char                suffix;
    char                suffix_extra;

} operand_t;


/******************************************************************************
*       Instructions
*******************************************************************************/


/**
 *  \brief  Instruction Structure
 * 
 *          This structure represents an AArch64 32-bit-wide Instruction. The
 *          original 32-bit opcode is assigned to `opcode`, and the address of
 *          the instruction is assigned to `addr` (optional). The `parsed` string
 *          is unused and can be set by clients when disassembling a file or set
 *          of instructions.
 * 
 *          The decode group and subgroup can be assigned so it's easier to
 *          identify the type of instruction and how it's being decoded.
 * 
 *          The actual instruction, e.g. ARM64_INSTRUCTION_LDR, is assigned to
 *          `type`, any conditions (for branches) is set assigned to `cond` and
 *          if the instruction handles vectors, the arrangement specifier is
 *          assigned to `spec`.
 * 
 *          Each operand is appended to the `operands` array, and each bit field
 *          that is checked is added to `fields`.
 */
typedef struct instruction_t
{
    /* Fully decoded, parsed instruction */
    char               *parsed;
    uint32_t            opcode;
    uint64_t            addr;

    /* Decode Group and Instruction type */
    uint32_t            group;
    uint32_t            subgroup;
    arm64_instr_t       type;
    int                 cond;           // Branch condition
    int                 spec;           // Vector Arrangement Specifier

    /* Operands */
    operand_t          *operands;
    uint32_t            operands_len;

    /* Fields, left to right */
    uint64_t           *fields;
    uint32_t            fields_len;

        
} instruction_t;

/**
 *  \brief  Create a new instruction_t structure for a given opcode, and set
 *          it's address if required.
 * 
 *  \param      opcode      32-bit opcode for the instruction.
 *  \param      addr        Address of the instruction, default to 0 (optional).
 * 
 *  \return An initialised instruction_t for the given opcode, not disassembled.
 *  
 */
LIBARCH_EXPORT LIBARCH_API
instruction_t *
libarch_instruction_create (uint32_t opcode, uint64_t addr);


/**
 *  \brief  Disassemble a given instruction, populating the rest of the instruction
 *          structure.
 * 
 *  \param      instr       Instruction to disassemble.
 * 
 *  \return A libarch return code depending on the result of the disassembly
 *          operation.
 * 
 */
LIBARCH_EXPORT LIBARCH_API
decode_status_t
libarch_disass (instruction_t **instr);


/******************************************************************************
*       Instruction API
*******************************************************************************/

/**
 * 
 * 
 */
LIBARCH_EXPORT LIBARCH_API
libarch_return_t
libarch_instruction_add_operand_immediate (instruction_t **instr, 
                                           uint64_t bits, 
                                           uint8_t type,
                                           uint32_t opts);


/**
 * 
 * 
 */
LIBARCH_API
libarch_return_t
libarch_instruction_add_operand_immediate_with_fix (instruction_t **instr, 
                                                    uint64_t bits, 
                                                    uint8_t type, 
                                                    char prefix, 
                                                    char suffix);


/**
 * 
 * 
 */
LIBARCH_API
libarch_return_t
libarch_instruction_add_operand_immediate_with_fix_extra (instruction_t **instr, 
                                                          uint64_t bits, 
                                                          uint8_t type, 
                                                          char prefix, 
                                                          char suffix);                                                    


/**
 * 
 * 
 */
LIBARCH_EXPORT LIBARCH_API
libarch_return_t
libarch_instruction_add_operand_shift (instruction_t **instr, 
                                       uint32_t shift, 
                                       uint8_t type);


/**
 * 
 */
LIBARCH_API
libarch_return_t
libarch_instruction_add_operand_shift_with_fix (instruction_t **instr, 
                                                uint32_t shift, 
                                                uint8_t type, 
                                                char prefix, 
                                                char suffix);


/**
 * 
 * 
 */
LIBARCH_EXPORT LIBARCH_API
libarch_return_t
libarch_instruction_add_operand_register (instruction_t **instr, 
                                          arm64_reg_t a64reg, 
                                          uint8_t size, 
                                          uint8_t type, 
                                          uint32_t opts);


/**
 * 
 * 
 */
LIBARCH_EXPORT LIBARCH_API
libarch_return_t
libarch_instruction_add_operand_register_with_fix (instruction_t **instr, 
                                                   arm64_reg_t a64reg, 
                                                   uint8_t size, 
                                                   uint8_t type, 
                                                   char prefix, 
                                                   char suffix);


/**
 * 
 * 
 */
LIBARCH_EXPORT LIBARCH_API
libarch_return_t
libarch_instruction_add_operand_target (instruction_t **instr, 
                                        char *target);


/**
 * 
 * 
 */
LIBARCH_EXPORT LIBARCH_API
libarch_return_t
libarch_instruction_add_operand_extra (instruction_t **instr, 
                                       int type, 
                                       int val);


/**
 * 
 * 
 */
LIBARCH_EXPORT LIBARCH_API
libarch_return_t
libarch_instruction_add_operand_extra_with_fix (instruction_t **instr, 
                                                int type, 
                                                int val, 
                                                char prefix, 
                                                char suffix);


/**
 * 
 * 
 */
LIBARCH_EXPORT LIBARCH_API
libarch_return_t
libarch_instruction_add_field (instruction_t **instr, int field);


#endif /* __libarch_disassembler_h__ */
