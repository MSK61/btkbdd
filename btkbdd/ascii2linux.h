/*
 * Convert an ASCII char to a Linux event device key. This is derived from the
 * table in linux2hid.h.
 *
 * Mohammed El-Afifi <Mohammed_ElAfifi@yahoo.com>
 * License: GPL
 */

#ifndef __ASCII2LINUX_H
#define __ASCII2LINUX_H

#define MAX_KEYS_PER_ASCII_CHAR 2
#define MAX_EVENTS_PER_ASCII_CHAR MAX_KEYS_PER_ASCII_CHAR * 2

struct input_event;

int ascii_char (struct input_event *, int, char);

#endif
