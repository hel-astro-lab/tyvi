# Copyright 2018 - 2026, Miro Palmu and the hel-astro-lab contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# Configures interprocedural optimizations for the project.
macro(TYVI_CONFIGURE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT output)
    if(result)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(WARNING "IPO is not supported: ${output}")
    endif()
endmacro()
