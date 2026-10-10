include_guard(GLOBAL)

# Pure target resolver, also used by configuration tests without native runners.
# Compiler capability is verified separately on the real configured toolchain.
function(novodex_resolve_physics_backend out_var system pointer_size test_override compiler frontend)
    if(test_override AND NOT (system STREQUAL "Windows" AND pointer_size EQUAL 4))
        message(FATAL_ERROR "NOVODEX_TEST_PORTABLE_WIN32 is only valid for Windows 32-bit test builds")
    endif()
    if(system STREQUAL "Windows" AND pointer_size EQUAL 4 AND NOT test_override)
        if(NOT (compiler STREQUAL "MSVC" OR
                (compiler STREQUAL "Clang" AND frontend STREQUAL "MSVC")))
            message(FATAL_ERROR "Win32 production x87 requires an MSVC-compatible compiler with x86 inline assembly and naked-function support; no portable fallback is selected")
        endif()
        set(${out_var} 1 PARENT_SCOPE)
    else()
        set(${out_var} 0 PARENT_SCOPE)
    endif()
endfunction()

function(novodex_check_x87_compiler)
    include(CheckCXXSourceCompiles)
    # Check the calling convention, naked bodies, FPU instructions and flag used
    # by the retained code, rather than trusting only the compiler's name.
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} /WX /arch:IA32 /Qfast_transcendentals")
    check_cxx_source_compiles("\n#if !defined(_M_IX86)\n#error x87 requires an x86 target\n#endif\n__declspec(naked) double __cdecl nx_x87_probe() {\n __asm { fld1 }\n __asm { fsqrt }\n __asm { ret }\n}\nint main() {\n unsigned short control_word;\n __asm { fnstcw control_word }\n return nx_x87_probe() != 1.0;\n}"
        NOVODEX_COMPILER_SUPPORTS_X87)
    if(NOT NOVODEX_COMPILER_SUPPORTS_X87)
        message(FATAL_ERROR "Win32 production x87 compiler capability check failed: x86 naked functions, inline x87 assembly and legacy flags are required; no portable fallback is selected")
    endif()
endfunction()

# Apply to every production dependency and private-source test in the directory
# tree. Legacy flags stay where they were; portable arithmetic is always strict.
function(novodex_portable_fp_options out_var)
    if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(options /fp:strict)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            list(APPEND options /clang:-ffp-contract=off)
            # Genuine vendor class allocators return null on failure. Preserve
            # their existing checked-new behavior instead of assuming success.
            list(APPEND options $<$<COMPILE_LANGUAGE:CXX>:/clang:-fcheck-new>)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        set(options -fno-fast-math -ffp-contract=off -frounding-math)
        if(CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|AppleClang)$")
            list(APPEND options $<$<COMPILE_LANGUAGE:CXX>:-fcheck-new>)
        endif()
    else()
        message(FATAL_ERROR "Portable strict floating-point options are not defined for ${CMAKE_CXX_COMPILER_ID}")
    endif()
    set(${out_var} "${options}" PARENT_SCOPE)
endfunction()
