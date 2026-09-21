#ifndef RASTER_DASHBOARD_ASSETS_H
#define RASTER_DASHBOARD_ASSETS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const uint8_t *raster_dashboard_get_html(size_t *length_out);
const uint8_t *raster_dashboard_get_css(size_t *length_out);
const uint8_t *raster_dashboard_get_js(size_t *length_out);

#ifdef __cplusplus
}
#endif

#endif
