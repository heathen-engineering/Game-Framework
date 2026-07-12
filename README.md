# Game Framework

Game Framework Core — the engine-agnostic C++17 heart of Heathen Engineering's
subsystem/world architecture, inspired by Unreal Engine's Subsystem hierarchy
(EngineSubsystem/GameInstanceSubsystem/WorldSubsystem/LocalPlayerSubsystem) and
previously implemented once already in C# for Unity
(`com.heathen.gameframework`, [Unity-Game-Framework](https://github.com/heathen-engineering/Unity-Game-Framework)).

This repo follows the same split as [DataLens](https://github.com/heathen-engineering/DataLens):
a standalone, engine-agnostic core, consumed by per-engine integration layers
(a thin Godot GDExtension wrapper first — [Godot-Game-Framework](https://github.com/heathen-engineering/Godot-Game-Framework) —
O3DE and others to follow).

## Why not bind directly to an engine's object/reflection system?

Two real, verified constraints ruled that out for the Godot integration
specifically, and the same reasoning generalizes to any engine host:

1. Godot's GDExtension `ClassDB` does not support a class in one extension
   inheriting from a `ClassDB`-registered class in a different extension —
   confirmed via a reproduced `ERROR: Unimplemented yet` at
   `_register_extension_class_internal`, and a Godot maintainer forum
   response calling this "not officially supported" (tracked as unresolved:
   [godot-proposals#13997](https://github.com/godotengine/godot-proposals/issues/13997)).
2. String/reflection-based cross-extension dispatch (the officially
   supported alternative — `Object::call(StringName, ...)`) has real,
   unnecessary performance overhead for something this foundational, and
   ties the core's design to one engine's object model.

Instead: `Subsystem`/`SubsystemManager`/`World` are plain C++ classes, not
bound to any engine's reflection system at all. Consumers link against this
library as an ordinary shared/static dependency — real C++ inheritance, real
vtable dispatch, zero string matching. Verified empirically: two separately
compiled, separately `dlopen`'d consumer libraries, both linked against the
same core, correctly share one `SubsystemManager::instance()` and correctly
dispatch through a shared `Subsystem*` base pointer to each other's
independently-defined concrete subclasses.

Dependency ordering (`Subsystem::depends_on()`) identifies types via
`std::type_index`/`typeid()` — real, compiler-checked C++ type identity —
never a hand-typed string name.

## Scope (current)

Global and World subsystem tiers only, matching the Unity original's own
scope (it doesn't have GameInstance/Player tiers either). Those are a
deliberately later addition here, not a gap being worked around.

## Structure

- `Core/include/gameframework/` — public headers.
- `Core/src/` — implementation.
- `Core/tests/` — a small, dependency-free smoke-test executable (no
  external test framework, keeps CI simple across five platforms).

## License

Apache 2.0 — see `LICENSE`.
