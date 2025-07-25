# Configures user linker for configured_target.
macro(TYVI_CONFIGURE_TARGET_LINKER configured_target)
    include(CheckCXXCompilerFlag)

    set(USER_LINKER_OPTION
        "lld"
        CACHE STRING "Linker to be used"
    )
    set(USER_LINKER_OPTION_VALUES
        "lld"
        "gold"
        "bfd"
        "mold"
    )
    set_property(CACHE USER_LINKER_OPTION PROPERTY STRINGS ${USER_LINKER_OPTION_VALUES})
    list(
        FIND
        USER_LINKER_OPTION_VALUES
        ${USER_LINKER_OPTION}
        USER_LINKER_OPTION_INDEX
    )

    if(${USER_LINKER_OPTION_INDEX} EQUAL -1)
        message(
            WARNING
                [[
Using custom linker: '${USER_LINKER_OPTION}'
Supported linkers: ${USER_LINKER_OPTION_VALUES}
]]
        )
    endif()

    if(NOT tyvi_ENABLE_USER_LINKER)
        return()
    endif()

    set(LINKER_FLAG "-fuse-ld=${USER_LINKER_OPTION}")

    check_cxx_compiler_flag(${LINKER_FLAG} CXX_SUPPORTS_USER_LINKER)
    if(CXX_SUPPORTS_USER_LINKER)
        target_compile_options(${configured_target} INTERFACE ${LINKER_FLAG})
    endif()
endmacro()
