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
#include <functional>

namespace gameframework
{

/// <summary>
/// Opaque player identifier — a plain uint64_t wrapper, direct port of
/// Unity-Game-Framework's PlayerId.cs. Deliberately opaque/engine-agnostic:
/// a network layer or session manager mints ids in whatever space it likes
/// (a Steam ID, a socket-assigned sequence number, a UUID truncated to 64
/// bits, ...) — this type just gives every other piece of the framework a
/// single, comparable, hashable identity to key World::AddPlayer/
/// GetPlayer/RemovePlayer and PlayerState by, without caring what the
/// underlying value actually means.
/// </summary>
class PlayerId
{
public:
    static const PlayerId None;

    PlayerId() = default;
    explicit constexpr PlayerId(uint64_t value) : value_(value) {}

    constexpr uint64_t value() const { return value_; }
    constexpr bool is_valid() const { return value_ != 0; }

    constexpr bool operator==(const PlayerId &other) const { return value_ == other.value_; }
    constexpr bool operator!=(const PlayerId &other) const { return value_ != other.value_; }
    constexpr bool operator<(const PlayerId &other) const { return value_ < other.value_; }

private:
    uint64_t value_ = 0;
};

inline const PlayerId PlayerId::None = PlayerId();

} // namespace gameframework

namespace std
{
template <>
struct hash<gameframework::PlayerId>
{
    size_t operator()(const gameframework::PlayerId &id) const noexcept
    {
        return std::hash<uint64_t>{}(id.value());
    }
};
} // namespace std
