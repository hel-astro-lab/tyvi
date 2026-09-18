# Copyright 2025 - 2026, Miro Palmu and the tyvi contributors
# SPDX-License-Identifier: GPL-3.0-or-later

add_executable(bench
    bench.cpp
)

target_link_libraries(bench PRIVATE tyvi::tyvi)
target_compile_features(bench PUBLIC cxx_std_23)
set_target_properties(bench PROPERTIES VERSION ${PROJECT_VERSION})
