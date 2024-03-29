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

#ifndef __LIBARCH_REGISTER_H__
#define __LIBARCH_REGISTER_H__

#include "libarch.h"
#include "arm64/arm64-registers.h"

/**
 * \brief   Search the given register list for the specified register.
 * 
 * \param   reg     Register to search for.
 * \param   list    Register list to search in.
 * \param   len     Length of the register list.
 * 
 * \return  String representation for the specified `reg`.
 */
extern const char *
libarch_get_general_register (arm64_reg_t reg, const char **list, uint64_t len);


/**
 * \brief   Fetch the string for the specified system register.
 * 
 * \param   reg     System register to convert to string.
 * 
 * \return  String representation for the specified sytem `reg`.
*/
extern const char *
libarch_get_system_register (arm64_reg_t reg);



#endif /* __libarch_register_h__ */