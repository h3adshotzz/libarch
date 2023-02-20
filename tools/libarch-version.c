//===----------------------------------------------------------------------===//
//
//                         === The LIBARCH Project ===
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

#define DARK_WHITE      "\x1b[38;5;251m"
#define DARK_GREY       "\x1b[38;5;243m"
#define RED             "\x1b[38;5;88m"
#define BLUE            "\x1b[38;5;32m"
#define RESET           "\x1b[0m"
#define BOLD            "\x1b[1m"

#include <stdio.h>
#include <libarch-version.h>

int main (int argc, char *argv[])
{
    printf (BOLD "Copyright (C) Is This On? Holdings Ltd.\n\n" RESET);

    printf (BOLD RED "  Debug Information:\n", RESET);
    printf (BOLD DARK_WHITE "    Build Version:    " RESET DARK_GREY "%s (%s)\n", LIBARCH_BUILD_VERSION, LIBARCH_SOURCE_VERSION);
    printf (BOLD DARK_WHITE "    Build Target:     " RESET DARK_GREY "%s-%s\n", BUILD_TARGET, BUILD_ARCH);        
    printf (BOLD DARK_WHITE "    Build time:       " RESET DARK_GREY "%s\n", __TIMESTAMP__);
    printf (BOLD DARK_WHITE "    Compiler:         " RESET DARK_GREY "%s\n", __VERSION__);

    printf (BOLD DARK_WHITE "    Platform:         " RESET DARK_GREY);
#if LIBARCH_MACOS_PLATFORM_TYPE == LIBARCH_PLATFORM_TYPE_APPLE_SILICON
    printf ("apple-silicon (Apple Silicon)\n");
#else
    printf ("intel-genuine (Intel Genuine)\n");
#endif
#if LIBARCH_DEBUG
    printf (BLUE "\n    LIBARCH Version %s: %s; root:%s/%s_%s %s\n\n" RESET,
            LIBARCH_BUILD_VERSION, __TIMESTAMP__, LIBARCH_SOURCE_VERSION, LIBARCH_BUILD_TYPE, BUILD_ARCH_CAP, BUILD_ARCH);
#endif
}