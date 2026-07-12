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

#include "gameframework/SubsystemManager.h"

#include "gameframework/DependencyOrder.h"

namespace gameframework
{

SubsystemManager &SubsystemManager::instance()
{
    static SubsystemManager mgr;
    return mgr;
}

SubsystemManager::~SubsystemManager()
{
    shutdown();
}

void SubsystemManager::register_subsystem(std::unique_ptr<Subsystem> subsystem)
{
    std::type_index type(typeid(*subsystem));
    entries_.push_back(Entry{type, std::move(subsystem)});
}

void SubsystemManager::boot()
{
    if (booted_)
        return;
    booted_ = true;

    // Drop anything that shouldn't be created before ordering — matches
    // Unity-Game-Framework's GameFramework.Boot() sequence.
    std::vector<Entry> survivors;
    survivors.reserve(entries_.size());
    for (Entry &e : entries_)
    {
        if (e.subsystem->should_create())
            survivors.push_back(std::move(e));
    }
    entries_ = std::move(survivors);

    entries_ = DependencyOrder::sort<Entry>(
        std::move(entries_),
        [](const Entry &e) { return e.type; },
        [](const Entry &e) { return e.subsystem->depends_on(); });

    for (Entry &e : entries_)
        e.subsystem->do_initialize();
}

void SubsystemManager::shutdown()
{
    if (!booted_)
        return;
    booted_ = false;

    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it)
        it->subsystem->do_deinitialize();
}

int SubsystemManager::global_subsystem_count() const
{
    return int(entries_.size());
}

Subsystem *SubsystemManager::get_global_subsystem(std::type_index type) const
{
    for (const Entry &e : entries_)
    {
        if (e.type == type)
            return e.subsystem.get();
    }
    return nullptr;
}

Subsystem *SubsystemManager::get_global_subsystem_at(int index) const
{
    if (index < 0 || index >= int(entries_.size()))
        return nullptr;
    return entries_[size_t(index)].subsystem.get();
}

} // namespace gameframework
