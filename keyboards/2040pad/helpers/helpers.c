#pragma once
#include QMK_KEYBOARD_H

#define HOST_TELEMETRY_TIMEOUT_MS 2000

enum host_telemetry_value_ids {
    TELEMETRY_VALUE_HOST_STATUS = 0x01,
};

enum host_telemetry_valid_flags {
    TELEMETRY_VALID_VOLUME   = (1 << 0),
    TELEMETRY_VALID_CPU      = (1 << 1),
    TELEMETRY_VALID_RAM      = (1 << 2),
    TELEMETRY_VALID_NET_DOWN = (1 << 3),
    TELEMETRY_VALID_NET_UP   = (1 << 4),
};

typedef struct {
    uint8_t  protocol_version;
    uint8_t  sequence;
    uint8_t  valid_mask;
    uint8_t  volume_percent;
    uint8_t  cpu_percent;
    uint8_t  ram_percent;
    uint16_t net_down_kbps;
    uint16_t net_up_kbps;
    uint32_t host_time_s;
    uint32_t last_update_ms;
} host_telemetry_t;

host_telemetry_t host_telemetry = {0};
bool             host_telemetry_received = false;

static uint8_t clamp_percent_u8(uint8_t value) {
    return value > 100 ? 100 : value;
}

bool host_telemetry_update_from_payload(const uint8_t *payload, uint8_t payload_len) {
    if (payload_len < 14) {
        return false;
    }

    host_telemetry.protocol_version = payload[0];
    host_telemetry.sequence         = payload[1];
    host_telemetry.valid_mask       = payload[2];
    host_telemetry.volume_percent   = clamp_percent_u8(payload[3]);
    host_telemetry.cpu_percent      = clamp_percent_u8(payload[4]);
    host_telemetry.ram_percent      = clamp_percent_u8(payload[5]);
    host_telemetry.net_down_kbps    = ((uint16_t)payload[7] << 8) | payload[6];
    host_telemetry.net_up_kbps      = ((uint16_t)payload[9] << 8) | payload[8];
    host_telemetry.host_time_s      = ((uint32_t)payload[10]) | ((uint32_t)payload[11] << 8) | ((uint32_t)payload[12] << 16) | ((uint32_t)payload[13] << 24);
    host_telemetry.last_update_ms   = timer_read32();
    host_telemetry_received         = true;

    return true;
}

bool host_telemetry_write_payload(uint8_t *payload, uint8_t payload_len) {
    if (payload_len < 14) {
        return false;
    }

    for (uint8_t i = 0; i < payload_len; i++) {
        payload[i] = 0;
    }

    payload[0]  = host_telemetry.protocol_version;
    payload[1]  = host_telemetry.sequence;
    payload[2]  = host_telemetry.valid_mask;
    payload[3]  = host_telemetry.volume_percent;
    payload[4]  = host_telemetry.cpu_percent;
    payload[5]  = host_telemetry.ram_percent;
    payload[6]  = (uint8_t)(host_telemetry.net_down_kbps & 0xFF);
    payload[7]  = (uint8_t)((host_telemetry.net_down_kbps >> 8) & 0xFF);
    payload[8]  = (uint8_t)(host_telemetry.net_up_kbps & 0xFF);
    payload[9]  = (uint8_t)((host_telemetry.net_up_kbps >> 8) & 0xFF);
    payload[10] = (uint8_t)(host_telemetry.host_time_s & 0xFF);
    payload[11] = (uint8_t)((host_telemetry.host_time_s >> 8) & 0xFF);
    payload[12] = (uint8_t)((host_telemetry.host_time_s >> 16) & 0xFF);
    payload[13] = (uint8_t)((host_telemetry.host_time_s >> 24) & 0xFF);

    return true;
}

bool host_telemetry_is_fresh(void) {
    return host_telemetry_received && timer_elapsed32(host_telemetry.last_update_ms) <= HOST_TELEMETRY_TIMEOUT_MS;
}

enum custom_keycodes {
    LENC = SAFE_RANGE,
    RENC,
    MODE_SELECT,
};

enum encoder_modes {
    DEFAULT,
    BRIGHTNESS,
    MOUSE,
    TEXT,
    APPSW,
    MEDIA,
    // PONG,
    ENCODER_MODES_COUNT
};

const char* encoder_mode_names[] = {
    "DEFAULT",
    "BRIGHT",
    "MOUSE",
    "TEXT",
    "APPSW",
    "MEDIA",
    // "PONG"
};

uint8_t enc_mode = DEFAULT;

enum display_modes {
    NORMAL,
    CALCULATOR,
    ENCODER_SELECT
};

const char* display_mode_names[] = {
    "NUMPAD",
    "CALCULATOR"
};

uint8_t display_mode = NORMAL;
uint8_t display_mode_selector = NORMAL;
