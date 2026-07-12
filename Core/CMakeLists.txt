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

# ── Core objects (compiled once, shared by the static and shared libs) ────
add_library(gameframework_objects OBJECT
    src/DependencyOrder.cpp
    src/SubsystemManager.cpp
)
target_include_directories(gameframework_objects PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(gameframework_objects PUBLIC cxx_std_17)
set_target_properties(gameframework_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    target_compile_options(gameframework_objects PRIVATE /W4)
else()
    target_compile_options(gameframework_objects PRIVATE -Wall -Wextra)
endif()

# Static library — the primary target consumers (Godot/O3DE GDExtension-style
# integrations, tests) link against; ordinary static linking, no export/
# import decoration needed (GAMEFRAMEWORK_STATIC, see export.h).
add_library(gameframework STATIC $<TARGET_OBJECTS:gameframework_objects>)
target_link_libraries(gameframework PUBLIC gameframework_objects)
target_compile_definitions(gameframework PUBLIC GAMEFRAMEWORK_STATIC)
add_library(gameframework::gameframework ALIAS gameframework)

# Shared library — for consumers that need a real runtime dependency (e.g. a
# future C ABI layer for non-C++ hosts, matching DataLens's own
# static+shared split).
add_library(gameframework_shared SHARED $<TARGET_OBJECTS:gameframework_objects>)
target_compile_definitions(gameframework_objects PRIVATE GAMEFRAMEWORK_BUILDING)
set_target_properties(gameframework_shared PROPERTIES
    OUTPUT_NAME gameframework
    POSITION_INDEPENDENT_CODE ON
)
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
