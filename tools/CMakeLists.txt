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

####################### GENERIC LIBARCH TESTS ############################

## Libarch Version Tool
##
add_executable(libarch-version)
target_sources(libarch-version PUBLIC libarch-version.c)
target_include_directories(libarch-version PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(libarch-version libarch)

## Libarch Debug Tool
##
add_executable(libarch-debug)
target_sources(libarch-debug PUBLIC libarch-debug.c)
target_include_directories(libarch-debug PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(libarch-debug libarch)
