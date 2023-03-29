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

#ifndef __LIBARCH_ARM64_MISC_H__
#define __LIBARCH_ARM64_MISC_H__

#include <stdlib.h>

/* SysOp's */       /** NOTE: MOVE TO arm64-common.h */
enum {
    ARM64_SYSOP_AT = 0,
    ARM64_SYSOP_DC,
    ARM64_SYSOP_IC,
    ARM64_SYSOP_BRB,
    ARM64_SYSOP_TLBI,
    ARM64_SYSOP_SYS,
};




#endif /* __libarch_arm64_misc_h__ */
