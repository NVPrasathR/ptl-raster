#ifndef OTA_VERIFY_H
#define OTA_VERIFY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int ota_digest_compute(const uint8_t *data, size_t length, uint8_t out_digest[32]);
int ota_verify_digest(const uint8_t *data, size_t length, const uint8_t expected_digest[32]);
void ota_digest_to_hex(const uint8_t digest[32], char out_hex[65]);

#ifdef __cplusplus
}
#endif

#endif
