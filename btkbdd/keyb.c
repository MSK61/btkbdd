/*
 * Input and HID logic
 * Lubomir Rintel <lkundrak@v3.sk>
 * License: GPL
 */

#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "ascii2linux.h"
#include "btkbdd.h"
#include "hid.h"
#include "linux2hid.h"

/* A packet we're sending to host after a keypress */
struct key_report {
	uint8_t type;
	uint8_t report;
	uint8_t mods;
	uint8_t reserved;
	uint8_t key[6];
} __attribute__((packed));

/* Keyboard status (keys pressed and LEDs lit) */
struct status {
	struct key_report report;
};

/* Process an evdev event */
static int
input_event (status, event, ctrl, intr)
	struct status *status;
	struct input_event *event;
	int ctrl;
	int intr;
{
	int mod = 0;

	if (event->type != EV_KEY)
		return 0;

	/* We're just a poor 101-key keyboard. */
	if (event->code >= 256) {
		DBG("Ignored code 0x%x > 0xff.\n", event->code);
		return 0;
	}

	/* Apply modifiers. "Left/RightGUI is Windows / Command / Meta" */
	switch (event->code) {
	case KEY_LEFTCTRL: mod = HIDP_LEFTCTRL; break;
	case KEY_LEFTSHIFT: mod = HIDP_LEFTSHIFT; break;
	case KEY_LEFTALT: mod = HIDP_LEFTALT; break;
	case KEY_LEFTMETA: mod = HIDP_LEFTGUI; break;
	case KEY_RIGHTCTRL: mod = HIDP_RIGHTCTRL; break;
	case KEY_RIGHTSHIFT: mod = HIDP_RIGHTSHIFT; break;
	case KEY_RIGHTALT: mod = HIDP_RIGHTALT; break;
	case KEY_RIGHTMETA: mod = HIDP_RIGHTGUI; break;
	}

	if (mod) {
		/* If a modifier was (de)pressed, update the track... */
		if (event->value) {
			status->report.mods |= mod;
		} else {
			status->report.mods &= ~mod;
		}
	} else {
		/* ...otherwise update the array of keys pressed. */
		int i;
		int code = linux2hid[event->code];

		DBG("code %d value %d hid %d mods 0x%x\n",
			event->code, event->value, linux2hid[event->code], status->report.mods);

		for (i = 0; i < 6; i++) {
			/* Remove key if already enabled */
			if (status->report.key[i] == code)
				status->report.key[i] = 0;

			/* Add key if it was pushed */
			if (event->value && status->report.key[i] == 0) {
				status->report.key[i] = code;
				break;
			}

			/* Shift the rest. Probably not needed, but keyboards do that */
			if (i < 5 && status->report.key[i] == 0) {
				status->report.key[i] = status->report.key[i+1];
				status->report.key[i+1] = 0;
			}
		}
	}

#ifdef DEBUG
	int i;
	uint8_t *buf = (uint8_t *)&status->report;
	for (i = 0; i < sizeof(status->report); i++)
		fprintf (stderr, "%02x ", buf[i]);
	fprintf (stderr, "\n");
#endif

	return 1;
}

/* Handshake with Apple crap */
static int
hello (control)
	int control;
{
	/* Apple disconnects immediately,
	 * if we don't send this within a second. */
	if (write (control, "\xa1\x13\x03", 3) != 3) {
		perror ("Could not send a handshake.");
		return -1;
	}
	if (write (control, "\xa1\x13\x02", 3) != 3) {
		perror ("Could not send a handshake.");
		return -1;
	}
	/* Apple is known to require a small delay,
	 * otherwise it eats the first character. */
	sleep (1);

	return 0;
}

/* Dispatch the work */
static void
session (src, tgt, input)
	bdaddr_t src;
	bdaddr_t *tgt;
	char *input;
{
	char activation[11] = {0};	/* arbitrary activation sequence */
	int control = -1, intr = -1;	/* host sockets */
	struct status status;		/* keyboard state */
	int token_idx;
	const char *tokens[] = {activation, input, "\n"};
	int num_of_tokens = sizeof(tokens) / sizeof(tokens[0]);

	/* Initialize the keyboard state */
	status.report.type = HIDP_TRANS_DATA | HIDP_DATA_RTYPE_INPUT;
	status.report.report = 0x01;
	status.report.mods = 0;
	status.report.reserved = 0;
	status.report.key[0] = status.report.key[1] = status.report.key[2]
		= status.report.key[3] = status.report.key[4]
		= status.report.key[5] = 0;
	/* The first few keystrokes tend to get lost at the receiving end. A few
	 * dummy keystrokes at the beginning will just skip this flaky phase and
	 * get the receiver ready for handling the real payload. This's just an
	 * arbitrary sequence(a few backspaces); any sequence would actually
	 * work. */
	memset (activation, 27, sizeof(activation) - sizeof(activation[0]));

	for (token_idx = 0; token_idx < num_of_tokens; token_idx++) {
		const char *in_char = NULL;

		for (in_char = tokens[token_idx]; *in_char; in_char++) {
			DBG("Entered main loop.\n");
			struct input_event events[MAX_EVENTS_PER_ASCII_CHAR];
			int i;
			int num_of_events = ascii_char (
			    events, MAX_EVENTS_PER_ASCII_CHAR, *in_char);

			for (i = 0; i < num_of_events; i++) {
				/* Read the keyboard event and update status */
				int ret = input_event (
				    &status, events + i, control, intr);
				if (ret == 0)
					continue;
				DBG("Input event.\n");

				/* Noone managed to connect to us so far.
				 * Try to reach out for a host ourselves. */
				if (control == -1) {
					/* Noone to talk to? */
					if (!bacmp (tgt, BDADDR_ANY))
						break;

					control = l2cap_connect (&src, tgt, L2CAP_PSM_HIDP_CTRL);
					if (control == -1)
						break;
					intr = l2cap_connect (&src, tgt, L2CAP_PSM_HIDP_INTR);
					if (intr == -1) {
						close (control);
						control = -1;
						break;
					}
					if (hello (control) == -1)
						break;
				}

				/* Simulate a 200 ms delay. */
				if (token_idx > 0 || in_char > tokens[token_idx] || i > 0)
					usleep (200000);

				/* Send the packet to the host. */
				if (write (intr, &status.report, sizeof(status.report)) <= 0) {
					perror ("Could not send a packet to the host");
					break;
				}

			}
		}

	}

	if (control != -1)
		close (control);
	if (intr != -1)
		close (intr);
}

int
loop (input, src, tgt)
	char *input;
	bdaddr_t src;
	bdaddr_t *tgt;
{
	int hci = -1;

	if (hci == -1) {
		if (bacmp (&src, BDADDR_ANY)) {
			char addr[18];

			ba2str (&src, addr);
			hci = hci_devid (addr);
			/* Not yet plugged in or visited by udev? */
			if (hci == -1)
				perror ("Can not initialize HCI device");
		} else {
			/* No source device specified. Assume first. */
			hci = 0;
		}
		if (hci >= 0) {
			if (sdp_open () == 1)
				sdp_add_keyboard ();
		}
	}

	session (src, tgt, input);
	sdp_remove ();
	return 1;
}
