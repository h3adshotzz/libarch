##===----------------------------------------------------------------------===//
##
##                                 Libarch
##
##  This  document  is the property of "Is This On?" It is considered to be
##  confidential and proprietary and may not be, in any form, reproduced or
##  transmitted, in whole or in part, without express permission of Is This
##  On?.
##
##  Copyright (C) 2022, Harry Moulton - Is This On? Holdings Ltd
##
##  Harry Moulton <me@h3adsh0tzz.com>
##
##===----------------------------------------------------------------------===//

cmake_minimum_required(VERSION 3.15)

############################ CONFIGURATION #####################################

target_include_directories(libarch
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(libarch
    PUBLIC
        libarch.c
        instruction.c
        register.c
        utils.c

        decoder/data-processing.c
        decoder/branch.c
        decoder/load-and-store.c
)
