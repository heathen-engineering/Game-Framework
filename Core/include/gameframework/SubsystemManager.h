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

#include <functional>
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
    /// Does NOT destroy the registered instances or forget World-scope
    /// factories — boot() can be called again afterwards to bring
    /// everything back up in the same process. For host-application/engine
    /// shutdown, where registered subsystems' owning modules may be about
    /// to unload (e.g. a Godot GDExtension being dlclose'd), call
    /// release_all() instead/as well — see its own doc comment.
    void shutdown();

    /// Deinitializes (as shutdown() does) then actually destroys every
    /// registered Global subsystem instance and forgets every registered
    /// World-scope factory, instead of leaving them owned by this manager
    /// until its own static-storage-duration destructor runs.
    ///
    /// This matters specifically for consumers where a registered
    /// Subsystem's concrete type — and therefore its vtable — lives inside
    /// a dynamically-loaded module that can be unloaded before this
    /// process-wide singleton itself is destroyed (e.g. a Godot
    /// GDExtension .so). Godot dlcloses every extension's library as part
    /// of its own engine shutdown, which completes well before the C
    /// runtime's atexit-driven static destructor chain runs at actual
    /// process exit() — so if this singleton's own destructor is what
    /// first tries to destroy those Subsystem instances, it does so
    /// through an already-unloaded module's vtable. Confirmed with gdb:
    /// SIGSEGV in Subsystem::do_deinitialize()/~Subsystem() called from
    /// ~SubsystemManager() at libc exit(), on an instance registered by an
    /// extension that had already been dlclose'd.
    ///
    /// Call this from the LAST extension's uninitialize callback that's
    /// still guaranteed every registered subsystem's owning module is
    /// still loaded — for the Godot integration, that's
    /// Godot-Game-Framework's own uninitialize_foundation_gameframework_module(),
    /// since Godot calls every extension's terminator before dlclose-ing
    /// any of them.
    void release_all();

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

    /// Registers a World-scoped subsystem TYPE (not an instance) — every
    /// World gets its own freshly-constructed instance of every type
    /// registered here, created/destroyed with that World. This is the
    /// closest engine-agnostic equivalent to Unity's
    /// SubsystemDiscovery.TypesForScope(SubsystemScope.World) reflection
    /// scan: since C++ has no runtime type discovery, each consumer
    /// explicitly registers the World-scope types it provides (typically
    /// once, at the same startup point Global subsystems are registered).
    template <typename T>
    void register_world_subsystem_type()
    {
        register_world_subsystem_factory(
            std::type_index(typeid(T)),
            []() -> std::unique_ptr<Subsystem> { return std::make_unique<T>(); });
    }

    /// Internal — called by World::initialize() only. Constructs one fresh
    /// instance of every registered World-scope type that should_create()s,
    /// dependency-ordered (same DependencyOrder sort boot() uses, run
    /// against this freshly-constructed set — each instance's own
    /// depends_on() is what's actually consulted, not anything cached at
    /// registration time).
    std::vector<std::unique_ptr<Subsystem>> create_world_subsystems() const;

private:
    SubsystemManager() = default;
    ~SubsystemManager();
    SubsystemManager(const SubsystemManager &) = delete;
    SubsystemManager &operator=(const SubsystemManager &) = delete;

    void register_world_subsystem_factory(std::type_index type, std::function<std::unique_ptr<Subsystem>()> factory);

    struct Entry
    {
        std::type_index type;
        std::unique_ptr<Subsystem> subsystem;
    };

    struct WorldFactoryEntry
    {
        std::type_index type;
        std::function<std::unique_ptr<Subsystem>()> factory;
    };

    std::vector<Entry> entries_;
    std::vector<WorldFactoryEntry> world_factories_;
    bool booted_ = false;
};

} // namespace gameframework
