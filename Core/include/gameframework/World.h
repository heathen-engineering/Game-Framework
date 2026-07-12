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
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "gameframework/PlayerId.h"
#include "gameframework/Subsystem.h"
#include "gameframework/export.h"

namespace gameframework
{

class GameState;
class GameMode;
class PlayerState;

/// <summary>
/// A pure in-memory context — a World, in the Unreal sense — with no ties
/// to any engine's scene/level concept. Direct port of
/// Unity-Game-Framework's World.cs. Doc note carried over verbatim because
/// it's the whole reason this type exists: most engines have no first-class
/// world object (a Godot SceneTree/Scene, like a Unity Scene, is a node
/// container, not 1:1 with a simulation world) — multiple Worlds can
/// coexist (a PauseWorld alongside a GameplayWorld, Player1World/
/// Player2World for split-screen, ...), each with its own independent set
/// of World-scoped Subsystems.
///
/// Owned and created exclusively by WorldManagerSubsystem — World itself
/// has no public constructor consumers call directly, matching the Unity
/// original's ownership model (WorldManagerSubsystem is just the first
/// ordinary consumer of the Subsystem abstraction, not a special case).
///
/// GameState/GameMode/PlayerState are optional Unreal-style structure on
/// top: State and Players live on World, not on GameMode, specifically so
/// they survive on a client that has no GameMode (GameMode is server-only
/// and never replicated — see GameMode.h/GameState.h's own doc comments).
/// </summary>
class GAMEFRAMEWORK_API World
{
public:
    World(const World &) = delete;
    World &operator=(const World &) = delete;
    ~World();

    const std::string &name() const { return name_; }
    bool is_alive() const { return alive_; }

    // -- World-scoped subsystems --
    Subsystem *get_subsystem(std::type_index type) const;
    template <typename T>
    T *get_subsystem() const
    {
        return static_cast<T *>(get_subsystem(std::type_index(typeid(T))));
    }
    int subsystem_count() const;
    Subsystem *get_subsystem_at(int index) const;

    // -- optional Unreal-style structure --
    GameState *state() const { return state_.get(); }
    GameState *set_state(std::unique_ptr<GameState> state);
    template <typename T, typename... Args>
    T &set_state(Args &&...args)
    {
        auto owned = std::make_unique<T>(std::forward<Args>(args)...);
        T &ref = *owned;
        set_state(std::move(owned));
        return ref;
    }

    GameMode *mode() const { return mode_.get(); }
    GameMode *set_mode(std::unique_ptr<GameMode> mode);
    template <typename T, typename... Args>
    T &set_mode(Args &&...args)
    {
        auto owned = std::make_unique<T>(std::forward<Args>(args)...);
        T &ref = *owned;
        set_mode(std::move(owned));
        return ref;
    }

    const std::vector<PlayerState *> &players() const { return player_order_; }
    PlayerState *add_player(PlayerId id, std::unique_ptr<PlayerState> player);
    template <typename T>
    T &add_player(PlayerId id)
    {
        auto owned = std::make_unique<T>();
        T &ref = *owned;
        add_player(id, std::move(owned));
        return ref;
    }
    bool remove_player(PlayerId id);
    PlayerState *get_player(PlayerId id) const;

private:
    friend class WorldManagerSubsystem;

    explicit World(std::string name);

    /// Brings up every World-scoped Subsystem in dependency order — called
    /// once by WorldManagerSubsystem::create() right after construction.
    void initialize();
    /// Tears down mode, players, state, then subsystems in reverse init
    /// order — called once by WorldManagerSubsystem::destroy().
    void dispose();

    std::string name_;
    bool alive_ = true;

    struct SubsystemEntry
    {
        std::type_index type;
        std::unique_ptr<Subsystem> subsystem;
    };
    std::vector<SubsystemEntry> subsystems_;

    std::unique_ptr<GameState> state_;
    std::unique_ptr<GameMode> mode_;
    std::unordered_map<uint64_t, std::unique_ptr<PlayerState>> players_by_id_;
    std::vector<PlayerState *> player_order_;
};

} // namespace gameframework
