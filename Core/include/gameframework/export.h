/*
 * Copyright (c) 2026 Heathen Engineering Limited
 * Irish Registered Company #556277
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

// GAMEFRAMEWORK_STATIC is defined by CMake when building/consuming the
// static library target (no export/import decoration needed at all —
// static linking resolves symbols at link time, not load time). The shared
// library target needs explicit dllexport/dllimport on Windows (MSVC has
// no default-visible-symbols equivalent to ELF/Mach-O), and explicit
// default visibility on Linux/macOS is good practice even though GCC/Clang
// default to it unless -fvisibility=hidden is passed.
#if defined(GAMEFRAMEWORK_STATIC)
    #define GAMEFRAMEWORK_API
#elif defined(_WIN32)
    #if defined(GAMEFRAMEWORK_BUILDING)
        #define GAMEFRAMEWORK_API __declspec(dllexport)
    #else
        #define GAMEFRAMEWORK_API __declspec(dllimport)
    #endif
#else
    #define GAMEFRAMEWORK_API __attribute__((visibility("default")))
#endif
