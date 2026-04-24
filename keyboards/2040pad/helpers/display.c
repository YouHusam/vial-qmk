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
    oled_write_P(PSTR("Layer:"), false);

    switch (get_highest_layer(layer_state)) {
        case 0:
            oled_write_P(PSTR(" Numpad  "), false);
            break;
        case 1:
            oled_write_P(PSTR(" Layer 1"), true);
            break;
        case 2:
            oled_write_P(PSTR(" Layer 2"), true);
            break;
        case 3:
            oled_write_P(PSTR(" Layer 3"), true);
            break;
    }
}

void render_encoder_mode(void) {
    oled_set_cursor(0, 2);
    oled_write_P(PSTR("Enc Mode: "), false);
    oled_write_P(PSTR(encoder_mode_names[enc_mode]), false);
}

void render_rect_bar_row(uint8_t row, uint8_t value, bool valid, const char *label) {
    uint8_t cols   = oled_max_chars();
    uint8_t llen   = (uint8_t)strlen(label);
    uint8_t filled = 0;

    if (valid) {
        if (value > 100) value = 100;
        /* Full-width character bar; rounded for smoother transitions. */
        filled = (uint8_t)(((uint16_t)value * cols + 50) / 100);
    }

    oled_set_cursor(0, row);
    for (uint8_t i = 0; i < cols; i++) {
        bool inv = (i < filled);
        oled_write_char(i < llen ? label[i] : ' ', inv);
    }
}

static void fmt_kbps(char *buf, uint8_t buflen, uint16_t kbps) {
    if (kbps < 1000) {
        snprintf(buf, buflen, "%3uk", (unsigned)kbps);
    } else if (kbps < 10000) {
        snprintf(buf, buflen, "%u.%uM", kbps / 1000, (kbps % 1000) / 100);
    } else {
        snprintf(buf, buflen, "%3uM", kbps / 1000);
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
            fmt_kbps(dl, sizeof(dl), host_telemetry.net_down_kbps);
        }
        if (host_telemetry.valid_mask & TELEMETRY_VALID_NET_UP) {
            fmt_kbps(ul, sizeof(ul), host_telemetry.net_up_kbps);
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

    uint8_t metrics[3] = {
        host_telemetry.volume_percent,
        host_telemetry.cpu_percent,
        host_telemetry.ram_percent,
    };
    uint8_t masks[3] = {
        TELEMETRY_VALID_VOLUME,
        TELEMETRY_VALID_CPU,
        TELEMETRY_VALID_RAM,
    };
    const char *labels[3] = {
        "VOL",
        "CPU",
        "RAM",
    };

    for (uint8_t row = 0; row < 3; row++) {
        render_rect_bar_row(row + 4, metrics[row], (host_telemetry.valid_mask & masks[row]) != 0, labels[row]);
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
