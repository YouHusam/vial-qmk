/* Copyright 2022 Jose Pablo Ramirez <jp.ramangulo@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H

#ifdef VIA_ENABLE
#    include "via.h"
#endif

#define TELEMETRY_CMD_VENDOR_SET 0xFC

#include <helpers/helpers.c>
#include <helpers/encoders.c>

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_MEDIA_PREV_TRACK, KC_MEDIA_NEXT_TRACK, LT(1, LENC), LT(2, RENC), MS_BTN1, KC_KP_7, KC_KP_8, KC_KP_9, KC_KP_SLASH, KC_KP_4, KC_KP_5, KC_KP_6, KC_KP_ASTERISK, KC_KP_1, KC_KP_2, KC_KP_3, KC_KP_MINUS, KC_KP_0, KC_KP_DOT, KC_KP_PLUS, KC_KP_ENTER

                 ),
    [1] = LAYOUT(QK_BOOT, QK_RBT, LT(3, LENC), MODE_SELECT, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______

                 ),
    [2] = LAYOUT(_______, _______, MODE_SELECT, LT(3, RENC), _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, QK_CLEAR_EEPROM, _______, _______, _______

                 ),
    [3] = LAYOUT(MODE_SELECT, _______, LENC, RENC, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, QK_CLEAR_EEPROM, _______, _______, _______),
};

#ifdef ENCODER_MAP_ENABLE
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = {ENCODER_CCW_CW(MS_WHLD, MS_WHLU), ENCODER_CCW_CW(KC_VOLU, KC_VOLD)},
    [1] = {ENCODER_CCW_CW(_______, _______), ENCODER_CCW_CW(_______, _______)},
    [2] = {ENCODER_CCW_CW(_______, _______), ENCODER_CCW_CW(_______, _______)},
    [3] = {ENCODER_CCW_CW(_______, _______), ENCODER_CCW_CW(_______, _______)},
};
#endif

#ifdef OLED_ENABLE
#    include <helpers/display.c>
#endif

static void cad_toggle_pan(void) {
    if (cad_btn_a_held) {
        cad_release_all();
        return;
    }

    cad_release_all();
    register_code(MS_BTN3);
    cad_btn_a_held   = true;
    cad_last_move_ms = timer_read32();
}

static void cad_toggle_rotate(void) {
    if (cad_btn_b_held) {
        cad_release_all();
        return;
    }

    cad_release_all();
    if (display_mode == CAD_ONSHAPE) {
        register_code(MS_BTN2);
    } else {
        register_mods(MOD_BIT(KC_LSFT));
        send_keyboard_report();
        wait_ms(10);
        register_code(MS_BTN3);
    }
    cad_btn_b_held   = true;
    cad_last_move_ms = timer_read32();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    bool is_cad_mode = display_mode == CAD_ONSHAPE || display_mode == CAD_FUSION;

    if (is_cad_mode) {
        /* Handle physical A/B buttons by matrix location so Vial remaps stay compatible. */
        if (record->event.key.row == 0 && record->event.key.col == 0) {
            if (record->event.pressed) {
                cad_toggle_pan();
            }
            return false;
        }

        if (record->event.key.row == 0 && record->event.key.col == 1) {
            if (record->event.pressed) {
                cad_toggle_rotate();
            }
            return false;
        }
    }

    switch (keycode) {
        case LT(1, LENC):
        case LENC:
            if (record->event.pressed && record->tap.count) {
                if (display_mode == ENCODER_SELECT) {
                    display_mode = NORMAL;
                    return false;
                }
                if (display_mode == CALCULATOR) {
                    process_record_user_calc(keycode, record);
                    return false;
                    break;
                }
                left_encoder_pressed(record->event.pressed);
                return false;
            } else if (!record->event.pressed && record->tap.count) {
                left_encoder_pressed(record->event.pressed);
                return false;
            }
            break;
        case LT(2, RENC):
        case RENC:
            if (display_mode == CALCULATOR) {
                process_record_user_calc(keycode, record);
                return false;
                break;
            }
            if (record->event.pressed && record->tap.count) {
                if (display_mode == ENCODER_SELECT) {
                    display_mode = display_mode_selector;
                    if (display_mode == CAD_ONSHAPE || display_mode == CAD_FUSION) {
                        cad_release_all();
                        cad_last_move_ms = timer_read32();
                    }
                    return false;
                }
                right_encoder_pressed(record->event.pressed);
                return false;
            } else if (!record->event.pressed && record->tap.count) {
                right_encoder_pressed(record->event.pressed);
                return false;
            }
            break;
        case MODE_SELECT:
            if (record->event.pressed) {
                if (display_mode == CAD_ONSHAPE || display_mode == CAD_FUSION) {
                    cad_release_all();
                    display_mode_selector = display_mode;
                    display_mode          = ENCODER_SELECT;
                    return false;
                }
                display_mode_selector = display_mode;
                display_mode          = ENCODER_SELECT;
            }
            return false;
            break;
        default:
            if (record->event.pressed) {
                if (display_mode == CALCULATOR) {
                    process_record_user_calc(keycode, record);
                    return false;
                }
                return (display_mode != ENCODER_SELECT) || (display_mode != CALCULATOR);
            }
    }
    return true;
}

#ifdef ENCODER_ENABLE

bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index == 0) {
        if (clockwise) {
            left_encoder_cw();
        } else {
            left_encoder_ccw();
        }
    } else if (index == 1) {
        if (clockwise) {
            right_encoder_cw();
        } else {
            right_encoder_ccw();
        }
    }
    return false;
}
#endif

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    bool is_cad_mode = display_mode == CAD_ONSHAPE || display_mode == CAD_FUSION;
    bool cad_active  = cad_btn_a_held || cad_btn_b_held;

    if (!is_cad_mode || !cad_active) {
        return mouse_report;
    }

    if (mouse_report.x != 0 || mouse_report.y != 0) {
        cad_last_move_ms = timer_read32();
    } else if (timer_elapsed32(cad_last_move_ms) > CAD_IDLE_TIMEOUT_MS) {
        cad_release_all();
    }

    return mouse_report;
}

#ifdef VIA_ENABLE
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &(data[0]);

    if (*command_id != TELEMETRY_CMD_VENDOR_SET) {
        *command_id = id_unhandled;
        return;
    }

    if (length < 2 || data[1] != TELEMETRY_VALUE_HOST_STATUS) {
        *command_id = id_unhandled;
        return;
    }

    uint8_t *value_data        = &(data[2]);
    uint8_t  value_data_length = length > 2 ? (uint8_t)(length - 2) : 0;

    if (!host_telemetry_update_from_payload(value_data, value_data_length)) {
        *command_id = id_unhandled;
    }
}
#endif
