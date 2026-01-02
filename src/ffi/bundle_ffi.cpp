#include "../world.hpp"

extern "C" {
    Bundle* _BundleCreate(World* world, component_id bitmask, std::byte* buffer, std::size_t count) {
        return std::make_unique<Bundle>(world->components, bitmask, buffer, count, false).release();
    }

    void _BundleDestroy(Bundle* bundle) {
        std::unique_ptr<Bundle> _(bundle);
    }
}
