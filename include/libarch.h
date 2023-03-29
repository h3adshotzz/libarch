//===----------------------------------------------------------------------===//
//
//                         === The HTool Project ===
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

#ifndef __LIBARCH_LIBARCH_H__
#define __LIBARCH_LIBARCH_H__

#include <stdlib.h>

/* General definitions to make the code nicer to read */
#define LIBARCH_API
#define LIBARCH_EXPORT      extern
#define LIBARCH_PRIVATE     static


typedef enum libarch_return_t
{
    LIBARCH_RETURN_FAILURE,
    LIBARCH_RETURN_SUCCESS,
    LIBARCH_RETURN_VOID
} libarch_return_t;

/**
 *  \brief  Instruction decode status. Inspiration from Capstone Engine, this
 *          is a bit cleaner and nicer to read than using libarch_return_t
 *          all of the decoder functions. 
 */
typedef enum decode_status_t
{
    LIBARCH_DECODE_STATUS_FAIL,
    LIBARCH_DECODE_STATUS_SOFT_FAIL,
    LIBARCH_DECODE_STATUS_SUCCESS,
} decode_status_t;


#endif /* __libarch_libarch_h__ */
