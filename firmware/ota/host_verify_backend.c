#include "host_verify_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ota_host_backend_authenticate(void *ctx, const char *token, const char *target) {
  ota_host_backend_t *backend = (ota_host_backend_t *)ctx;
  if (backend == NULL || !backend->configured) {
    return -1;
  }
  if (token == NULL || target == NULL) {
    return -1;
  }
  if (strncmp(target, backend->expected_target, OTA_HOST_TARGET_MAX) != 0) {
    return -1;
  }
  if (backend->expected_token[0] == '\0' ||
      strncmp(token, backend->expected_token, OTA_HOST_TOKEN_MAX) != 0) {
    return -1;
  }
  return 0;
}

int ota_host_backend_verify_target(void *ctx, const char *target, const char *version,
                                 uint32_t length, const uint8_t *digest, size_t digest_len) {
  ota_host_backend_t *backend = (ota_host_backend_t *)ctx;
  (void)digest;
  (void)digest_len;
  if (backend == NULL || !backend->configured) {
    return -1;
  }
  if (target == NULL || version == NULL) {
    return -1;
  }
  if (strncmp(target, backend->expected_target, OTA_HOST_TARGET_MAX) != 0) {
    return -1;
  }
  if (strncmp(version, backend->expected_version, OTA_VERSION_LEN) != 0) {
    return -1;
  }
  if (length != backend->expected_length && backend->expected_length != 0u) {
    return -1;
  }
  return 0;
}

int ota_host_backend_verify_signature(void *ctx, const ota_image_meta_t *meta,
                                      const uint8_t *calculated_digest, size_t digest_len) {
  ota_host_backend_t *backend = (ota_host_backend_t *)ctx;
  if (backend == NULL || !backend->configured || meta == NULL ||
      calculated_digest == NULL || digest_len != OTA_DIGEST_LEN ||
      meta->signature_length == 0u) {
    return -1;
  }
  return memcmp(meta->digest, calculated_digest, OTA_DIGEST_LEN) == 0 ? 0 : -1;
}

int ota_host_backend_staging_write(void *ctx, const uint8_t *data, size_t length, size_t offset) {
  ota_host_backend_t *backend = (ota_host_backend_t *)ctx;
  (void)backend;
  (void)data;
  (void)length;
  (void)offset;
  return 0;
}

int ota_host_backend_stage_commit(void *ctx, const ota_image_meta_t *meta) {
  ota_host_backend_t *backend = (ota_host_backend_t *)ctx;
  (void)backend;
  if (meta == NULL) {
    return -1;
  }
  return 0;
}

int ota_host_backend_mark_pending(void *ctx, const ota_image_meta_t *meta) {
  ota_host_backend_t *backend = (ota_host_backend_t *)ctx;
  (void)backend;
  if (meta == NULL) {
    return -1;
  }
  return 0;
}

int ota_host_backend_confirm_health(void *ctx, const ota_image_meta_t *meta) {
  ota_host_backend_t *backend = (ota_host_backend_t *)ctx;
  (void)backend;
  if (meta == NULL) {
    return -1;
  }
  return 0;
}
