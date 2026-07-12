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

#include <memory>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "gameframework/Subsystem.h"
#include "gameframework/export.h"

namespace gameframework
{

class World;

/// <summary>
/// Owns every registered Subsystem (Global scope) and every World (which in
/// turn own their own World-scoped subsystems). This is the single, real
/// point of contact host applications/engine integrations need — an
/// ordinary C++ singleton (Meyer's singleton via instance(), defined once
/// in SubsystemManager.cpp so every consumer that links against this
/// library shares the exact same instance, verified empirically: two
/// separately-compiled, separately-dlopen'd consumer libraries both
/// registering into and reading from the same SubsystemManager::instance()).
///
/// register_subsystem() takes a type_name string purely as a stable
/// identity for dependency-ordering lookups (Subsystem::depends_on()) —
/// this is NOT reflection/RTTI-based discovery (no engine, no ClassDB, no
/// TypeCache equivalent exists or is needed here); each consumer explicitly
/// registers its own concrete subsystem instances, typically from its own
/// engine-integration-layer startup code.
/// </summary>
class GAMEFRAMEWORK_API SubsystemManager
{
public:
    static SubsystemManager &instance();

    /// Takes ownership. Identity for depends_on() lookups is the concrete
    /// subsystem's own typeid() — inferred automatically from *subsystem,
    /// never a hand-typed string.
    void register_subsystem(std::unique_ptr<Subsystem> subsystem);

    /// Convenience: constructs T in place and registers it.
    template <typename T, typename... Args>
    T &register_subsystem(Args &&...args)
    {
        auto owned = std::make_unique<T>(std::forward<Args>(args)...);
        T &ref = *owned;
        register_subsystem(std::move(owned));
        return ref;
    }

    /// Boots every registered Global subsystem that should_create()s, in
    /// depends_on() order (deterministic topological sort — see
    /// DependencyOrder.h). Idempotent; a second call is a no-op.
    void boot();
    /// Tears down every initialised Global subsystem in reverse boot order.
    void shutdown();

    int global_subsystem_count() const;
    Subsystem *get_global_subsystem(std::type_index type) const;
    template <typename T>
    T *get_global_subsystem() const
    {
        return static_cast<T *>(get_global_subsystem(std::type_index(typeid(T))));
    }
    /// Index-based access for iteration (dock/debug UI, tests) — order
    /// matches the boot order once booted, registration order beforehand.
    Subsystem *get_global_subsystem_at(int index) const;

private:
    SubsystemManager() = default;
    ~SubsystemManager();
    SubsystemManager(const SubsystemManager &) = delete;
    SubsystemManager &operator=(const SubsystemManager &) = delete;

    struct Entry
    {
        std::type_index type;
        std::unique_ptr<Subsystem> subsystem;
    };

    std::vector<Entry> entries_;
    bool booted_ = false;
};

} // namespace gameframework
