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

#include <string>
#include <typeindex>
#include <utility>
#include <vector>

#include "gameframework/export.h"

namespace gameframework
{

class World;

/// <summary>
/// Base type for every Game Framework subsystem — engine-agnostic C++,
/// ported from Unity-Game-Framework's Subsystem.cs (itself inspired by
/// Unreal Engine's Subsystem hierarchy). Deliberately NOT bound to any
/// engine's object/reflection system: this header is compiled directly
/// into every consumer's own binary (ordinary static linking against
/// libgameframework — see this repo's README for why, and
/// docs/godot-integration-notes.md for the specific GDExtension
/// cross-extension-registration limitation that ruled out an
/// engine-object-based design).
///
/// Constructors must be trivial and side-effect-free — SubsystemManager
/// constructs every candidate just to inspect scope()/depends_on()/
/// start_mode() before deciding whether to keep it; all real work happens
/// in initialize()/deinitialize().
/// </summary>
class GAMEFRAMEWORK_API Subsystem
{
public:
    enum class Scope
    {
        Global = 0,
        World = 1,
    };

    enum class StartMode
    {
        Disabled = 0,
        OnDemand = 1,
        Automatic = 2,
    };

    virtual ~Subsystem() = default;

    // Deliberately non-copyable — a Subsystem is always singly-owned via
    // unique_ptr (SubsystemManager/World never duplicate one), and staying
    // explicit about it matters beyond style here: a user-declared
    // destructor suppresses IMPLICIT move generation but NOT implicit copy
    // generation, so without this, Subsystem (and every subclass, e.g.
    // WorldManagerSubsystem, which owns a vector<unique_ptr<World>>) would
    // still get an implicitly-generated copy constructor/assignment that
    // tries to copy move-only members — compiles fine under some
    // toolchains' lazier instantiation, hard-fails under MSVC's stricter
    // one referencing unique_ptr's always-deleted copy-assignment (hit and
    // fixed in this repo's CI: the actual error pointed at
    // WorldManagerSubsystem.cpp/SubsystemManager.cpp, nowhere near the true
    // cause here in the base class).
    Subsystem(const Subsystem &) = delete;
    Subsystem &operator=(const Subsystem &) = delete;
    Subsystem() = default;

    /// Global unless overridden.
    virtual Scope scope() const { return Scope::Global; }

    /// Same-scope-only ordering dependencies, by real C++ type identity
    /// (typeid()) — deliberately not by hand-typed string name. e.g.
    /// { std::type_index(typeid(SomeOtherSubsystem)) }. This is
    /// compiler-checked (the type must actually exist and be spelled
    /// correctly to compile at all), unlike a hand-written string that
    /// could silently typo/mismatch across separately-compiled consumers.
    virtual std::vector<std::type_index> depends_on() const { return {}; }

    virtual StartMode start_mode() const { return StartMode::Automatic; }

    /// Default: start_mode() != Disabled. Override for more elaborate
    /// creation gating (e.g. platform checks).
    virtual bool should_create() const { return start_mode() != StartMode::Disabled; }

    bool is_initialised() const { return initialised_; }

    /// Internal — called by SubsystemManager/World only, idempotent.
    void do_initialize()
    {
        if (initialised_)
            return;
        initialised_ = true;
        initialize();
    }

    /// Internal — called by SubsystemManager/World only, idempotent.
    void do_deinitialize()
    {
        if (!initialised_)
            return;
        initialised_ = false;
        deinitialize();
    }

    World *world() const { return world_; }
    /// Internal — set by SubsystemManager/World before do_initialize() runs
    /// for a World-scoped subsystem; never set for Global ones.
    void set_world(World *w) { world_ = w; }

    // -- optional capabilities: default/empty, override only what you need.
    // A (label, value) pair list for a debug/inspector view. --
    virtual std::vector<std::pair<std::string, std::string>> debug_info() const { return {}; }
    /// A list of human-readable health issue strings. Empty means healthy.
    virtual std::vector<std::string> health_issues() const { return {}; }

protected:
    virtual void initialize() {}
    virtual void deinitialize() {}

private:
    World *world_ = nullptr;
    bool initialised_ = false;
};

} // namespace gameframework
