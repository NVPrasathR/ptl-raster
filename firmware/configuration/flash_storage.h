#ifndef RASTER_FLASH_STORAGE_H
#define RASTER_FLASH_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RASTER_STORAGE_SLOT_COUNT 2u

typedef bool (*raster_storage_read_fn)(uint32_t slot, void *destination, size_t length);
typedef bool (*raster_storage_write_fn)(uint32_t slot, const void *source, size_t length);
typedef bool (*raster_storage_erase_fn)(uint32_t slot);

typedef struct {
    raster_storage_read_fn read;
    raster_storage_write_fn write;
    raster_storage_erase_fn erase;
    size_t slot_size;
} raster_storage_backend_t;

bool raster_storage_set_backend(const raster_storage_backend_t *backend);
bool raster_storage_is_configured(void);
bool raster_storage_read(uint32_t slot, void *destination, size_t length);
bool raster_storage_write(uint32_t slot, const void *source, size_t length);
bool raster_storage_erase(uint32_t slot);

#endif
