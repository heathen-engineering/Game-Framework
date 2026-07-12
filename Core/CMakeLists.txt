cmake_minimum_required(VERSION 3.16)
project(gameframework VERSION 0.1.0 LANGUAGES CXX)

# ── Options ──────────────────────────────────────────────────────────────
option(GAMEFRAMEWORK_BUILD_TESTS "Build the test suite" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(GAMEFRAMEWORK_SOURCES
    src/DependencyOrder.cpp
    src/SubsystemManager.cpp
)

# Each target compiles its own copy of the (tiny, currently 2-file) sources
# directly, rather than sharing a single OBJECT library between them —
# deliberately simpler than DataLens's static+shared-share-one-OBJECT-library
# pattern, which this repo's own CI caught failing under CMake's Xcode
# generator specifically: a STATIC target whose only input is
# $<TARGET_OBJECTS:...> (no real source files of its own) silently never got
# its final archive step invoked at all — no error, no warning, the
# libgameframework.a just never existed anywhere in the build tree. Direct
# compilation per target avoids that whole class of generator-specific edge
# case; the double-compile cost is negligible at this size.
function(gameframework_add_target name)
    cmake_parse_arguments(ARG "SHARED" "" "" ${ARGN})
    if(ARG_SHARED)
        add_library(${name} SHARED ${GAMEFRAMEWORK_SOURCES})
    else()
        add_library(${name} STATIC ${GAMEFRAMEWORK_SOURCES})
    endif()
    target_include_directories(${name} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_features(${name} PUBLIC cxx_std_17)
    set_target_properties(${name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(MSVC)
        target_compile_options(${name} PRIVATE /W4)
    else()
        target_compile_options(${name} PRIVATE -Wall -Wextra)
    endif()
endfunction()

# Static library — the primary target consumers (Godot/O3DE GDExtension-style
# integrations, tests) link against; ordinary static linking, no export/
# import decoration needed (GAMEFRAMEWORK_STATIC, see export.h).
gameframework_add_target(gameframework)
target_compile_definitions(gameframework PUBLIC GAMEFRAMEWORK_STATIC)
add_library(gameframework::gameframework ALIAS gameframework)

# Shared library — for consumers that need a real runtime dependency (e.g. a
# future C ABI layer for non-C++ hosts, matching DataLens's own
# static+shared split).
#
# The target (and OUTPUT_NAME) deliberately stay "gameframework_shared", NOT
# "gameframework" — on Windows/MSVC, a DLL target also generates an import
# .lib alongside the .dll (distinct from a .so's behavior on Linux/macOS,
# which needs no such stub), so giving this the same base name as the STATIC
# target above collides: both would try to produce gameframework.lib in the
# same output directory ("ninja: error: multiple rules generate
# gameframework.lib" — hit and fixed in this repo's first CI run).
gameframework_add_target(gameframework_shared SHARED)
target_compile_definitions(gameframework_shared PRIVATE GAMEFRAMEWORK_BUILDING)
add_library(gameframework::shared ALIAS gameframework_shared)

if(GAMEFRAMEWORK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(DIRECTORY include/ DESTINATION include)
install(TARGETS gameframework gameframework_shared
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
