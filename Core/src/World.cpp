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

#include "gameframework/World.h"

#include "gameframework/GameMode.h"
#include "gameframework/GameState.h"
#include "gameframework/PlayerState.h"
#include "gameframework/SubsystemManager.h"

namespace gameframework
{

World::World(std::string name) : name_(std::move(name))
{
}

World::~World()
{
    dispose();
}

void World::initialize()
{
    std::vector<std::unique_ptr<Subsystem>> fresh = SubsystemManager::instance().create_world_subsystems();
    subsystems_.reserve(fresh.size());
    for (std::unique_ptr<Subsystem> &sys : fresh)
    {
        sys->set_world(this);
        std::type_index type(typeid(*sys));
        subsystems_.push_back(SubsystemEntry{type, std::move(sys)});
    }
    for (SubsystemEntry &e : subsystems_)
        e.subsystem->do_initialize();
}

void World::dispose()
{
    if (!alive_)
        return;
    alive_ = false;

    // Stop the mode first, while subsystems/state/players are all still
    // live, so on_stop() can still read them — matches
    // Unity-Game-Framework's World.DisposeInternal() ordering exactly.
    if (mode_)
        mode_->stop();
    mode_.reset();

    player_order_.clear();
    players_by_id_.clear();
    state_.reset();

    for (auto it = subsystems_.rbegin(); it != subsystems_.rend(); ++it)
        it->subsystem->do_deinitialize();
    subsystems_.clear();
}

Subsystem *World::get_subsystem(std::type_index type) const
{
    for (const SubsystemEntry &e : subsystems_)
    {
        if (e.type == type)
            return e.subsystem.get();
    }
    return nullptr;
}

int World::subsystem_count() const
{
    return int(subsystems_.size());
}

Subsystem *World::get_subsystem_at(int index) const
{
    if (index < 0 || index >= int(subsystems_.size()))
        return nullptr;
    return subsystems_[size_t(index)].subsystem.get();
}

GameState *World::set_state(std::unique_ptr<GameState> state)
{
    state_ = std::move(state);
    if (state_)
        state_->set_world(this);
    return state_.get();
}

GameMode *World::set_mode(std::unique_ptr<GameMode> mode)
{
    if (mode_)
        mode_->stop();
    mode_ = std::move(mode);
    if (mode_)
        mode_->start(this);
    return mode_.get();
}

PlayerState *World::add_player(PlayerId id, std::unique_ptr<PlayerState> player)
{
    auto existing = players_by_id_.find(id.value());
    if (existing != players_by_id_.end())
        return existing->second.get(); // no-op — already present, matches Unity original

    player->set_id(id);
    player->set_world(this);
    PlayerState *ptr = player.get();
    players_by_id_.emplace(id.value(), std::move(player));
    player_order_.push_back(ptr);

    if (mode_)
        mode_->notify_player_joined(ptr);

    return ptr;
}

bool World::remove_player(PlayerId id)
{
    auto it = players_by_id_.find(id.value());
    if (it == players_by_id_.end())
        return false;

    PlayerState *ptr = it->second.get();
    if (mode_)
        mode_->notify_player_left(ptr);

    for (auto order_it = player_order_.begin(); order_it != player_order_.end(); ++order_it)
    {
        if (*order_it == ptr)
        {
            player_order_.erase(order_it);
            break;
        }
    }
    players_by_id_.erase(it);
    return true;
}

PlayerState *World::get_player(PlayerId id) const
{
    auto it = players_by_id_.find(id.value());
    return it != players_by_id_.end() ? it->second.get() : nullptr;
}

} // namespace gameframework
