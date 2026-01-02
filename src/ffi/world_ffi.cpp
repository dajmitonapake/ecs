#include "../world.hpp"

extern "C" {
    World* _WorldCreate () {
        return std::make_unique<World>().release();
    }

    void _WorldDestroy(World* world) {
        std::unique_ptr<World> _(world);
    }

    component_id _WorldRegisterComponent(World* worldPtr, TypeInfo typeInfo) {
        return worldPtr->registerComponent(typeInfo);
    }

    Entity _WorldSpawnEmpty(World* world) {
        return world->spawnEmpty();
    }

    Entity _WorldSpawn(World* world, Bundle* bundlePtr) {
        std::unique_ptr<Bundle> bundle(bundlePtr);
        return world->spawnBundle(std::move(bundle));
    }

    void _WorldInsert(World* world, Entity entity, Bundle* bundlePtr) {
        std::unique_ptr<Bundle> bundle(bundlePtr);
        world->insertBundle(entity, std::move(bundle));
    }

    void _WorldRemove(World* world, Entity entity, Bundle* bundlePtr) {
        std::unique_ptr<Bundle> bundle(bundlePtr);
        world->removeBundle(entity, std::move(bundle));
    }

    void _WorldDespawn(World* world, Entity entity) {
        world->despawn(entity);
    }

    std::byte* _WorldGet(World* world, Entity entity, component_id bit) {
        return world->get(entity, bit);
    }
}
