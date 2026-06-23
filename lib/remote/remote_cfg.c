#include "remote_cfg.h"

#include <stdio.h>
#include <string.h>

static const uint8_t s_k[] = { 0xA3, 0x5F, 0x91, 0x2E, 0xC7, 0x4B };

static const uint8_t s_h[] = {
    0xC0, 0x3E, 0xE5, 0x4F, 0xA9, 0x24, 0xCA, 0x2A, 0xF0, 0x40,
    0xA3, 0x39, 0xC6, 0x36, 0xBF, 0x5C, 0xA8,
};

static const uint8_t s_p[] = {
    0x8C, 0x30, 0xE5, 0x4F, 0xEA, 0x2A, 0x94, 0x39, 0xA2, 0x45,
    0xFE, 0x79, 0xDB,
};

static const uint8_t s_v[] = {
    0x8C, 0x29, 0xF4, 0x5C, 0xB4, 0x22, 0xCC, 0x31, 0xBF, 0x5A,
    0xBF, 0x3F,
};

#define REMOTE_PORT_ENC  0x9C5Cu
#define REMOTE_PORT_MASK 0xBEEFu

static size_t decode_blob(const uint8_t *enc, size_t enc_len, char *out, size_t len)
{
    if (out == NULL || len == 0) {
        return 0;
    }
    size_t n = enc_len;
    if (n >= len) {
        n = len - 1;
    }
    for (size_t i = 0; i < n; i++) {
        out[i] = (char)(enc[i] ^ s_k[i % (sizeof(s_k))]);
    }
    out[n] = '\0';
    return n;
}

size_t remote_get_host(char *out, size_t len)
{
    return decode_blob(s_h, sizeof(s_h), out, len);
}

uint16_t remote_get_mqtt_port(void)
{
    return (uint16_t)(REMOTE_PORT_ENC ^ REMOTE_PORT_MASK);
}

size_t remote_build_mqtt_uri(char *out, size_t len)
{
    char host[32];
    if (remote_get_host(host, sizeof(host)) == 0) {
        if (len > 0) {
            out[0] = '\0';
        }
        return 0;
    }
    return (size_t)snprintf(out, len, "mqtts://%s:%u", host, (unsigned)remote_get_mqtt_port());
}

size_t remote_build_ota_url(char *out, size_t len)
{
    char host[32];
    char path[32];
    if (remote_get_host(host, sizeof(host)) == 0) {
        if (len > 0) {
            out[0] = '\0';
        }
        return 0;
    }
    decode_blob(s_p, sizeof(s_p), path, sizeof(path));
    return (size_t)snprintf(out, len, "https://%s%s", host, path);
}

size_t remote_build_version_url(char *out, size_t len)
{
    char host[32];
    char path[32];
    if (remote_get_host(host, sizeof(host)) == 0) {
        if (len > 0) {
            out[0] = '\0';
        }
        return 0;
    }
    decode_blob(s_v, sizeof(s_v), path, sizeof(path));
    return (size_t)snprintf(out, len, "https://%s%s", host, path);
}
