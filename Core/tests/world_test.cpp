#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#include "gameframework/GameMode.h"
#include "gameframework/GameState.h"
#include "gameframework/PlayerState.h"
#include "gameframework/SubsystemManager.h"
#include "gameframework/World.h"
#include "gameframework/WorldManagerSubsystem.h"

using namespace gameframework;

namespace
{

std::vector<std::string> *g_log = nullptr;

// A World-scoped subsystem — registered as a TYPE (via
// register_world_subsystem_type<T>()), not an instance, so every World gets
// its own.
class InventorySubsystem : public Subsystem
{
public:
    Scope scope() const override { return Scope::World; }

protected:
    void initialize() override { g_log->push_back("Inventory::init"); }
    void deinitialize() override { g_log->push_back("Inventory::deinit"); }
};

class MyGameState : public GameState
{
public:
    int score = 0;
    void add_score(int amount)
    {
        score += amount;
        mark_changed();
    }
};

class MyPlayerState : public PlayerState
{
public:
    std::string display_name;
};

class MyGameMode : public GameMode
{
protected:
    void on_start() override { g_log->push_back("MyGameMode::on_start"); }
    void on_stop() override { g_log->push_back("MyGameMode::on_stop"); }
    void on_player_joined_impl(PlayerState *) override { g_log->push_back("MyGameMode::joined"); }
    void on_player_left_impl(PlayerState *) override { g_log->push_back("MyGameMode::left"); }
};

} // namespace

int main()
{
    std::vector<std::string> log;
    g_log = &log;

    SubsystemManager &mgr = SubsystemManager::instance();
    mgr.register_subsystem<WorldManagerSubsystem>();
    mgr.register_world_subsystem_type<InventorySubsystem>();

    // boot() should auto-create a default World (named "Main") after
    // WorldManagerSubsystem itself initializes, and that World should have
    // already brought up InventorySubsystem.
    mgr.boot();

    WorldManagerSubsystem *wm = mgr.get_global_subsystem<WorldManagerSubsystem>();
    assert(wm != nullptr);
    assert(wm->worlds().size() == 1);
    assert(wm->main() == wm->worlds()[0]);
    assert(wm->main()->name() == "Main");

    World *world = wm->main();
    assert(world->subsystem_count() == 1);
    InventorySubsystem *inv = world->get_subsystem<InventorySubsystem>();
    assert(inv != nullptr);
    assert(inv->is_initialised());
    assert(inv->world() == world);
    assert(log.size() == 1 && log[0] == "Inventory::init");
    printf("default world auto-created, InventorySubsystem booted: OK\n");

    // State/mode/players.
    MyGameState &state = world->set_state<MyGameState>();
    state.add_score(10);
    assert(state.score == 10);
    assert(state.revision() == 1); // one mark_changed() call
    assert(state.world() == world);
    printf("state: score=%d revision=%llu (expect 10, 1)\n", state.score, (unsigned long long)state.revision());

    world->set_mode<MyGameMode>();
    assert(log.size() == 2 && log[1] == "MyGameMode::on_start");

    PlayerId p1(1001);
    MyPlayerState &ps1 = world->add_player<MyPlayerState>(p1);
    ps1.display_name = "Alice";
    assert(log.size() == 3 && log[2] == "MyGameMode::joined");
    assert(world->players().size() == 1);
    assert(world->get_player(p1) == &ps1);
    assert(ps1.id() == p1);
    assert(ps1.world() == world);
    printf("player joined, GameMode notified: OK\n");

    // State/players survive with NO GameMode at all — the load-bearing
    // design point from World.h's doc comment (state lives on World, not
    // GameMode, specifically so it's still there without one).
    world->set_mode(nullptr);
    assert(log.size() == 4 && log[3] == "MyGameMode::on_stop");
    assert(world->state() == &state); // still here
    assert(world->get_player(p1) == &ps1); // still here
    printf("state/players survive GameMode removal: OK\n");

    bool removed = world->remove_player(p1);
    assert(removed);
    assert(world->players().empty());
    assert(world->get_player(p1) == nullptr);
    printf("player removed: OK\n");

    // WorldManagerSubsystem::destroy() tears the world down: subsystems in
    // reverse order (just Inventory here), after state/players are cleared.
    size_t before_destroy = log.size();
    wm->destroy(world);
    assert(log.size() == before_destroy + 1);
    assert(log.back() == "Inventory::deinit");
    assert(wm->worlds().empty());
    assert(wm->main() == nullptr);
    printf("world destroyed, subsystem torn down: OK\n");

    mgr.shutdown();

    printf("ALL CHECKS PASSED\n");
    return 0;
}
