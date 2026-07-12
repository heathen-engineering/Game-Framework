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

#include "gameframework/GameMode.h"

#include "gameframework/World.h"

namespace gameframework
{

GameState *GameMode::state() const
{
    return world_ ? world_->state() : nullptr;
}

const std::vector<PlayerState *> &GameMode::players() const
{
    static const std::vector<PlayerState *> empty;
    return world_ ? world_->players() : empty;
}

void GameMode::start(World *w)
{
    world_ = w;
    on_start();
    if (on_started)
        on_started();
}

void GameMode::stop()
{
    on_stop();
    if (on_stopped)
        on_stopped();
    world_ = nullptr;
}

void GameMode::notify_player_joined(PlayerState *player)
{
    on_player_joined_impl(player);
    if (on_player_joined)
        on_player_joined(player);
}

void GameMode::notify_player_left(PlayerState *player)
{
    on_player_left_impl(player);
    if (on_player_left)
        on_player_left(player);
}

} // namespace gameframework
