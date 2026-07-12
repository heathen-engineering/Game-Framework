# Minimal, dependency-free smoke tests — no external test framework (keeps
# CI simple across 5 platforms; revisit if the suite grows enough to need
# Catch2-style fixtures/matchers). Separate executables per area, each its
# own process — SubsystemManager is a process-wide singleton, so keeping
# unrelated test areas in separate binaries avoids one area's registrations
# leaking into another's assertions.
add_executable(gameframework_tests smoke_test.cpp)
target_link_libraries(gameframework_tests PRIVATE gameframework::gameframework)
add_test(NAME gameframework_smoke COMMAND gameframework_tests)

add_executable(gameframework_world_tests world_test.cpp)
target_link_libraries(gameframework_world_tests PRIVATE gameframework::gameframework)
add_test(NAME gameframework_world COMMAND gameframework_world_tests)
