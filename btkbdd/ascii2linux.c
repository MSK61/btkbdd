/*
 * ASCII-to-Linux conversion
 * Mohammed El-Afifi <Mohammed_ElAfifi@yahoo.com>
 * License: GPL
 */

#include <linux/input.h>

#include "ascii2linux.h"

#define MAX_ASCII_CHAR 127

enum shift_status {
	no_shift,
	left_shift,
	right_shift
};

/* Key combination */
struct key_comb {
	unsigned char key;
	enum shift_status shift;
};

static struct key_comb linux_codes[MAX_ASCII_CHAR + 1] = {
	[0 ... MAX_ASCII_CHAR] = {KEY_RESERVED},
	['\n'] = {KEY_ENTER, no_shift},
	[27] = {KEY_BACKSPACE, no_shift},
	[' '] = {KEY_SPACE, no_shift},
	['!'] = {KEY_1, right_shift},
	['"'] = {KEY_APOSTROPHE, left_shift},
	['#'] = {KEY_3, right_shift},
	['$'] = {KEY_4, right_shift},
	['%'] = {KEY_5, right_shift},
	['&'] = {KEY_7, left_shift},
	['\''] = {KEY_APOSTROPHE, no_shift},
	['('] = {KEY_9, left_shift},
	[')'] = {KEY_0, left_shift},
	['*'] = {KEY_8, left_shift},
	['+'] = {KEY_EQUAL, left_shift},
	[','] = {KEY_COMMA, no_shift},
	['-'] = {KEY_MINUS, no_shift},
	['.'] = {KEY_DOT, no_shift},
	['/'] = {KEY_SLASH, no_shift},
	['0'] = {KEY_0, no_shift},
	['1'] = {KEY_1, no_shift},
	['2'] = {KEY_2, no_shift},
	['3'] = {KEY_3, no_shift},
	['4'] = {KEY_4, no_shift},
	['5'] = {KEY_5, no_shift},
	['6'] = {KEY_6, no_shift},
	['7'] = {KEY_7, no_shift},
	['8'] = {KEY_8, no_shift},
	['9'] = {KEY_9, no_shift},
	[':'] = {KEY_SEMICOLON, left_shift},
	[';'] = {KEY_SEMICOLON, no_shift},
	['<'] = {KEY_COMMA, left_shift},
	['='] = {KEY_EQUAL, no_shift},
	['>'] = {KEY_DOT, left_shift},
	['?'] = {KEY_SLASH, left_shift},
	['@'] = {KEY_2, right_shift},
	['A'] = {KEY_A, right_shift},
	['B'] = {KEY_B, right_shift},
	['C'] = {KEY_C, right_shift},
	['D'] = {KEY_D, right_shift},
	['E'] = {KEY_E, right_shift},
	['F'] = {KEY_F, right_shift},
	['G'] = {KEY_G, right_shift},
	['H'] = {KEY_H, left_shift},
	['I'] = {KEY_I, left_shift},
	['J'] = {KEY_J, left_shift},
	['K'] = {KEY_K, left_shift},
	['L'] = {KEY_L, left_shift},
	['M'] = {KEY_M, left_shift},
	['N'] = {KEY_N, left_shift},
	['O'] = {KEY_O, left_shift},
	['P'] = {KEY_P, left_shift},
	['Q'] = {KEY_Q, right_shift},
	['R'] = {KEY_R, right_shift},
	['S'] = {KEY_S, right_shift},
	['T'] = {KEY_T, right_shift},
	['U'] = {KEY_U, left_shift},
	['V'] = {KEY_V, right_shift},
	['W'] = {KEY_W, right_shift},
	['X'] = {KEY_X, right_shift},
	['Y'] = {KEY_Y, left_shift},
	['Z'] = {KEY_Z, right_shift},
	['['] = {KEY_LEFTBRACE, no_shift},
	['\\'] = {KEY_BACKSLASH, no_shift},
	[']'] = {KEY_RIGHTBRACE, no_shift},
	['^'] = {KEY_6, left_shift},
	['_'] = {KEY_MINUS, left_shift},
	['`'] = {KEY_GRAVE, no_shift},
	['a'] = {KEY_A, no_shift},
	['b'] = {KEY_B, no_shift},
	['c'] = {KEY_C, no_shift},
	['d'] = {KEY_D, no_shift},
	['e'] = {KEY_E, no_shift},
	['f'] = {KEY_F, no_shift},
	['g'] = {KEY_G, no_shift},
	['h'] = {KEY_H, no_shift},
	['i'] = {KEY_I, no_shift},
	['j'] = {KEY_J, no_shift},
	['k'] = {KEY_K, no_shift},
	['l'] = {KEY_L, no_shift},
	['m'] = {KEY_M, no_shift},
	['n'] = {KEY_N, no_shift},
	['o'] = {KEY_O, no_shift},
	['p'] = {KEY_P, no_shift},
	['q'] = {KEY_Q, no_shift},
	['r'] = {KEY_R, no_shift},
	['s'] = {KEY_S, no_shift},
	['t'] = {KEY_T, no_shift},
	['u'] = {KEY_U, no_shift},
	['v'] = {KEY_V, no_shift},
	['w'] = {KEY_W, no_shift},
	['x'] = {KEY_X, no_shift},
	['y'] = {KEY_Y, no_shift},
	['z'] = {KEY_Z, no_shift},
	['{'] = {KEY_LEFTBRACE, left_shift},
	['|'] = {KEY_BACKSLASH, left_shift},
	['}'] = {KEY_RIGHTBRACE, left_shift},
	['~'] = {KEY_GRAVE, left_shift}
};

static void
fill_event (event, key, value)
	struct input_event *event;
	unsigned char key;
	int value;
{

	event->type = EV_KEY;
	event->code = key;
	event->value = value;
}

static int
fill_key_presses (keys, num_of_keys, events, max_events, ev_idx)
	unsigned char keys[];
	int num_of_keys;
	struct input_event events[];
	int max_events;
	int *ev_idx;
{
	int key_idx;

	for (key_idx = 0; key_idx < num_of_keys && *ev_idx < max_events;
	     key_idx++, (*ev_idx)++)
		fill_event (events + *ev_idx, keys[key_idx], 1);

	/* Press events for all keys could be generated. */
	return key_idx == num_of_keys;
}

static int
fill_key_releases (keys, num_of_keys, events, max_events, ev_idx)
	unsigned char keys[];
	int num_of_keys;
	struct input_event events[];
	int max_events;
	int *ev_idx;
{
	int key_idx;

	for (key_idx = num_of_keys - 1; key_idx >= 0 && *ev_idx < max_events;
	     key_idx--, (*ev_idx)++)
		fill_event (events + *ev_idx, keys[key_idx], 0);

	/* Release events for all keys could be generated. */
	return key_idx < 0;
}

static int
gen_keys (comb, keys, max_keys)
	struct key_comb *comb;
	unsigned char keys[];
	int max_keys;
{
	int num_of_keys = 0;

	switch (comb->shift) {
	case left_shift: keys[num_of_keys] = KEY_LEFTSHIFT; break;
	case right_shift: keys[num_of_keys] = KEY_RIGHTSHIFT; break;
	default: break;
	}

	if (comb->shift != no_shift)
		num_of_keys++;

	keys[num_of_keys++] = comb->key;
	return num_of_keys;
}

static int
translatable (input)
	char input;
{

	return input >= 0 && input <= MAX_ASCII_CHAR &&
	       linux_codes[(unsigned char)input].key != KEY_RESERVED;
}

/* Process an ASCII char */
int
ascii_char (events, max_events, input)
	struct input_event *events;
	int max_events;
	char input;
{
	int ev_idx = 0;
	unsigned char keys[MAX_KEYS_PER_ASCII_CHAR];
	int num_of_keys;

	if (!translatable (input))
		return -1;

	num_of_keys =
	    gen_keys (linux_codes + input, keys, MAX_KEYS_PER_ASCII_CHAR);
	return (fill_key_presses (keys, num_of_keys, events, max_events,
				  &ev_idx) &&
		fill_key_releases (keys, num_of_keys, events, max_events,
				   &ev_idx))
		   ? ev_idx
		   : -1;
}
