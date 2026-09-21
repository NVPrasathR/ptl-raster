#ifndef OTA_STATE_H
#define OTA_STATE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_TARGET_LEN 32
#define OTA_VERSION_LEN 32
#define OTA_DIGEST_LEN 32
#define OTA_SIG_LEN 64
#define OTA_REASON_LEN 128
#define OTA_MAX_BUFFERED_IMAGE_SIZE (192u * 1024u)

#define OTA_ERR_NONE 0u
#define OTA_ERR_AUTH 1u
#define OTA_ERR_CONFIG 2u
#define OTA_ERR_LENGTH 3u
#define OTA_ERR_TARGET 4u
#define OTA_ERR_DIGEST 5u
#define OTA_ERR_SIGNATURE 6u
#define OTA_ERR_BACKEND 7u
#define OTA_ERR_STATE 8u
#define OTA_ERR_ABORTED 9u

typedef enum {
  OTA_STATE_IDLE = 0,
  OTA_STATE_AUTH_REQUIRED,
  OTA_STATE_READY,
  OTA_STATE_RECEIVING,
  OTA_STATE_STAGING,
  OTA_STATE_PENDING,
  OTA_STATE_HEALTH_CONFIRM,
  OTA_STATE_APPLIED,
  OTA_STATE_FAILED,
  OTA_STATE_ABORTED
} ota_state_t;

typedef struct {
  char target[OTA_TARGET_LEN];
  char version[OTA_VERSION_LEN];
  uint32_t image_length;
  uint8_t digest[OTA_DIGEST_LEN];
  char digest_hex[65];
  uint8_t signature[OTA_SIG_LEN];
  size_t signature_length;
  uint32_t flags;
} ota_image_meta_t;

typedef struct ota_backend_ops {
  int (*authenticate)(void *ctx, const char *token, const char *target);
  int (*verify_target)(void *ctx, const char *target, const char *version, uint32_t length,
                      const uint8_t *digest, size_t digest_len);
  int (*verify_signature)(void *ctx, const ota_image_meta_t *meta,
                          const uint8_t *calculated_digest, size_t digest_len);
  int (*staging_write)(void *ctx, const uint8_t *data, size_t length, size_t offset);
  int (*stage_commit)(void *ctx, const ota_image_meta_t *meta);
  int (*mark_pending)(void *ctx, const ota_image_meta_t *meta);
  int (*confirm_health)(void *ctx, const ota_image_meta_t *meta);
  void *ctx;
} ota_backend_ops_t;

typedef struct {
  ota_state_t state;
  ota_image_meta_t meta;
  uint8_t *staging_buffer;
  size_t staging_capacity;
  size_t staging_used;
  uint32_t received_bytes;
  int authenticated;
  int health_ok;
  int target_accepted;
  uint32_t last_error;
  char last_error_string[OTA_REASON_LEN];
  char token[64];
} ota_session_t;

void ota_session_init(ota_session_t *session);
void ota_session_reset(ota_session_t *session);
int ota_authenticate(ota_session_t *session, const ota_backend_ops_t *backend,
                    const char *token, const char *target);
int ota_begin_image(ota_session_t *session, const char *target, const char *version,
                   uint32_t image_length);
int ota_set_image_authenticity(ota_session_t *session,
                               const uint8_t expected_digest[OTA_DIGEST_LEN],
                               const uint8_t *signature, size_t signature_length);
int ota_stream_chunk(ota_session_t *session, const uint8_t *data, size_t length);
int ota_finalize_upload(ota_session_t *session, const ota_backend_ops_t *backend);
int ota_accept_pending(ota_session_t *session, const ota_backend_ops_t *backend);
int ota_confirm_health(ota_session_t *session, const ota_backend_ops_t *backend);
int ota_abort(ota_session_t *session, const char *reason);

#ifdef __cplusplus
}
#endif

#endif
