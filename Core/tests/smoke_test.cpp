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

    assert(mgr.global_subsystem_count() == 3);

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

    // A second boot() call must be a no-op (idempotent).
    mgr.boot();
    assert(order.size() == 2);

    mgr.shutdown();

    // Deinitialize must run in REVERSE order: B before A.
    assert(order.size() == 4);
    assert(order[2] == "~B");
    assert(order[3] == "~A");
    printf("shutdown order: %s, %s (expect ~B, ~A)\n", order[2].c_str(), order[3].c_str());

    printf("ALL CHECKS PASSED\n");
    return 0;
}
