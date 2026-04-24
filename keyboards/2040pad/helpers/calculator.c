#include QMK_KEYBOARD_H
#include <string.h>
#include <stdlib.h>
#include "quantum.h"
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "helpers.c"

/*
* -----------------------------------
* Here is the part for the calculator
* -----------------------------------
*/

enum operators { enter, add, subtract, multiply, divide, empty };
int               has_dot             = false;  // Is there already a dot in big_output?
int               positive            = true;   // sign of big_output
int               current_index_big   = 0;      // current position in the big output + indicator for 0 division
unsigned int      current_index_small = 0;      // current position in the small output
const uint8_t     MAX_SMALL           = 21;
const uint8_t     MAX_BIG             = 11;
static const char reset_big[12]       = {'0', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
static const char reset_small[22]     = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
char              big_output[12]      = {'0', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
char              small_output[22]    = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
int               operator            = empty;
int               is_op               = false;  // flag to know if last action was an operator (excluding =)
double small_value = 0.0;  // computed value in small area
double big_value   = 0.0;  // value currently being displayed
struct RightValue {
    double content; //value of RightValue
    char   str[12]; // string representation of the value
    int    validity; // whether or not it should be considered for condition check
};
struct RightValue right_value = {0.0, "", 0};

/*
 * Function:  get_length
 * --------------------
 * give the length of char array
 * only used for a particular case
 *  c: char array
 *
 *  returns: give the length of the array considering anything but ' ' as non empty
 *           stops at the first ' ' encountered going from left to right
 */
int get_length(char *c) {
    int len = strlen(c);
    for (int i = 0; i < len; i++) {
        if (c[i] == ' ') {
            return (i);
        }
    }
    return (len);
}


/*
* Display section
*/

char output[] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00
};


/*
 * Function:  set_small_output
 * --------------------
 * Write the value of small_output to output
 * Mapping between ascii table and glcfont.c is direct
 */
void set_small_output(void){
    for (int i=42; i<63; i++){
        output[i] = 0x20;
    }
    int end = get_length(small_output) - 1;
    for (int i=end; i>=0; i--){
        output[62 - (end - i)] = small_output[i];
    }
}

/*
 * Function:  add_2x3_char
 * --------------------
 * Add to a string a character built with 6 tiles defined in glcfont.c
 */
void add_2x3_char(char* out, char in, int* anchor, int offset, int shift){
    out[*anchor - 1 - offset] = in + shift;
    out[*anchor -1 + 21 - offset] = in + shift + 32;
    out[*anchor -1 + 42 - offset] = in + shift + 64;
    out[*anchor - offset] = in + shift + 1;
    out[*anchor + 21 - offset] = in + shift + 32 + 1;
    out[*anchor + 42 - offset] = in + shift + 64 + 1;
    --*anchor;
}

/*
 * Function:  set_big_output
 * --------------------
 * Write the value contained in big_output to the screen
 * Characters are 2x3 tiles
 */
void set_big_output(void) {
    for (int i=105; i<168; i++){
        output[i] = 0x20;
    }
    int end = get_length(big_output) - 1;
    int last = 125; // TODO SET WITH CONFIG
    for (int i = end; i >= 0; i--) {
        int offset = end - i;
        if (big_output[i] == '.'){
            output[last - offset] = big_output[i] + 102;
            output[last + 21 - offset] = big_output[i] + 102 + 32;
            output[last + 42 - offset] = big_output[i] + 102 + 64;
        }else if ((big_output[i] == '+') || (big_output[i] == '-')){
            int shift = 112;
            char c = big_output[i];
            add_2x3_char(output, c, &last, offset, shift);
        }else if (big_output[i] == 'E'){
            int shift = 80;
            char c = big_output[i];
            add_2x3_char(output, c, &last, offset, shift);
		}else if (big_output[i] == 'a'){
			int shift = 54;
			char c = big_output[i];
			add_2x3_char(output, c, &last, offset, shift);
		}else if (big_output[i] == 'N'){
            int shift = 75;
            char c = big_output[i];
            add_2x3_char(output, c, &last, offset, shift);
        }else{
            int shift = 80 + big_output[i] - 48;
            char c = big_output[i];
            add_2x3_char(output, c, &last, offset, shift);
        }
    }
}

void render_calc(void){
    set_small_output();
    set_big_output();
    // oled_clear();
    oled_write(output, false);
}

/*
* Handling section
*/

/*
 * Function:  reset
 * --------------------
 * Resets all variables
 */
void reset(void) {
    oled_clear();
    current_index_big   = 0;
    current_index_small = 0;
    memcpy(small_output, reset_small, sizeof(reset_small));
    memcpy(big_output, reset_big, sizeof(reset_big));
    has_dot              = false;
    operator             = empty;
    positive             = true;
    right_value.validity = 0;
    is_op                = false;
    return;
}


/*
 * Function:  offset_small_output
 * --------------------
 * Modifies small_output (global) to move characters to the left by a given amount
 * Offsets character that are not overwritten remain.
 *  int: offset, by how many unit small_output is offset to the left
 *
 * Example:
 *  123456 (3) -> 456456
 */
void offset_small_output(int offset) {
    for (int i = 0; i < (MAX_SMALL - offset); i++) {
        small_output[i] = small_output[i + offset];
    }
    return;
}

/*
 * Function:  append_to_small_output
 * --------------------
 * Modifies small_output (global) to append to the current_small_index (global)
 * If there is not enough room existing characters are offset to the left
 *  c: char array that is appened to small_output
 */
void append_to_small_output(char *input) {
    int len = get_length(input);
    if ((current_index_small + len - 1) > MAX_SMALL - 1) {
        int offset = current_index_small - MAX_SMALL + len;
        offset_small_output(offset);
        for (int i = 0; i < len; i++) {
            small_output[MAX_SMALL - offset + i] = input[i];
        }
    } else {
        for (int i = 0; i < len; i++) {
            small_output[current_index_small + i] = input[i];
        }
        current_index_small += len;
    }
    return;
}

/*
 * Function:  void remove_decimals
 * --------------------
 * Takes a char array a modify it inplace to remove non significant decimals
 * Uses has_dot (global)
 *  c: char array that fill be modified
 * Examples:
 *  2.00100 -> 2.001
 *  2.00 -> 2
 */
void remove_decimals(char *c) {
    int flag = 1;
    int end  = 10;
    while (flag) {
        if ((c[end] == '0') || (c[end] == ' ')) {
            c[end] = ' ';
            end--;
        } else if (c[end] == '.') {
            c[end] = ' ';
            flag   = 0;
        } else {
            flag = 0;
        }
    }
    return;
}

/*
 * Function:  format_double_to_buffer
 * --------------------
 * Formats a double value into the big_output buffer
 * Manual formatting to avoid snprintf issues with floats
 */
void format_double_to_buffer(double value) {
    char temp[20];
    int len;

    // Handle special cases
    if (isnan(value)) {
        strcpy(big_output, "NaN");
        len = 3;
        for (int i = len; i < MAX_BIG; i++) big_output[i] = ' ';
        big_output[MAX_BIG] = '\0';
        return;
    }
    if (isinf(value)) {
        if (value > 0) {
            strcpy(big_output, "Inf");
        } else {
            strcpy(big_output, "-Inf");
        }
        len = strlen(big_output);
        for (int i = len; i < MAX_BIG; i++) big_output[i] = ' ';
        big_output[MAX_BIG] = '\0';
        return;
    }

    // Use sprintf with integer conversion for better compatibility
    double abs_val = fabs(value);

    // Scientific notation for very large/small numbers
    if (abs_val >= 1e9 || (abs_val < 0.001 && abs_val != 0.0)) {
        // Manual scientific notation formatting
        int exponent = 0;
        double mantissa = value;

        if (abs_val >= 10.0) {
            while (fabs(mantissa) >= 10.0) {
                mantissa /= 10.0;
                exponent++;
            }
        } else if (abs_val < 1.0 && abs_val > 0.0) {
            while (fabs(mantissa) < 1.0) {
                mantissa *= 10.0;
                exponent--;
            }
        }

        // Format: mantissa E exponent
        int int_part = (int)mantissa;
        int frac_part = (int)((fabs(mantissa) - fabs((double)int_part)) * 100);

        if (exponent >= 0) {
            sprintf(temp, "%d.%02dE+%d", int_part, frac_part, exponent);
        } else {
            sprintf(temp, "%d.%02dE%d", int_part, frac_part, exponent);
        }
    } else {
        // Regular number - convert to integer representation
        // Round to 6 decimal places to avoid floating point precision issues
        double rounded_value = round(value * 1000000.0) / 1000000.0;

        // Check if it's essentially an integer
        double int_check = rounded_value - (long)rounded_value;

        if (fabs(int_check) < 0.000001) {
            // It's an integer or very close to it
            sprintf(temp, "%ld", (long)rounded_value);
        } else {
            // Has decimal part - use integer arithmetic to avoid %f
            int negative = (rounded_value < 0) ? 1 : 0;
            long int_part = (long)fabs(rounded_value);
            long frac_part = (long)round((fabs(rounded_value) - int_part) * 1000000);

            if (negative) {
                sprintf(temp, "-%ld", int_part);
            } else {
                sprintf(temp, "%ld", int_part);
            }

            len = strlen(temp);

            // Add decimal part if there's room and it's non-zero
            if (frac_part > 0 && len < MAX_BIG - 2) {
                temp[len++] = '.';

                // Add up to 6 decimal places, trimming trailing zeros
                char dec_str[7];
                sprintf(dec_str, "%06ld", frac_part);

                // Trim trailing zeros
                int dec_len = 6;
                while (dec_len > 1 && dec_str[dec_len - 1] == '0') {
                    dec_len--;
                }

                // Copy what fits
                for (int i = 0; i < dec_len && len < MAX_BIG; i++) {
                    temp[len++] = dec_str[i];
                }
                temp[len] = '\0';
            }
        }
    }    // Truncate if too long
    len = strlen(temp);
    if (len > MAX_BIG) {
        len = MAX_BIG;
    }

    // Copy to big_output and pad with spaces
    memcpy(big_output, temp, len);
    for (int i = len; i < MAX_BIG; i++) {
        big_output[i] = ' ';
    }
    big_output[MAX_BIG] = '\0';
}

/*
 * Function:  parse_big_output_to_double
 * --------------------
 * Converts the big_output string to a double value
 */
double parse_big_output_to_double(void) {
    char temp[12];
    int len = get_length(big_output);

    if (len == 0) return 0.0;

    // Copy only the numeric part (no trailing spaces)
    memcpy(temp, big_output, len);
    temp[len] = '\0';

    return strtod(temp, NULL);
}

/*
 * Function:  perform_operation
 * --------------------
 * Performs arithmetic operations using standard C double
 */
void perform_operation(int op, double v1, double v2) {
    if (op == add) {
        small_value = v1 + v2;
    } else if (op == subtract) {
        small_value = v1 - v2;
    } else if (op == multiply) {
        small_value = v1 * v2;
    } else if (op == divide) {
        small_value = v1 / v2;
    } else {
        return;  // Unknown operation
    }

    big_value = small_value;
    format_double_to_buffer(big_value);
    current_index_big = get_length(big_output);

    if (isnan(big_value) || isinf(big_value)) {
        current_index_big = -1;
    }
    return;
}





/*
 * Function:  get_operator_sign
 * --------------------
 * Returns the string corresponding to the operator
 */
char *get_operator_sign(int op) {
    if (op == multiply) {
        return ("*");
    } else if (op == divide) {
        return ("/");
    } else if (op == subtract) {
        return ("-");
    } else if (op == add) {
        return ("+");
    } else if (op == enter) {
        return ("=");
    } else {
        // oh shit..
        return ("!");
    }
}

/*
 * Function:  perform_enter
 * --------------------
 * Perform the operation using the big and small values
 * Store the big value into the right value (in case = is called again)
 *
 */
void perform_enter(void) {
    if (has_dot) remove_decimals(big_output);
    big_value           = parse_big_output_to_double();
    right_value.content = big_value;
    memcpy(right_value.str, big_output, sizeof(big_output));
    right_value.validity = 1;
    append_to_small_output(big_output);
    append_to_small_output(get_operator_sign(enter));
    current_index_big   = 0;
    current_index_small = 0;
    has_dot             = false;
    perform_operation(operator, small_value, big_value);
    operator = enter;  // Set operator to enter so we know we just finished a calculation
    return;
}

/*
 * Function:  process_operator
 * --------------------
 * Processes [*+-/]
 *  op: enum of the operator
 *
 */
void process_operator(int op) {
    // after NaN do nothing
    if (current_index_big < 0) {
        return;
    }

    // If the last operation was enter (=), treat this as starting a new calculation
    // with the result as the first operand
    if (operator == enter && !is_op) {
        operator = op;
        if (has_dot) remove_decimals(big_output);
        big_value   = parse_big_output_to_double();
        small_value = big_value;
        strcpy(small_output, big_output);
        current_index_small = get_length(big_output);
        append_to_small_output(get_operator_sign(op));
        right_value.validity = 0;
        current_index_big    = 0;
        has_dot              = false;
        is_op                = true;
        return;
    }

	// if no is no previous operator takes the big value and puts into the small value
	// and append the operator at the end while storing its value
    if (operator == empty) {
        operator = op;
        if (has_dot) remove_decimals(big_output);
        big_value   = parse_big_output_to_double();
        small_value = big_value;
        strcpy(small_output, big_output);
        current_index_small = get_length(big_output);
        append_to_small_output(get_operator_sign(op));
        right_value.validity = 0;
        current_index_big    = 0;
        has_dot              = false;
    } else {
        // if last input was also an operator
        if (is_op) {
            if (op == enter) {
                if (right_value.validity) {
                    memcpy(small_output, reset_small, sizeof(reset_small));
                    current_index_small = 0;
                    append_to_small_output(big_output);
                    append_to_small_output(get_operator_sign(operator));
                    append_to_small_output(right_value.str);
					append_to_small_output("=");
                    perform_operation(operator, big_value, right_value.content);
                    current_index_small = 0;
                } else {
					if (operator != enter){
                        perform_enter();
                    }else{
                        return;
                    }
                }
            } else {
                if (right_value.validity) {
                    right_value.validity = 0;
                    operator             = op;
                    memcpy(small_output, reset_small, sizeof(reset_small));
                    append_to_small_output(big_output);
                    append_to_small_output(get_operator_sign(op));
                } else {
                    // we update the last operator
                    operator = op;
                    // we change the current sign in the small output
                    // [0] to get a char
                    small_output[current_index_small - 1] = get_operator_sign(op)[0];
                }
            }
        } else {
            if (op == enter) {
                perform_enter();
            } else {
                if (has_dot) remove_decimals(big_output);
                big_value = parse_big_output_to_double();
                right_value.validity = 0;
                append_to_small_output(big_output);
                append_to_small_output(get_operator_sign(op));
                current_index_big = 0;
                has_dot           = false;
                operator = op;
            }
        }
    }
    is_op = true;
    return;
}

void set_small_value(void) { small_value = parse_big_output_to_double(); }

/*
 * Function:  process_value
 * --------------------
 * Processes [0-9]
 * If 0 checks if the current value is different than "0"
 * Check if there is still room
 * 	if yes appends the value
 * 	else do nothing
 */
void process_value(char value) {
    // If we just finished a calculation (operator is enter), start completely fresh
    if (operator == enter) {
        oled_clear();  // Clear the display
        current_index_small = 0;
        memcpy(small_output, reset_small, sizeof(reset_small));
        memcpy(big_output, reset_big, sizeof(reset_big));
        current_index_big = 0;
        has_dot = false;
        operator = empty;
        small_value = 0.0;
        big_value = 0.0;
        right_value.validity = 0;
    }

    // If we just pressed an operator (is_op is true), clear only big_output for new number
    // but keep the operation intact
    if (is_op) {
        memcpy(big_output, reset_big, sizeof(reset_big));
        current_index_big = 0;
        has_dot = false;
        is_op = false;
    }

    if (value == '0') {
        if (current_index_big < 10) {
            if (current_index_big <= 0) {
                memcpy(big_output, reset_big, sizeof(reset_big));
                current_index_big = 0;
            }
            big_output[current_index_big] = value;
            current_index_big++;
        }
    } else {
        // if there is still room
        if ((current_index_big < 10) && (current_index_big >= 0)) {
            if ((current_index_big == 1) && (big_output[0] == '0')) {
                big_output[0] = value;
            } else {
                if (current_index_big == 0) {
                    memcpy(big_output, reset_big, sizeof(reset_big));
                }
                big_output[current_index_big] = value;
                current_index_big++;
            }
        } else if (current_index_big < 0) {
            reset();
            big_output[current_index_big] = value;
            current_index_big++;
        }
        // else do nothing
    }
}

/*
 * Function:  process_dot
 * --------------------
 * If there is no dot:
 * 	if at the start of the output add 0.
 * 	else add a dot to the current output
 *
 */
void process_dot(void) {
    // If we just finished a calculation (operator is enter), start completely fresh
    if (operator == enter) {
        oled_clear();  // Clear the display
        current_index_small = 0;
        memcpy(small_output, reset_small, sizeof(reset_small));
        memcpy(big_output, reset_big, sizeof(reset_big));
        current_index_big = 0;
        has_dot = false;
        operator = empty;
        small_value = 0.0;
        big_value = 0.0;
        right_value.validity = 0;
    }

    // If we just pressed an operator (is_op is true), clear only big_output for new number
    // but keep the operation intact
    if (is_op) {
        memcpy(big_output, reset_big, sizeof(reset_big));
        current_index_big = 0;
        has_dot = false;
        is_op = false;
    }

    if (!has_dot) {
        if ((current_index_big < 11) && (current_index_big > 0)) {
            big_output[current_index_big] = '.';
            current_index_big++;
            has_dot = true;
        } else if (current_index_big < 11) {
            big_output[current_index_big] = '0';
            current_index_big++;
            big_output[current_index_big] = '.';
            current_index_big++;
            has_dot = true;
        } else {
            // do nothing
        }
    }
}

void process_record_user_calc(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case LT(2, RENC):
            if (record->event.pressed) {
                reset();
            }
            break;
        case KC_PDOT:
            if (record->event.pressed) {
                process_dot();
            }
            break;
        case KC_P0:
            if (record->event.pressed) {
                process_value('0');
            }
            break;
        case KC_P1:
            if (record->event.pressed) {
                process_value('1');
            }
            break;
        case KC_P2:
            if (record->event.pressed) {
                process_value('2');
            }
            break;
        case KC_P3:
            if (record->event.pressed) {
                process_value('3');
            }
            break;
        case KC_P4:
            if (record->event.pressed) {
                process_value('4');
            }
            break;
        case KC_P5:
            if (record->event.pressed) {
                process_value('5');
            }
            break;
        case KC_P6:
            if (record->event.pressed) {
                process_value('6');
            }
            break;
        case KC_P7:
            if (record->event.pressed) {
                process_value('7');
            }
            break;
        case KC_P8:
            if (record->event.pressed) {
                process_value('8');
            }
            break;
        case KC_P9:
            if (record->event.pressed) {
               process_value('9');
            }
            break;
        case KC_PENT:  // enter
            if (record->event.pressed) {
                process_operator(enter);
             }
            break;
        case KC_PPLS:  // add
            if (record->event.pressed) {
                process_operator(add);
            }
            break;
        case KC_PAST: // multiply
            if (record->event.pressed) {
                process_operator(multiply);
            }
            break;
        case KC_PMNS: // subtract
            if (record->event.pressed) {
                process_operator(subtract);
            }
            break;
        case KC_PSLS: // divide
            if (record->event.pressed) {
                process_operator(divide);
            }
            break;
    }
};
