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
#include "gameframework/PlayerId.h"
#include "gameframework/export.h"

namespace gameframework
{

class World;

/// <summary>
/// Optional per-player replicated state, keyed by PlayerId — plain data, no
/// logic, same Revision-as-replication-seam pattern as GameState (see that
/// header's doc comment for the full rationale). Direct port of
/// Unity-Game-Framework's PlayerState.cs.
///
/// Default authority is Owner (the owning client may write, replicated to
/// observers) rather than GameState's Server default — the framework only
/// declares this; enforcement is a future replication adapter's job.
/// </summary>
class GAMEFRAMEWORK_API PlayerState
{
public:
    virtual ~PlayerState() = default;

    PlayerId id() const { return id_; }
    /// Internal — set by World::add_player() only.
    void set_id(PlayerId id) { id_ = id; }

    virtual Authority authority() const { return Authority::Owner; }

    World *world() const { return world_; }
    /// Internal — set by World::add_player() only.
    void set_world(World *w) { world_ = w; }

    uint64_t revision() const { return revision_; }

protected:
    /// Call after mutating any field a replicator should care about.
    void mark_changed() { ++revision_; }

private:
    PlayerId id_;
    World *world_ = nullptr;
    uint64_t revision_ = 0;
};

} // namespace gameframework
