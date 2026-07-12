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
#include <vector>

#include "gameframework/export.h"

namespace gameframework
{

class World;
class GameState;
class PlayerState;

/// <summary>
/// Optional per-World logic that dictates rules and flow — direct port of
/// Unity-Game-Framework's GameMode.cs. Server-only and never replicated
/// (unlike GameState/PlayerState, which live ON World specifically so they
/// survive on a client that has no GameMode at all — see GameState.h's doc
/// comment). No Authority property, because on a client GameMode doesn't
/// exist at all, so the concept doesn't apply the way it does to
/// data.
///
/// Dual API, matching the Unity original's own "per preference, both fire"
/// design: override the virtual on_*() hooks for the common
/// subclass-a-GameMode-type case, or attach std::function callbacks for
/// composition — both fire, in that order, for every event.
///
/// Constructors must be trivial — constructed via World::set_mode<T>(),
/// real setup happens in on_start().
/// </summary>
class GAMEFRAMEWORK_API GameMode
{
public:
    virtual ~GameMode() = default;

    World *world() const { return world_; }
    GameState *state() const;
    const std::vector<PlayerState *> &players() const;

    std::function<void()> on_started;
    std::function<void()> on_stopped;
    std::function<void(PlayerState *)> on_player_joined;
    std::function<void(PlayerState *)> on_player_left;

    /// Internal — called by World::set_mode() only.
    void start(World *w);
    /// Internal — called by World::set_mode() (replacing this mode) or
    /// World's own teardown only.
    void stop();
    /// Internal — called by World::add_player() only.
    void notify_player_joined(PlayerState *player);
    /// Internal — called by World::remove_player() only.
    void notify_player_left(PlayerState *player);

protected:
    virtual void on_start() {}
    virtual void on_stop() {}
    virtual void on_player_joined_impl(PlayerState *) {}
    virtual void on_player_left_impl(PlayerState *) {}

private:
    World *world_ = nullptr;
};

} // namespace gameframework
