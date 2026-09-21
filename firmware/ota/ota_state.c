#include "ota_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ota_verify.h"

static void ota_set_error(ota_session_t *session, uint32_t code, const char *reason) {
  if (session == NULL) {
    return;
  }
  session->last_error = code;
  if (reason != NULL) {
    snprintf(session->last_error_string, sizeof(session->last_error_string), "%s", reason);
  } else {
    session->last_error_string[0] = '\0';
  }
}

void ota_session_init(ota_session_t *session) {
  if (session == NULL) {
    return;
  }
  memset(session, 0, sizeof(*session));
  session->state = OTA_STATE_IDLE;
  session->staging_capacity = 0u;
  session->staging_used = 0u;
  session->received_bytes = 0u;
}

void ota_session_reset(ota_session_t *session) {
  if (session == NULL) {
    return;
  }
  free(session->staging_buffer);
  session->staging_buffer = NULL;
  session->staging_capacity = 0u;
  session->staging_used = 0u;
  session->received_bytes = 0u;
  session->authenticated = 0;
  session->health_ok = 0;
  session->target_accepted = 0;
  session->last_error = OTA_ERR_NONE;
  session->last_error_string[0] = '\0';
  session->state = OTA_STATE_IDLE;
  memset(&session->meta, 0, sizeof(session->meta));
  session->token[0] = '\0';
}

int ota_authenticate(ota_session_t *session, const ota_backend_ops_t *backend,
                    const char *token, const char *target) {
  if (session == NULL || backend == NULL || backend->authenticate == NULL) {
    ota_set_error(session, OTA_ERR_CONFIG, "board backend not configured");
    return -1;
  }
  if (token == NULL || target == NULL) {
    ota_set_error(session, OTA_ERR_AUTH, "missing token or target");
    return -1;
  }
  if (backend->authenticate(backend->ctx, token, target) != 0) {
    ota_set_error(session, OTA_ERR_AUTH, "callback rejected authentication");
    return -1;
  }
  session->authenticated = 1;
  snprintf(session->token, sizeof(session->token), "%s", token);
  session->state = OTA_STATE_READY;
  return 0;
}

int ota_begin_image(ota_session_t *session, const char *target, const char *version,
                   uint32_t image_length) {
  if (session == NULL || target == NULL || version == NULL) {
    ota_set_error(session, OTA_ERR_CONFIG, "must provide target and version");
    return -1;
  }
  if (!session->authenticated) {
    ota_set_error(session, OTA_ERR_AUTH, "upload requires authenticated session");
    return -1;
  }
  if (image_length == 0u || image_length > OTA_MAX_BUFFERED_IMAGE_SIZE) {
    ota_set_error(session, OTA_ERR_LENGTH, "image length out of range");
    return -1;
  }
  if (session->staging_buffer != NULL) {
    free(session->staging_buffer);
    session->staging_buffer = NULL;
  }
  session->staging_capacity = (size_t)image_length;
  session->staging_buffer = (uint8_t *)calloc(1u, session->staging_capacity);
  if (session->staging_buffer == NULL) {
    ota_set_error(session, OTA_ERR_CONFIG, "staging allocation failed");
    return -1;
  }
  memset(&session->meta, 0, sizeof(session->meta));
  snprintf(session->meta.target, sizeof(session->meta.target), "%s", target);
  snprintf(session->meta.version, sizeof(session->meta.version), "%s", version);
  session->meta.image_length = image_length;
  session->received_bytes = 0u;
  session->staging_used = 0u;
  session->health_ok = 0;
  session->target_accepted = 0;
  session->state = OTA_STATE_RECEIVING;
  return 0;
}

int ota_set_image_authenticity(ota_session_t *session,
                               const uint8_t expected_digest[OTA_DIGEST_LEN],
                               const uint8_t *signature, size_t signature_length) {
  if (session == NULL || expected_digest == NULL || signature == NULL ||
      signature_length == 0u || signature_length > OTA_SIG_LEN ||
      session->state != OTA_STATE_RECEIVING || session->received_bytes != 0u) {
    ota_set_error(session, OTA_ERR_SIGNATURE, "invalid or late authenticity metadata");
    return -1;
  }
  memcpy(session->meta.digest, expected_digest, OTA_DIGEST_LEN);
  memcpy(session->meta.signature, signature, signature_length);
  session->meta.signature_length = signature_length;
  ota_digest_to_hex(session->meta.digest, session->meta.digest_hex);
  return 0;
}

int ota_stream_chunk(ota_session_t *session, const uint8_t *data, size_t length) {
  if (session == NULL || data == NULL || length == 0u) {
    return 0;
  }
  if (session->state != OTA_STATE_RECEIVING && session->state != OTA_STATE_STAGING) {
    ota_set_error(session, OTA_ERR_STATE, "image not in an active receive state");
    return -1;
  }
  if (length > session->staging_capacity ||
      (size_t)session->received_bytes > session->staging_capacity - length ||
      (size_t)session->received_bytes + length > session->meta.image_length) {
    ota_set_error(session, OTA_ERR_LENGTH, "chunk exceeds declared image length");
    return -1;
  }
  if (session->staging_buffer == NULL) {
    ota_set_error(session, OTA_ERR_CONFIG, "staging buffer unavailable");
    return -1;
  }
  memcpy(session->staging_buffer + session->received_bytes, data, length);
  session->received_bytes += (uint32_t)length;
  session->staging_used = session->received_bytes;
  return 0;
}

int ota_finalize_upload(ota_session_t *session, const ota_backend_ops_t *backend) {
  uint8_t calculated_digest[OTA_DIGEST_LEN];
  uint8_t digest_difference = 0u;
  if (session == NULL || backend == NULL) {
    return -1;
  }
  if (session->state == OTA_STATE_ABORTED) {
    ota_set_error(session, OTA_ERR_ABORTED, "upload was interrupted and must be retried");
    return -1;
  }
  if (session->received_bytes != session->meta.image_length) {
    ota_set_error(session, OTA_ERR_LENGTH, "image length mismatch before verification");
    return -1;
  }
  if (backend->verify_target == NULL || backend->verify_signature == NULL ||
      backend->staging_write == NULL || backend->stage_commit == NULL) {
    ota_set_error(session, OTA_ERR_BACKEND, "target backend unavailable; fail closed");
    return -1;
  }
  if (session->staging_buffer == NULL) {
    ota_set_error(session, OTA_ERR_CONFIG, "missing staged bytes");
    return -1;
  }
  if (session->meta.signature_length == 0u) {
    ota_set_error(session, OTA_ERR_SIGNATURE, "detached signature is required");
    return -1;
  }
  if (ota_digest_compute(session->staging_buffer, session->meta.image_length,
                         calculated_digest) != 0) {
    ota_set_error(session, OTA_ERR_DIGEST, "digest generation failed");
    return -1;
  }
  for (size_t index = 0u; index < OTA_DIGEST_LEN; ++index) {
    digest_difference |= calculated_digest[index] ^ session->meta.digest[index];
  }
  if (digest_difference != 0u) {
    ota_set_error(session, OTA_ERR_DIGEST, "image digest does not match signed metadata");
    return -1;
  }
  if (backend->verify_target(backend->ctx, session->meta.target, session->meta.version,
                            session->meta.image_length, calculated_digest,
                            sizeof(calculated_digest)) != 0) {
    ota_set_error(session, OTA_ERR_TARGET, "backend rejected target metadata");
    return -1;
  }
  if (backend->verify_signature(backend->ctx, &session->meta, calculated_digest,
                                sizeof(calculated_digest)) != 0) {
    ota_set_error(session, OTA_ERR_SIGNATURE, "firmware signature rejected");
    return -1;
  }
  if (backend->staging_write(backend->ctx, session->staging_buffer, session->meta.image_length, 0u) != 0) {
    ota_set_error(session, OTA_ERR_BACKEND, "staging write failed");
    return -1;
  }
  if (backend->stage_commit(backend->ctx, &session->meta) != 0) {
    ota_set_error(session, OTA_ERR_BACKEND, "stage commit rejected");
    return -1;
  }
  session->target_accepted = 1;
  session->state = OTA_STATE_PENDING;
  return 0;
}

int ota_accept_pending(ota_session_t *session, const ota_backend_ops_t *backend) {
  if (session == NULL || backend == NULL || backend->mark_pending == NULL) {
    ota_set_error(session, OTA_ERR_BACKEND, "pending transition backend missing");
    return -1;
  }
  if (session->state != OTA_STATE_PENDING) {
    ota_set_error(session, OTA_ERR_STATE, "firmware must be pending before health confirm");
    return -1;
  }
  if (backend->mark_pending(backend->ctx, &session->meta) != 0) {
    ota_set_error(session, OTA_ERR_BACKEND, "backend rejected pending state");
    return -1;
  }
  session->state = OTA_STATE_HEALTH_CONFIRM;
  return 0;
}

int ota_confirm_health(ota_session_t *session, const ota_backend_ops_t *backend) {
  if (session == NULL || backend == NULL || backend->confirm_health == NULL) {
    ota_set_error(session, OTA_ERR_BACKEND, "health-confirm backend missing");
    return -1;
  }
  if (session->state != OTA_STATE_HEALTH_CONFIRM) {
    ota_set_error(session, OTA_ERR_STATE, "firmware not waiting for health confirmation");
    return -1;
  }
  if (backend->confirm_health(backend->ctx, &session->meta) != 0) {
    ota_set_error(session, OTA_ERR_BACKEND, "device health did not pass");
    session->state = OTA_STATE_FAILED;
    return -1;
  }
  session->health_ok = 1;
  session->state = OTA_STATE_APPLIED;
  return 0;
}

int ota_abort(ota_session_t *session, const char *reason) {
  if (session == NULL) {
    return -1;
  }
  if (reason != NULL) {
    snprintf(session->last_error_string, sizeof(session->last_error_string), "%s", reason);
  } else {
    session->last_error_string[0] = '\0';
  }
  session->last_error = OTA_ERR_ABORTED;
  session->state = OTA_STATE_ABORTED;
  return 0;
}
