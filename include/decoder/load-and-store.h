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

#ifndef __LIBARCH_DECODER__LOAD_AND_STORE_H__
#define __LIBARCH_DECODER__LOAD_AND_STORE_H__

#include <stdio.h>
#include <stdlib.h>

#include "instruction.h"
#include "register.h"

#include "arm64/arm64-vector-specifiers.h"
#include "arm64/arm64-instructions.h"
#include "arm64/arm64-translation.h"
#include "arm64/arm64-conditions.h"
#include "arm64/arm64-registers.h"
#include "arm64/arm64-pstate.h"
#include "arm64/arm64-common.h"

// tmp
#include "utils.h"


/**
 * 
 * 
 */
LIBARCH_EXPORT LIBARCH_API
decode_status_t
disass_load_and_store_instruction (instruction_t *instr);


#endif /* __libarch_decoder__load_and_store_h__ */