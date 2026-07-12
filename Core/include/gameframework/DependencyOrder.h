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
#include <typeindex>
#include <vector>

#include "gameframework/export.h"

namespace gameframework
{

/// <summary>
/// Deterministic DFS topological sort by declared dependency type identity
/// (std::type_index, never a hand-typed string — see Subsystem::depends_on's
/// doc comment) — direct port of Unity-Game-Framework's DependencyOrder.cs.
/// Roots are sorted by std::type_index's own ordering first (stable,
/// implementation-defined but consistent within a single process — good
/// enough for "deterministic within one run," which is all the ordering
/// guarantee actually requires) for run-to-run determinism within a
/// session. A dependency not present in 'items' is silently skipped (the
/// caller's responsibility to log, not this pure utility's). A cycle is
/// broken at the point it's detected (the involved items still come out,
/// in DFS order) rather than failing the whole sort.
/// </summary>
class GAMEFRAMEWORK_API DependencyOrder
{
public:
    /// 'get_type' and 'get_depends_on' are applied to each item in 'items'
    /// to build the graph. Returns 'items' reordered so every item appears
    /// after everything it depends on.
    template <typename T>
    static std::vector<T> sort(
        std::vector<T> items,
        const std::function<std::type_index(const T &)> &get_type,
        const std::function<std::vector<std::type_index>(const T &)> &get_depends_on)
    {
        std::vector<std::type_index> types;
        types.reserve(items.size());
        for (const T &item : items)
            types.push_back(get_type(item));

        std::vector<size_t> order = sort_indices(types, [&](size_t i) { return get_depends_on(items[i]); });

        std::vector<T> result;
        result.reserve(items.size());
        for (size_t idx : order)
            result.push_back(std::move(items[idx]));
        return result;
    }

private:
    /// Returns an ordering of indices [0, types.size()) such that every
    /// index appears after every index its depends_on() names resolve to.
    static std::vector<size_t> sort_indices(
        const std::vector<std::type_index> &types,
        const std::function<std::vector<std::type_index>(size_t)> &get_depends_on);
};

} // namespace gameframework
