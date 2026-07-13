#include <cassert>
#include <cstdio>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "gameframework/SubsystemManager.h"

using namespace gameframework;

namespace
{

std::vector<std::string> *g_init_order = nullptr;

class SubsystemA : public Subsystem
{
protected:
    void initialize() override { g_init_order->push_back("A"); }
    void deinitialize() override { g_init_order->push_back("~A"); }
};

// Depends on A — must initialize after it, deinitialize before it.
class SubsystemB : public Subsystem
{
public:
    std::vector<std::type_index> depends_on() const override
    {
        return {std::type_index(typeid(SubsystemA))};
    }

protected:
    void initialize() override { g_init_order->push_back("B"); }
    void deinitialize() override { g_init_order->push_back("~B"); }
};

class DisabledSubsystem : public Subsystem
{
public:
    StartMode start_mode() const override { return StartMode::Disabled; }

protected:
    void initialize() override { g_init_order->push_back("DISABLED_SHOULD_NOT_APPEAR"); }
};

// Survives should_create() (not Disabled) but must NOT auto-initialize in
// boot() — only via an explicit initialize_global_subsystem_at() call.
class OnDemandSubsystem : public Subsystem
{
public:
    StartMode start_mode() const override { return StartMode::OnDemand; }

protected:
    void initialize() override { g_init_order->push_back("ONDEMAND"); }
};

// Ticks; SubsystemA/B/DisabledSubsystem deliberately don't override
// wants_tick(), so tick_all() must skip them (proven by tick_all() not
// crashing on their default no-op tick() and TickingSubsystem's own count
// being the only thing that moves).
class TickingSubsystem : public Subsystem
{
public:
    bool wants_tick() const override { return true; }
    void tick(double delta) override
    {
        ++tick_count;
        last_delta = delta;
    }

    int tick_count = 0;
    double last_delta = 0.0;
};

} // namespace

int main()
{
    std::vector<std::string> order;
    g_init_order = &order;

    SubsystemManager &mgr = SubsystemManager::instance();

    // Register B before A, to prove dependency ordering (not registration
    // order) determines the actual init sequence.
    mgr.register_subsystem(std::make_unique<SubsystemB>());
    mgr.register_subsystem(std::make_unique<SubsystemA>());
    mgr.register_subsystem(std::make_unique<DisabledSubsystem>());
    mgr.register_subsystem(std::make_unique<OnDemandSubsystem>());
    TickingSubsystem &ticker = mgr.register_subsystem<TickingSubsystem>();

    assert(mgr.global_subsystem_count() == 5);

    // tick_all() before boot() must be a no-op — nothing is initialised yet.
    mgr.tick_all(0.016);
    assert(ticker.tick_count == 0);

    mgr.boot();

    // DisabledSubsystem should have been dropped before boot (should_create()
    // == false), so only A and B actually initialize, A before B.
    assert(order.size() == 2);
    assert(order[0] == "A");
    assert(order[1] == "B");
    printf("boot order: %s, %s (expect A, B)\n", order[0].c_str(), order[1].c_str());

    SubsystemA *a = mgr.get_global_subsystem<SubsystemA>();
    SubsystemB *b = mgr.get_global_subsystem<SubsystemB>();
    assert(a != nullptr);
    assert(b != nullptr);
    assert(a->is_initialised());
    assert(b->is_initialised());

    // OnDemandSubsystem survives should_create() (registered, not Disabled)
    // but boot() must NOT have auto-initialized it.
    OnDemandSubsystem *on_demand = mgr.get_global_subsystem<OnDemandSubsystem>();
    assert(on_demand != nullptr);
    assert(!on_demand->is_initialised());
    assert(order.size() == 2); // still just A, B — ONDEMAND not in there yet

    // DisabledSubsystem was dropped entirely; OnDemandSubsystem/TickingSubsystem
    // survived — count is 4 post-boot (A, B, OnDemand, Ticking), not the 5 registered.
    assert(mgr.global_subsystem_count() == 4);

    // Manually initialize it now (the "dev calls InitialiseClient() themselves"
    // / "user flips it on from the settings tab" path) — find its index first,
    // since initialize_global_subsystem_at() is index-based like the rest of
    // this dock-facing surface.
    int on_demand_index = -1;
    for (int i = 0; i < mgr.global_subsystem_count(); ++i)
    {
        if (mgr.get_global_subsystem_at(i) == on_demand)
            on_demand_index = i;
    }
    assert(on_demand_index >= 0);
    assert(mgr.initialize_global_subsystem_at(on_demand_index));
    assert(on_demand->is_initialised());
    assert(order.size() == 3);
    assert(order[2] == "ONDEMAND");
    // A second manual call must be a no-op (already initialised).
    assert(!mgr.initialize_global_subsystem_at(on_demand_index));
    printf("on-demand: manual init worked, idempotent on retry\n");

    // Now that everything's booted, tick_all() must reach TickingSubsystem
    // (and only it — SubsystemA/B/DisabledSubsystem's default tick() is a
    // no-op, so nothing else to assert on them beyond "didn't crash").
    mgr.tick_all(0.016);
    mgr.tick_all(0.032);
    assert(ticker.tick_count == 2);
    assert(ticker.last_delta == 0.032);
    printf("tick_all: count=%d last_delta=%f (expect 2, 0.032)\n", ticker.tick_count, ticker.last_delta);

    // A second boot() call must be a no-op (idempotent).
    mgr.boot();
    assert(order.size() == 3);

    mgr.shutdown();

    // Deinitialize must run in REVERSE order: B before A. OnDemandSubsystem
    // doesn't override deinitialize() (base no-op), so it contributes
    // nothing to order here even though do_deinitialize() does run on it.
    assert(order.size() == 5);
    assert(order[3] == "~B");
    assert(order[4] == "~A");
    printf("shutdown order: %s, %s (expect ~B, ~A)\n", order[3].c_str(), order[4].c_str());

    // tick_all() after shutdown() must be a no-op again — nothing is
    // initialised (release_all() isn't required for this; shutdown() alone
    // already flips is_initialised() back to false on every entry).
    mgr.tick_all(0.016);
    assert(ticker.tick_count == 2);
    printf("tick_all after shutdown: count=%d (expect still 2)\n", ticker.tick_count);

    printf("ALL CHECKS PASSED\n");
    return 0;
}
