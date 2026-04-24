#include "keycodes.h"
#include QMK_KEYBOARD_H
#include "helpers.c"

void left_encoder_pressed(bool pressed) {
    switch (enc_mode) {
        case DEFAULT:
            if (pressed) {
                tap_code(KC_NUM_LOCK);
            }
            break;
        case BRIGHTNESS:
            if (pressed) {
                register_code(MS_BTN3);
            } else {
                unregister_code(MS_BTN3);
            }
            break;
        case MOUSE:
            if (pressed) {
                register_code(MS_BTN1);
            } else {
                unregister_code(MS_BTN1);
            }
            break;
        case TEXT:
            if (pressed) {
                register_code(KC_LCTL);
                tap_code(KC_C);
                unregister_code(KC_LCTL);
            }
            break;
        case APPSW:
            if (pressed) {
                tap_code(KC_ENTER);
            }
            break;
        case MEDIA:
            if (pressed) {
                tap_code(KC_MPLY);
            }
            break;
            // case PONG:
            //     if (pressed) {
            //         // pong_input(player_left, pressed);
            //         pong_press(player_left);
            //     }
            break;
        default:
            break;
    }
}

void right_encoder_pressed(bool pressed) {
    switch (enc_mode) {
        case DEFAULT:
            if (pressed) {
                tap_code(KC_MUTE);
            }
            break;
        case BRIGHTNESS:
            if (pressed) {
                register_code(MS_BTN3);
            } else {
                unregister_code(MS_BTN3);
            }
            break;
        case MOUSE:
            if (pressed) {
                register_code(MS_BTN2);
            } else {
                unregister_code(MS_BTN2);
            }
            break;
        case TEXT:
            if (pressed) {
                register_code(KC_LCTL);
                tap_code(KC_V);
                unregister_code(KC_LCTL);
            }
            break;
        case APPSW:
            if (pressed) {
                register_code(KC_LALT);
                tap_code(KC_ESC);
                unregister_code(KC_LALT);
            }
            break;
        case MEDIA:
            if (pressed) {
                tap_code(KC_MUTE);
            }
            break;
        // case PONG:
        //     if (pressed) {
        //         // pong_input(player_right, pressed);
        //         pong_press(player_right);
        //     }
        //     break;
        default:
            break;
    }
}

#ifdef ENCODER_ENABLE
void left_encoder_cw(void) {
    if (display_mode == ENCODER_SELECT) {
        if (enc_mode == 0) {
            enc_mode = ENCODER_MODES_COUNT - 1;
        } else {
            enc_mode = (enc_mode - 1) % ENCODER_MODES_COUNT;
        }
        return;
    }
    uint8_t current_layer = get_highest_layer(layer_state);
    switch (enc_mode) {
        case DEFAULT:
        case BRIGHTNESS:
            if (current_layer == 1) {
                register_code(KC_LCTL);
                tap_code(KC_F14);
                unregister_code(KC_LCTL);
            } else if (current_layer == 2) {
                register_code(KC_LSFT);
                tap_code(KC_F14);
                unregister_code(KC_LSFT);
            } else {
                tap_code(KC_F14);
            }
            break;
        case MOUSE:
            tap_code(MS_WHLU);
            break;
        case TEXT:
            register_code(KC_LCTL);
            register_code(KC_LSFT);
            tap_code(KC_W);
            unregister_code(KC_LSFT);
            unregister_code(KC_LCTL);
            break;
        case APPSW:
            register_code(KC_LCTL);
            register_code(KC_LALT);
            register_code(KC_LSFT);
            wait_ms(20);
            tap_code(KC_TAB);
            unregister_code(KC_LSFT);
            unregister_code(KC_LALT);
            unregister_code(KC_LCTL);
            break;
        case MEDIA:
            tap_code(KC_MPRV);
            break;
        // case PONG:
        // pong_input(player_left, clockwise);
        // break;
        default:
            break;
    }
}

void left_encoder_ccw(void) {
    if (display_mode == ENCODER_SELECT) {
        enc_mode = (enc_mode + 1) % ENCODER_MODES_COUNT;
        return;
    }
    uint8_t current_layer = get_highest_layer(layer_state);
    switch (enc_mode) {
        case DEFAULT:
        case BRIGHTNESS:
            if (current_layer == 1) {
                register_code(KC_LCTL);
                tap_code(KC_F15);
                unregister_code(KC_LCTL);
            } else if (current_layer == 2) {
                register_code(KC_LSFT);
                tap_code(KC_F15);
                unregister_code(KC_LSFT);
            } else {
                tap_code(KC_F15);
            }
            break;
        case MOUSE:
            tap_code(MS_DOWN);
            break;
        case TEXT:
            register_code(KC_LCTL);
            tap_code(KC_W);
            unregister_code(KC_LCTL);
            break;
        case APPSW:
            register_code(KC_LCTL);
            register_code(KC_LALT);
            wait_ms(20);
            tap_code(KC_TAB);
            unregister_code(KC_LALT);
            unregister_code(KC_LCTL);
            break;
        case MEDIA:
            tap_code(KC_MNXT);
            break;
        // case PONG:
        // pong_input(player_left, anticlockwise);
        // break;
        default:
            break;
    }
}

void right_encoder_cw(void) {
    if (display_mode == ENCODER_SELECT) {
        if (display_mode_selector == 0) {
            display_mode_selector = ENCODER_SELECT - 1;
        } else {
            display_mode_selector = (display_mode_selector - 1) % ENCODER_SELECT;
        }
        return;
    }
    uint8_t current_layer = get_highest_layer(layer_state);
    switch (enc_mode) {
        case DEFAULT:
            tap_code(KC_VOLD);
            break;
        case BRIGHTNESS:
            if (current_layer == 1) {
                register_code(KC_LCTL);
                register_code(KC_LCTL);
                tap_code(KC_F14);
                unregister_code(KC_LCTL);
                unregister_code(KC_LCTL);
            } else if (current_layer == 2) {
                register_code(KC_LCTL);
                register_code(KC_LSFT);
                tap_code(KC_F14);
                unregister_code(KC_LSFT);
                unregister_code(KC_LCTL);
            } else {
                register_code(KC_LCTL);
                tap_code(KC_F14);
                unregister_code(KC_LCTL);
            }
            break;
        case MOUSE:
            tap_code(MS_LEFT);
            break;
        case TEXT:
            register_code(KC_LCTL);
            tap_code(KC_LEFT);
            unregister_code(KC_LCTL);
            break;
        case APPSW:
            register_code(KC_LCTL);
            register_code(KC_LSFT);
            wait_ms(20);
            tap_code(KC_TAB);
            unregister_code(KC_LSFT);
            unregister_code(KC_LCTL);
            break;
        case MEDIA:
            tap_code(KC_VOLD);
            break;
        // case PONG:
        // pong_input(player_right, clockwise);
        // break;
        default:
            break;
    }
}

void right_encoder_ccw(void) {
    if (display_mode == ENCODER_SELECT) {
        display_mode_selector = (display_mode_selector + 1) % ENCODER_SELECT;
        return;
    }
    uint8_t current_layer = get_highest_layer(layer_state);
    switch (enc_mode) {
        case DEFAULT:
            tap_code(KC_VOLU);
            break;
        case BRIGHTNESS:
            if (current_layer == 1) {
                register_code(KC_LCTL);
                register_code(KC_LCTL);
                tap_code(KC_F15);
                unregister_code(KC_LCTL);
                unregister_code(KC_LCTL);
            } else if (current_layer == 2) {
                register_code(KC_LCTL);
                register_code(KC_LSFT);
                tap_code(KC_F15);
                unregister_code(KC_LSFT);
                unregister_code(KC_LCTL);
            } else {
                register_code(KC_LCTL);
                tap_code(KC_F15);
                unregister_code(KC_LCTL);
            }
            break;
        case MOUSE:
            tap_code(MS_RGHT);
            break;
        case TEXT:
            register_code(KC_LCTL);
            tap_code(KC_RIGHT);
            unregister_code(KC_LCTL);
            break;
        case APPSW:
            register_code(KC_LCTL);
            wait_ms(20);
            tap_code(KC_TAB);
            unregister_code(KC_LCTL);
            break;
        case MEDIA:
            tap_code(KC_VOLU);
            break;
        // case PONG:
        // pong_input(player_right, anticlockwise);
        // break;
        default:
            break;
    }
}
#endif
