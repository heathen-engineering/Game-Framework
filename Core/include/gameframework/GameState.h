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

#include <cstdint>

#include "gameframework/Authority.h"
#include "gameframework/export.h"

namespace gameframework
{

class World;

/// <summary>
/// Optional per-World replicated game state — plain data, no logic, direct
/// port of Unity-Game-Framework's GameState.cs. Deliberately data-only "so
/// a replication layer only ever touches data" (the same load-bearing
/// split GameMode.h documents from the other side: state is data, mode is
/// logic).
///
/// Lives on World, not on GameMode, specifically so it survives on a
/// client that has no GameMode at all (GameMode is server-only and never
/// replicated; GameState is the thing a client actually receives).
///
/// revision() is the replication diff seam — bumped by mark_changed(),
/// meant for a future replicator to compare against a last-seen value and
/// decide whether to resend. The framework declares this seam; it does not
/// implement serialization or a replicator itself, and — per World.h's
/// design note — must not depend on any networking library to stay
/// genuinely engine/platform-agnostic.
/// </summary>
class GAMEFRAMEWORK_API GameState
{
public:
    virtual ~GameState() = default;

    virtual Authority authority() const { return Authority::Server; }

    World *world() const { return world_; }
    /// Internal — set by World::set_state() only.
    void set_world(World *w) { world_ = w; }

    uint64_t revision() const { return revision_; }

protected:
    /// Call after mutating any field a replicator should care about.
    void mark_changed() { ++revision_; }

private:
    World *world_ = nullptr;
    uint64_t revision_ = 0;
};

} // namespace gameframework
