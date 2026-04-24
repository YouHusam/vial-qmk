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

#pragma once

#include QMK_KEYBOARD_H

/* CAD mode constants for precision and motion detection */
#define CAD_PRECISION_DIVISOR   3   /* Decrease joystick sensitivity when precision mode is enabled */
#define CAD_MOTION_THRESHOLD    1   /* Minimum joystick movement to trigger motion detection */
#define CAD_ROTATE_RELEASE_MS   500 /* Rotate mode motion timeout before auto-release */

/* CAD action state enumeration */
enum cad_action_state {
    CAD_ACTION_NONE,
    CAD_ACTION_PAN,
    CAD_ACTION_ROTATE,
};

/* Extern state variables (defined in helpers.c) */
extern bool     is_pan_enabled;
extern bool     cad_btn_b_held;
extern uint8_t  cad_action;
extern uint8_t  cad_held_buttons;
extern bool     cad_shift_held;
extern uint32_t cad_pan_last_motion_ms;
extern uint32_t cad_rotate_last_motion_ms;

/* CAD mode action functions */
void cad_press_pan(void);
void cad_press_rotate(void);
void cad_release_active_action(void);
void cad_set_pan_enabled(bool enabled);
int8_t cad_scale_precision(int8_t value);
