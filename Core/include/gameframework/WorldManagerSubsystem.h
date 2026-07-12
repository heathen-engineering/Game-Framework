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
#include <string>
#include <vector>

#include "gameframework/Subsystem.h"
#include "gameframework/World.h"
#include "gameframework/export.h"

namespace gameframework
{

/// <summary>
/// Owns every World. An ordinary Global-scope Subsystem — deliberately not
/// special-cased, proving the Subsystem abstraction the same way
/// Unity-Game-Framework's WorldManagerSubsystem.cs does ("the World system
/// is therefore not a special case: it is the first consumer of the
/// subsystem structure").
///
/// SubsystemManager calls create_default_world() only after every Global
/// subsystem has finished initializing (see SubsystemManager::boot()) —
/// World-scoped subsystems can therefore assume the entire Global layer is
/// already up, matching the Unity original's documented ordering guarantee.
/// </summary>
class GAMEFRAMEWORK_API WorldManagerSubsystem : public Subsystem
{
public:
    const std::vector<World *> &worlds() const { return world_order_; }
    World *main() const { return main_; }

    World *create(const std::string &name = "");
    /// Idempotent — a second call with an already-destroyed/unknown World
    /// is a no-op.
    void destroy(World *world);

    /// Internal — called by SubsystemManager::boot() only, once, after all
    /// other Global subsystems have initialized. If no World exists yet,
    /// auto-creates one named "Main" — a single-world host needs zero
    /// explicit setup, a multi-world one calls create() itself before this
    /// runs (or just uses additional worlds alongside the auto-created one).
    void create_default_world();

protected:
    void deinitialize() override;

private:
    std::vector<std::unique_ptr<World>> worlds_owned_;
    std::vector<World *> world_order_;
    World *main_ = nullptr;
};

} // namespace gameframework
