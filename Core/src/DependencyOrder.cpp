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

#include "gameframework/DependencyOrder.h"

#include <algorithm>
#include <unordered_map>

namespace gameframework
{

namespace
{
enum class VisitState
{
    Unvisited,
    Visiting,
    Done,
};
} // namespace

std::vector<size_t> DependencyOrder::sort_indices(
    const std::vector<std::type_index> &types,
    const std::function<std::vector<std::type_index>(size_t)> &get_depends_on)
{
    std::unordered_map<std::type_index, size_t> index_by_type;
    index_by_type.reserve(types.size());
    for (size_t i = 0; i < types.size(); i++)
        index_by_type[types[i]] = i;

    // Roots visited in a stable, type_index-ordered sequence first, for
    // determinism within a single run — matches Unity-Game-Framework's
    // DependencyOrder.cs intent (there, ordinal name sort; here, no stable
    // human-readable name exists without RTTI's own .name(), which isn't
    // portable/demangled consistently across compilers, so type_index's
    // own operator< — implementation-defined but consistent within one
    // process — is used instead).
    std::vector<size_t> root_order(types.size());
    for (size_t i = 0; i < types.size(); i++)
        root_order[i] = i;
    std::sort(root_order.begin(), root_order.end(),
              [&](size_t a, size_t b) { return types[a] < types[b]; });

    std::vector<VisitState> state(types.size(), VisitState::Unvisited);
    std::vector<size_t> result;
    result.reserve(types.size());

    std::function<void(size_t)> visit = [&](size_t i)
    {
        if (state[i] == VisitState::Done)
            return;
        if (state[i] == VisitState::Visiting)
            return; // cycle detected — break it here, item still comes out below
        state[i] = VisitState::Visiting;

        for (const std::type_index &dep_type : get_depends_on(i))
        {
            auto it = index_by_type.find(dep_type);
            if (it == index_by_type.end())
                continue; // unresolved dependency — skipped, not fatal
            visit(it->second);
        }

        state[i] = VisitState::Done;
        result.push_back(i);
    };

    for (size_t i : root_order)
        visit(i);

    return result;
}

} // namespace gameframework
