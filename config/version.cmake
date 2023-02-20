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

set(VERSION_CMD "${CMAKE_CURRENT_SOURCE_DIR}/config/version_generator.py")
set(MASTER_VERSION "${CMAKE_CURRENT_SOURCE_DIR}/src/MasterVersion")
set(TEMPLATE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config/")
set(OUTFILE "${CMAKE_CURRENT_SOURCE_DIR}/include/libarch-version.h")
set(BUILD_TYPE "${CMAKE_BUILD_TYPE}")


add_custom_target(generate_version
                DEPENDS ${VERSION_CMD}
                        ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

add_custom_command(TARGET generate_version
                PRE_BUILD
                COMMAND python3 ${VERSION_CMD} -m ${MASTER_VERSION} -o ${OUTFILE} -b ${BUILD_TYPE} -t ${TEMPLATE_DIR} -v
)
