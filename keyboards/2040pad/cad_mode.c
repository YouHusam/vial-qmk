#include QMK_KEYBOARD_H
#include "cad_mode.h"

/*
 * Button state is written into cad_held_buttons and applied once per scan
 * inside pointing_device_task_user. This keeps buttons and joystick movement
 * in the same HID report, avoiding the dual-report race that mousekey causes.
 */

static void cad_release_pan(void) {
    if (cad_action != CAD_ACTION_PAN) {
        return;
    }

    cad_held_buttons = 0;
    cad_action       = CAD_ACTION_NONE;
}

static void cad_release_rotate(void) {
    if (cad_action != CAD_ACTION_ROTATE) {
        return;
    }

    cad_held_buttons = 0;

    if (cad_shift_held) {
        unregister_mods(MOD_BIT(KC_LSFT));
        del_weak_mods(MOD_BIT(KC_LSFT));
        send_keyboard_report();
        cad_shift_held = false;
    }

    cad_action = CAD_ACTION_NONE;
}

/*
 * Called each scan while joystick is moving and pan mode is active.
 * Pan = middle click for all CAD apps.
 */
void cad_press_pan(void) {
    if (cad_action == CAD_ACTION_PAN) {
        return;
    }

    if (cad_action == CAD_ACTION_ROTATE) {
        cad_release_rotate();
    }

    if (display_mode == CAD_ONSHAPE) {
        cad_held_buttons = MOUSE_BTN3;  /* middle click */
    } else if (display_mode == CAD_FUSION) {
        cad_held_buttons = MOUSE_BTN3;  /* middle click */
    }

    cad_action             = CAD_ACTION_PAN;
    cad_pan_last_motion_ms = timer_read32();
}

/*
 * Called each scan while joystick is moving and rotate mode is active.
 * Rotate = right click for Onshape, Shift+middle click for Fusion.
 */
void cad_press_rotate(void) {
    if (cad_action == CAD_ACTION_ROTATE) {
        cad_rotate_last_motion_ms = timer_read32();
        return;
    }

    if (cad_action == CAD_ACTION_PAN) {
        cad_release_pan();
    }

    if (display_mode == CAD_ONSHAPE) {
        cad_held_buttons = MOUSE_BTN2;  /* right click */
    } else if (display_mode == CAD_FUSION) {
        register_mods(MOD_BIT(KC_LSFT));
        send_keyboard_report();
        wait_ms(10); /* ensure the shift-modified report is sent before the button press */
        cad_shift_held   = true;
        cad_held_buttons = MOUSE_BTN3;  /* middle click + shift */
    }

    cad_action                = CAD_ACTION_ROTATE;
    cad_rotate_last_motion_ms = timer_read32();
}

void cad_release_active_action(void) {
    if (cad_action == CAD_ACTION_PAN) {
        cad_release_pan();
    } else if (cad_action == CAD_ACTION_ROTATE) {
        cad_release_rotate();
    }
}

void cad_set_pan_enabled(bool enabled) {
    if (is_pan_enabled == enabled) {
        return;
    }

    /* Always release any active action before switching mode. */
    cad_release_active_action();
    is_pan_enabled = enabled;

    if (enabled) {
        cad_pan_last_motion_ms = timer_read32();
    } else {
        cad_rotate_last_motion_ms = timer_read32();
    }
}

int8_t cad_scale_precision(int8_t value) {
    if (value == 0 || CAD_PRECISION_DIVISOR <= 1) {
        return value;
    }

    int8_t scaled = value / CAD_PRECISION_DIVISOR;
    if (scaled == 0) {
        scaled = value > 0 ? 1 : -1;
    }

    return scaled;
}
