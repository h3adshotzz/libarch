//===----------------------------------------------------------------------===//
//
//                             === Libarch ===
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

#ifndef LIBARCH_VERSION_H
#define LIBARCH_VERSION_H

/* LIBARCH Build Version, i.e. 3.1.2 */
#define LIBARCH_BUILD_VERSION_MAJOR       "1"
#define LIBARCH_BUILD_VERSION_MINOR       "0"
#define LIBARCH_BUILD_VERSION_REVISION    "0"
#define LIBARCH_BUILD_VERSION             "1.0.0"

/* LIBARCH Build Type, i.e. INTERNAL, BETA, RELEASE */
#define LIBARCH_BUILD_TYPE                "Debug"

/* LIBARCH Source Version, i.e. 312.56.12 */
#define LIBARCH_SOURCE_VERSION_MAJOR      "8"
#define LIBARCH_SOURCE_VERSION_MINOR      "59"

#define LIBARCH_SOURCE_VERSION            "libarch-100.8.59"

/**
 *  Define a Target type for LIBARCH to both display within the help
 *  and other version information texts, and to determine platform
 *  when it comes to specific libraries that are only defined on
 *  one platform.
 *
 */
#ifdef __APPLE__
#   define BUILD_TARGET         "darwin"
#   define BUILD_TARGET_CAP     "Darwin"
#else
#   define BUILD_TARGET         "linux"
#   define BUILD_TARGET_CAP     "Linux"
#endif


/**
 *  Define an Architecture type for LIBARCH to both display within
 *  the help and other version information texts, and to determine
 *  architecture when it comes to platform-specific operations.
 *
 */
#ifdef __x86_64__
#   define BUILD_ARCH           "x86_64"
#   define BUILD_ARCH_CAP       "X86_64"
#elif __arm__
#   define BUILD_ARCH           "arm"
#   define BUILD_ARCH_CAP       "ARM"
#elif __arm64__
#	define BUILD_ARCH			"arm64"
#   define BUILD_ARCH_CAP       "ARM64"
#endif

#define LIBARCH_PLATFORM_TYPE_APPLE_SILICON       1
#define LIBARCH_PLATFORM_TYPE_INTEL_GENUINE       2

#if defined(__arm64__) && defined(__APPLE__)
#   define LIBARCH_MACOS_PLATFORM_TYPE            LIBARCH_PLATFORM_TYPE_APPLE_SILICON
#else
#   define LIBARCH_MACOS_PLATFORM_TYPE            LIBARCH_PLATFORM_TYPE_INTEL_GENUINE
#endif

#endif /* __libarch_version_h__ */
