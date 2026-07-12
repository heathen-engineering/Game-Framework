# Minimal, dependency-free smoke tests — no external test framework (keeps
# CI simple across 5 platforms; revisit if the suite grows enough to need
# Catch2-style fixtures/matchers).
add_executable(gameframework_tests smoke_test.cpp)
target_link_libraries(gameframework_tests PRIVATE gameframework::gameframework)
add_test(NAME gameframework_smoke COMMAND gameframework_tests)
