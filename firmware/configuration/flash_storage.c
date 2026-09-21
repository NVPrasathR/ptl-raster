#include "flash_storage.h"

#include <string.h>

static raster_storage_backend_t g_backend;

bool raster_storage_set_backend(const raster_storage_backend_t *backend) {
    if (backend == NULL || backend->read == NULL || backend->write == NULL ||
        backend->erase == NULL || backend->slot_size == 0u) {
        memset(&g_backend, 0, sizeof(g_backend));
        return false;
    }
    g_backend = *backend;
    return true;
}

bool raster_storage_is_configured(void) {
    return g_backend.read != NULL && g_backend.write != NULL &&
           g_backend.erase != NULL && g_backend.slot_size != 0u;
}

bool raster_storage_read(uint32_t slot, void *destination, size_t length) {
    return raster_storage_is_configured() && destination != NULL &&
           slot < RASTER_STORAGE_SLOT_COUNT && length <= g_backend.slot_size &&
           g_backend.read(slot, destination, length);
}

bool raster_storage_write(uint32_t slot, const void *source, size_t length) {
    return raster_storage_is_configured() && source != NULL &&
           slot < RASTER_STORAGE_SLOT_COUNT && length <= g_backend.slot_size &&
           g_backend.write(slot, source, length);
}

bool raster_storage_erase(uint32_t slot) {
    return raster_storage_is_configured() && slot < RASTER_STORAGE_SLOT_COUNT &&
           g_backend.erase(slot);
}
