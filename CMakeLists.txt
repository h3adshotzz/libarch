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

# Include a file from config/defaults.cmake which has default values.

# The default build type should be DEBUG. The build type can be set with
# -DCMAKE_BUILD_TYPE.
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "DEBUG" CACHE STRING "Build type: RELEASE, DEBUG" FORCE)
endif()

############################## PROJECT #########################################

# Declare the project
project("Libarch" VERSION ${LIBARCH_VERSION} LANGUAGES C)

add_library(libarch SHARED)
set_property(TARGET libarch PROPERTY C_STANDARD 11)

# Public headers
target_include_directories(libarch
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Setup versioning script
if (USE_VERSION_GENERATOR)
    include (config/version.cmake)
    add_dependencies (libarch generate_version)
endif()

# Add Sources
add_subdirectory(src/)

# Add other directories
add_subdirectory(tools)
#add_subdirectory(tests)
