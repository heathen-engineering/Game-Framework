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

namespace gameframework
{

/// <summary>
/// Declares who is authoritative over a piece of state — Server (only the
/// server may write, replicated read-only to others), Owner (the owning
/// client may write, replicated to observers), or Client (purely local,
/// never replicated). Direct port of Unity-Game-Framework's Authority.cs.
///
/// The framework only DECLARES authority; it enforces nothing and ships no
/// networking. An optional future replication/HLAPI adapter reads this to
/// decide ownership and replication direction — this stays a pure
/// declaration seam, not a mechanism, matching the Unity original's own
/// documented intent.
/// </summary>
enum class Authority
{
    Server = 0,
    Owner = 1,
    Client = 2,
};

} // namespace gameframework
