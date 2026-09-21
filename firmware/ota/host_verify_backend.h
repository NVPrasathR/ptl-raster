#ifndef OTA_HOST_VERIFY_BACKEND_H
#define OTA_HOST_VERIFY_BACKEND_H

#include <stddef.h>
#include <stdint.h>

#include "ota_state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_HOST_TARGET_MAX 64
#define OTA_HOST_TOKEN_MAX 64

typedef struct {
  char expected_target[OTA_HOST_TARGET_MAX];
  char expected_token[OTA_HOST_TOKEN_MAX];
  char expected_version[OTA_VERSION_LEN];
  uint32_t expected_length;
  int configured;
} ota_host_backend_t;

int ota_host_backend_authenticate(void *ctx, const char *token, const char *target);
int ota_host_backend_verify_target(void *ctx, const char *target, const char *version,
                                 uint32_t length, const uint8_t *digest, size_t digest_len);
int ota_host_backend_verify_signature(void *ctx, const ota_image_meta_t *meta,
                                      const uint8_t *calculated_digest, size_t digest_len);
int ota_host_backend_staging_write(void *ctx, const uint8_t *data, size_t length, size_t offset);
int ota_host_backend_stage_commit(void *ctx, const ota_image_meta_t *meta);
int ota_host_backend_mark_pending(void *ctx, const ota_image_meta_t *meta);
int ota_host_backend_confirm_health(void *ctx, const ota_image_meta_t *meta);

#ifdef __cplusplus
}
#endif

#endif
