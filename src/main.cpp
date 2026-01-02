#include "world.hpp"

#include <print>

struct A {
    int value;
};

struct B {
    int value;
};

struct C {
    int value;
};


int main() {
    auto world = World();

    world.registerComponent<A>();
    world.registerComponent<B>();
    world.registerComponent<C>();

    world.spawn(A{10}, B{10});

    auto entity = world.spawn(A{10}, B{10});

    auto location1 = world.entities.getLocation(entity).value();
    std::println("Entity location: arch: {}, row: {}", location1.archetype, location1.row);

    world.insert(entity, C{10});

    auto location2 = world.entities.getLocation(entity).value();
    std::println("Entity location: arch: {}, row: {}", location2.archetype, location2.row);

    world.despawn(entity);

    auto entity2 = world.spawn(A{10}, B{10});

    auto location3 = world.entities.getLocation(entity2).value();
    std::println("Entity location: arch: {}, row: {}", location3.archetype, location3.row);

    world.insert(entity2, C{10});

    auto location4 = world.entities.getLocation(entity2).value();
    std::println("Entity location: arch: {}, row: {}", location4.archetype, location4.row);

    world.iter<Entity, A, B, C>([&](Entity entity, A& a, B& b, C& c) {
        std::println("Entity: id: {}, gen: {}", entity.id, entity.generation);
        std::println("A: {}", a.value);
        std::println("B: {}", b.value);
        std::println("C: {}", c.value);
    });


    return 0;
}
