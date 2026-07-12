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

#include "gameframework/WorldManagerSubsystem.h"

#include <algorithm>

namespace gameframework
{

World *WorldManagerSubsystem::create(const std::string &name)
{
    // World's constructor is private (friend WorldManagerSubsystem only) —
    // std::make_unique can't reach it, since make_unique isn't itself the
    // friend, only this class is.
    std::unique_ptr<World> world(new World(name.empty() ? std::string("World") : name));
    World *ptr = world.get();
    ptr->initialize();

    worlds_owned_.push_back(std::move(world));
    world_order_.push_back(ptr);
    if (main_ == nullptr)
        main_ = ptr;

    return ptr;
}

void WorldManagerSubsystem::destroy(World *world)
{
    if (world == nullptr)
        return;

    auto it = std::find_if(worlds_owned_.begin(), worlds_owned_.end(),
                            [&](const std::unique_ptr<World> &w) { return w.get() == world; });
    if (it == worlds_owned_.end())
        return; // already destroyed / not ours — idempotent no-op

    world->dispose();

    world_order_.erase(std::remove(world_order_.begin(), world_order_.end(), world), world_order_.end());
    bool was_main = (main_ == world);
    worlds_owned_.erase(it); // world is destroyed here

    if (was_main)
        main_ = world_order_.empty() ? nullptr : world_order_.front();
}

void WorldManagerSubsystem::create_default_world()
{
    if (world_order_.empty())
        create("Main");
}

void WorldManagerSubsystem::deinitialize()
{
    // Reverse creation order, matching every other teardown-ordering
    // convention in this codebase (SubsystemManager::shutdown(),
    // World::dispose()'s own subsystem teardown).
    while (!worlds_owned_.empty())
        destroy(worlds_owned_.back().get());
}

} // namespace gameframework
