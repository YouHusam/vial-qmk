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

#include "dynamic_keymap.h"
#include "cad_mode.h"

#define TELEMETRY_CMD_VENDOR_SET 0xFC

#include <helpers/helpers.c>
#include <helpers/encoders.c>
#include "../../cad_mode.c"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Layer 0: Main numeric pad with CAD mode support */
    [0] = LAYOUT(
        KC_MEDIA_PREV_TRACK,  KC_MEDIA_NEXT_TRACK,  LT(1, LENC),    LT(2, RENC),
        MS_BTN1,
        KC_KP_7,              KC_KP_8,              KC_KP_9,         KC_KP_SLASH,
        KC_KP_4,              KC_KP_5,              KC_KP_6,         KC_KP_ASTERISK,
        KC_KP_1,              KC_KP_2,              KC_KP_3,         KC_KP_MINUS,
        KC_KP_0,              KC_KP_DOT,            KC_KP_PLUS,      KC_KP_ENTER
    ),
    /* Layer 1: Boot and encoder mode select */
    [1] = LAYOUT(
        QK_BOOT,              QK_RBT,               LT(3, LENC),     MODE_SELECT,
        _______,
        _______,              _______,              _______,         _______,
        _______,              _______,              _______,         _______,
        _______,              _______,              _______,         _______,
        _______,              _______,              _______,         _______
    ),
    /* Layer 2: Mode select and encoder mode select */
    [2] = LAYOUT(
        _______,              _______,              MODE_SELECT,     LT(3, RENC),
        _______,
        _______,              _______,              _______,         _______,
        _______,              _______,              _______,         _______,
        _______,              _______,              _______,         _______,
        _______,              QK_CLEAR_EEPROM,     _______,         _______
    ),
    /* Layer 3: Encoder navigation */
    [3] = LAYOUT(
        MODE_SELECT,          _______,              LENC,            RENC,
        _______,
        _______,              _______,              _______,         _______,
        _______,              _______,              _______,         _______,
        _______,              _______,              _______,         _______,
        _______,              QK_CLEAR_EEPROM,     _______,         _______
    ),
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

#ifdef VIAL_COMBO_ENABLE
static void ensure_default_vial_combo(void) {
    bool any_combo_defined = false;

    for (uint8_t i = 0; i < VIAL_COMBO_ENTRIES; i++) {
        vial_combo_entry_t entry = {0};
        if (dynamic_keymap_get_combo(i, &entry) == 0) {
            if (entry.output != KC_NO || entry.input[0] != KC_NO || entry.input[1] != KC_NO || entry.input[2] != KC_NO || entry.input[3] != KC_NO) {
                any_combo_defined = true;
                break;
            }
        }
    }

    if (!any_combo_defined && VIAL_COMBO_ENTRIES > 0) {
        vial_combo_entry_t entry = {
            .input = {KC_MPRV, KC_MNXT, KC_NO, KC_NO},
            .output = KC_MPLY,
        };
        dynamic_keymap_set_combo(0, &entry);
    }
}
#endif

void matrix_init_user(void) {
#ifdef VIAL_COMBO_ENABLE
    ensure_default_vial_combo();
#endif
}

#ifdef COMBO_SHOULD_TRIGGER
bool combo_should_trigger(uint16_t combo_index, combo_t *combo, uint16_t keycode, keyrecord_t *record) {
    (void)combo_index;
    (void)combo;
    (void)keycode;
    (void)record;

    return display_mode == NORMAL;
}
#endif



bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    bool is_cad_mode = display_mode == CAD_ONSHAPE || display_mode == CAD_FUSION;

    if (is_cad_mode) {
        /* Handle physical A/B buttons by matrix location so Vial remaps stay compatible. */
        if (record->event.key.row == 0 && record->event.key.col == 0) {
            if (record->event.pressed) {
                /* Toggle pan mode on A button press */
                is_pan_enabled = !is_pan_enabled;
                /* Release any active action when toggling pan off */
                if (!is_pan_enabled && cad_action != CAD_ACTION_NONE) {
                    cad_release_all();
                }
            }
            return false;
        }

        if (record->event.key.row == 0 && record->event.key.col == 1) {
            if (record->event.pressed) {
                cad_btn_b_held = !cad_btn_b_held;
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
    report_mouse_t raw_report = mouse_report;
    bool           has_motion;

    if (!is_cad_mode) {
        return mouse_report;
    }

    has_motion = abs(raw_report.x) >= CAD_MOTION_THRESHOLD || abs(raw_report.y) >= CAD_MOTION_THRESHOLD;

    /* Apply precision scaling when B button is held */
    if (cad_btn_b_held) {
        mouse_report.x = cad_scale_precision(mouse_report.x);
        mouse_report.y = cad_scale_precision(mouse_report.y);
    }

    /* Pan mode: motion-gated like rotate (only press buttons when moving) */
    if (is_pan_enabled) {
        if (has_motion) {
            cad_pan_last_motion_ms = timer_read32();
            cad_press_pan();
        } else if (cad_action == CAD_ACTION_PAN && timer_elapsed32(cad_pan_last_motion_ms) > CAD_ROTATE_RELEASE_MS) {
            cad_release_pan();
        }
    } else if (!is_pan_enabled && cad_action == CAD_ACTION_PAN) {
        /* Release pan if it was active but pan mode is now disabled */
        cad_release_pan();
    }

    /* Rotate mode: motion-gated (only press buttons when moving) */
    if (!is_pan_enabled) {
        if (has_motion) {
            cad_rotate_last_motion_ms = timer_read32();
            cad_press_rotate();
        } else if (cad_action == CAD_ACTION_ROTATE && timer_elapsed32(cad_rotate_last_motion_ms) > CAD_ROTATE_RELEASE_MS) {
            cad_release_rotate();
        }
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
