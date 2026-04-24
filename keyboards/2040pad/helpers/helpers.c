#pragma once
#include QMK_KEYBOARD_H

#define HOST_TELEMETRY_TIMEOUT_MS 2000
#define CAD_ROTATE_RELEASE_MS 500

/* Configurable base RAM size used to convert ram_percent to gigabytes. */
#define RAM_SIZE_GB 32

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
    /* Network speed split into two uint16_t fields to cover the full range
     * without needing uint32_t. kbps is the fractional part (0-999),
     * mbps is the integer Mbps part (0-65535, i.e. up to ~65 Gbps). */
    uint16_t net_down_kbps;  /* 0-999 */
    uint16_t net_down_mbps;
    uint16_t net_up_kbps;    /* 0-999 */
    uint16_t net_up_mbps;
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
    host_telemetry.net_down_mbps    = ((uint16_t)payload[9] << 8) | payload[8];
    host_telemetry.net_up_kbps      = ((uint16_t)payload[11] << 8) | payload[10];
    host_telemetry.net_up_mbps      = ((uint16_t)payload[13] << 8) | payload[12];
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
    payload[8]  = (uint8_t)(host_telemetry.net_down_mbps & 0xFF);
    payload[9]  = (uint8_t)((host_telemetry.net_down_mbps >> 8) & 0xFF);
    payload[10] = (uint8_t)(host_telemetry.net_up_kbps & 0xFF);
    payload[11] = (uint8_t)((host_telemetry.net_up_kbps >> 8) & 0xFF);
    payload[12] = (uint8_t)(host_telemetry.net_up_mbps & 0xFF);
    payload[13] = (uint8_t)((host_telemetry.net_up_mbps >> 8) & 0xFF);

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
    CAD_ONSHAPE,
    CAD_FUSION,
    ENCODER_SELECT
};

const char* display_mode_names[] = {
    "NUMPAD",
    "CALCULATOR",
    "ONSHAPE",
    "FUSION"
};

uint8_t display_mode = NORMAL;
uint8_t display_mode_selector = NORMAL;

bool     is_pan_enabled           = false;
bool     cad_btn_b_held           = false;
uint8_t  cad_action               = CAD_ACTION_NONE;
uint8_t  cad_held_buttons         = 0;    /* button bitmask applied inside the pointing device report each scan */
bool     cad_shift_held           = false;
uint32_t cad_pan_last_motion_ms    = 0;
uint32_t cad_rotate_last_motion_ms = 0;

void cad_release_all(void) {
    cad_held_buttons = 0;

    if (cad_shift_held) {
        unregister_mods(MOD_BIT(KC_LSFT));
        del_weak_mods(MOD_BIT(KC_LSFT));
        send_keyboard_report();
        cad_shift_held = false;
    }

    is_pan_enabled             = false;
    cad_btn_b_held             = false;
    cad_action                 = CAD_ACTION_NONE;
    cad_pan_last_motion_ms     = 0;
    cad_rotate_last_motion_ms  = 0;
}
