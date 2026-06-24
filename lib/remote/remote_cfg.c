#include "remote_cfg.h"

#include "remote_secret.h"

#include <stdio.h>

size_t remote_get_host(char *out, size_t len)
{
    return rsec_host(out, len);
}

uint16_t remote_get_mqtt_port(void)
{
    return rsec_mqtt_port();
}

size_t remote_build_mqtt_uri(char *out, size_t len)
{
    char host[32];
    if (rsec_host(host, sizeof(host)) == 0) {
        if (len > 0) {
            out[0] = '\0';
        }
        return 0;
    }
    return (size_t)snprintf(out, len, "mqtts://%s:%u", host, (unsigned)rsec_mqtt_port());
}

size_t remote_get_mqtt_user(char *out, size_t len)
{
    return rsec_mqtt_user(out, len);
}

size_t remote_get_mqtt_pass(char *out, size_t len)
{
    return rsec_mqtt_pass(out, len);
}

size_t remote_build_ota_url(char *out, size_t len)
{
    return rsec_ota_base(out, len);
}
