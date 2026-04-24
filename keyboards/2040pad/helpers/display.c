#include QMK_KEYBOARD_H

#include <stdbool.h>

#include "logo.c"
#include "helpers.c"
#include "calculator.c"

uint32_t timer        = 0;
bool     logo_cleared = false;

void render_lock_status(void) {
    led_t led_state = host_keyboard_led_state();

    oled_set_cursor(0, 0);
    oled_write_P(PSTR("Num:"), false);
    oled_write_P(PSTR(led_state.num_lock ? " On " : " Off"), led_state.num_lock);

    oled_set_cursor(oled_max_chars() - 9, 0);
    oled_write_P(PSTR("Caps:"), false);
    oled_write_P(PSTR(led_state.caps_lock ? " On " : " Off"), led_state.caps_lock);
}

void render_layer(void) {
    oled_set_cursor(0, 1);
    oled_write_P(PSTR("L:"), false);

    switch (get_highest_layer(layer_state)) {
        case 0:
            oled_write_P(PSTR(" L0 "), false);
            break;
        case 1:
            oled_write_P(PSTR(" L1 "), true);
            break;
        case 2:
            oled_write_P(PSTR(" L2 "), true);
            break;
        case 3:
            oled_write_P(PSTR(" L3 "), true);
            break;
    }
}

void render_encoder_mode(void) {
    oled_set_cursor(9, 1);
    oled_write_P(PSTR(" Enc:"), false);
    oled_write_P(PSTR(encoder_mode_names[enc_mode]), false);
}

void render_rect_bar_row(uint8_t row, uint8_t value, bool valid, const char *label, const char *value_str) {
    uint8_t cols     = oled_max_chars();
    uint8_t filled   = 0;
    uint8_t llen     = 0;
    uint8_t vslen    = 0;

    if (valid) {
        if (value > 100) value = 100;
        filled = (uint8_t)(((uint16_t)value * cols + 50) / 100);
    }

    while (label[llen] != '\0') llen++;
    if (value_str) {
        while (value_str[vslen] != '\0') vslen++;
    }

    /* value_str is right-aligned; compute where it starts */
    uint8_t vs_start = (vslen <= cols) ? (cols - vslen) : 0;

    oled_set_cursor(0, row);
    for (uint8_t col = 0; col < cols; col++) {
        char c;
        if (col < llen) {
            c = label[col];
        } else if (value_str && col >= vs_start) {
            c = value_str[col - vs_start];
        } else {
            c = ' ';
        }
        oled_write_char(c, col < filled);
    }
}

/* Format a network speed from its two split fields.
 * mbps: integer Mbps part (0 = sub-Mbps speed)
 * kbps: fractional kbps part (0-999) */
static void fmt_net_speed(char *buf, uint8_t buflen, uint16_t mbps, uint16_t kbps) {
    if (mbps == 0) {
        /* sub-Mbps: e.g. " 50k" */
        snprintf(buf, buflen, "%3uk", (unsigned)kbps);
    } else if (mbps < 100) {
        /* 1-99 Mbps: show one decimal place if kbps part significant, e.g. "9.5M" */
        if (kbps >= 100) {
            snprintf(buf, buflen, "%u.%uM", (unsigned)mbps, (unsigned)(kbps / 100));
        } else {
            snprintf(buf, buflen, "%3uM", (unsigned)mbps);
        }
    } else if (mbps < 1000) {
        /* 100-999 Mbps: no decimal (would overflow buffer), e.g. "600M" */
        snprintf(buf, buflen, "%3uM", (unsigned)mbps);
    } else if (mbps < 10000) {
        /* Gbps range with decimal: e.g. "1.2G" */
        snprintf(buf, buflen, "%u.%uG", (unsigned)(mbps / 1000), (unsigned)((mbps % 1000) / 100));
    } else {
        /* High Gbps: e.g. " 10G" */
        snprintf(buf, buflen, "%3uG", (unsigned)(mbps / 1000));
    }
}

void render_net_row(uint8_t row) {
    uint8_t  cols = oled_max_chars();
    uint8_t  half = cols / 2;
    char     line[22];
    uint8_t  i;

    for (i = 0; i < 21; i++) line[i] = ' ';
    line[21] = '\0';

    if (host_telemetry_is_fresh()) {
        char dl[6] = "---";
        char ul[6] = "---";

        if (host_telemetry.valid_mask & TELEMETRY_VALID_NET_DOWN) {
            fmt_net_speed(dl, sizeof(dl), host_telemetry.net_down_mbps, host_telemetry.net_down_kbps);
        }
        if (host_telemetry.valid_mask & TELEMETRY_VALID_NET_UP) {
            fmt_net_speed(ul, sizeof(ul), host_telemetry.net_up_mbps, host_telemetry.net_up_kbps);
        }

        line[0] = 'D';
        line[1] = ':';
        for (i = 0; dl[i] != '\0' && (2 + i) < half; i++) {
            line[2 + i] = dl[i];
        }

        line[half] = 'U';
        line[half + 1] = ':';
        for (i = 0; ul[i] != '\0' && (half + 2 + i) < 21; i++) {
            line[half + 2 + i] = ul[i];
        }
    }

    oled_set_cursor(0, row);
    oled_write(line, false);
}

void render_normal_mode(void) {
    render_lock_status();
    render_layer();
    render_encoder_mode();

    if (!host_telemetry_is_fresh()) {
        oled_set_cursor(0, 4);
        oled_write_P(PSTR("Artemis: offline     "), false);
        oled_set_cursor(0, 5);
        oled_write_P(PSTR("Awaiting host data   "), false);
        oled_set_cursor(0, 6);
        oled_write_P(PSTR("Send telemetry pkt   "), false);
        oled_set_cursor(0, 7);
        oled_write_P(PSTR("every ~500ms         "), false);
        return;
    }

    /* VOL bar — show percent value */
    {
        bool valid = (host_telemetry.valid_mask & TELEMETRY_VALID_VOLUME) != 0;
        char vs[5] = "---";
        if (valid) snprintf(vs, sizeof(vs), "%u%%", host_telemetry.volume_percent);
        render_rect_bar_row(4, host_telemetry.volume_percent, valid, "VOL", vs);
    }

    /* CPU bar — show percent value */
    {
        bool valid = (host_telemetry.valid_mask & TELEMETRY_VALID_CPU) != 0;
        char vs[5] = "---";
        if (valid) snprintf(vs, sizeof(vs), "%u%%", host_telemetry.cpu_percent);
        render_rect_bar_row(5, host_telemetry.cpu_percent, valid, "CPU", vs);
    }

    /* RAM bar — show GB used (base size configured by RAM_SIZE_GB in helpers.c) */
    {
        bool valid = (host_telemetry.valid_mask & TELEMETRY_VALID_RAM) != 0;
        char vs[7] = "---";
        if (valid) {
            uint16_t gb_x10 = (uint16_t)(((uint32_t)host_telemetry.ram_percent * RAM_SIZE_GB * 10 + 50) / 100);
            snprintf(vs, sizeof(vs), "%u.%uG", gb_x10 / 10, gb_x10 % 10);
        }
        render_rect_bar_row(6, host_telemetry.ram_percent, valid, "RAM", vs);
    }

    render_net_row(7);
}

void render_encoder_select_mode(void) {
    oled_set_cursor(0, 0);
    oled_write_P(PSTR("Encoder:"), false);
    oled_set_cursor(oled_max_chars() - 9, 0);
    oled_write_P(PSTR("Function:"), false);

    for (int i = 0; i < ENCODER_MODES_COUNT; i++) {
        oled_set_cursor(0, 2 + i);
        oled_write_P(PSTR(encoder_mode_names[i]), i == enc_mode);
    }

    for (int i = 0; i < ENCODER_SELECT; i++) {
        uint8_t col = oled_max_chars() - strlen(display_mode_names[i]);
        oled_set_cursor(col, 2 + i);
        oled_write_P(PSTR(display_mode_names[i]), i == display_mode_selector);
    }
}

static void oled_write_padded_token(const char *text, uint8_t width, bool invert) {
    for (uint8_t i = 0; i < width; i++) {
        char c = text[i] != '\0' ? text[i] : ' ';
        oled_write_char(c, invert);
    }
}

void render_cad_status_row(void) {
    const char *app = display_mode == CAD_FUSION ? "FUSION" : "ONSHAPE";
    const char *action = "---";
    const char *sens = cad_btn_b_held ? " LOW " : "NORM";

    if (is_pan_enabled) {
        action = " PAN ";
    } else {
        action = " ROT ";
    }

    oled_set_cursor(0, 2);
    oled_write_padded_token(app, 7, false);
    oled_write("    ", false);
    oled_write_padded_token(action, 5, is_pan_enabled);
    oled_write_padded_token(sens, 5, cad_btn_b_held);
}

bool oled_task_user(void) {
    static uint8_t previous_display_mode = NORMAL;

    if (timer_elapsed(timer) < 3000) {
        return false;
    }

    if (!logo_cleared) {
        oled_clear();
        logo_cleared = true;
    }

    if (previous_display_mode != display_mode) {
        oled_clear();
        previous_display_mode = display_mode;
    }

    switch (display_mode) {
        case NORMAL:
            render_normal_mode();
            break;
        case ENCODER_SELECT:
            render_encoder_select_mode();
            break;
        case CALCULATOR:
            render_calc();
            break;
        case CAD_ONSHAPE:
        case CAD_FUSION:
            render_normal_mode();
            render_cad_status_row();
            break;
    }

    return true;
}

void keyboard_post_init_user(void) {
    render_qmk_logo();
}

void oled_render_boot(bool bootloader) {
    oled_clear();
    for (int i = 0; i < 7; i++) {
        oled_set_cursor(0, i);
        if (bootloader) {
            oled_write_P(PSTR("Awaiting New Firmware "), false);
        } else {
            oled_write_P(PSTR("Rebooting "), false);
        }
    }

    oled_render_dirty(true);
}

bool shutdown_user(bool jump_to_bootloader) {
    oled_render_boot(jump_to_bootloader);

    return true;
}
