#pragma once

#include <initguid.h>

#define KEYBOARD_DEVICE 0x8001

#define IOCTL_KEYBOARD_SEND_INPUT CTL_CODE(KEYBOARD_DEVICE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
// Modifier Keys
#define LCTRL	0;
#define LSHIFT	1;
#define LALT	2;
#define LGUI	3;
#define RCTRL	4;
#define RSHIFT	5;
#define RALT	6;
#define RGUI	7;

enum ButtonStates
{
	ButtonStateUp,
	ButtonStateDown,

	max_ButtonStates
};

typedef struct _KEYSTROKE_DATA
{
	UCHAR scan_code;
	enum ButtonStates button_state;
		
} KEYSTROKE_DATA, *PKEYSTROKE_DATA;


// {3bfa5575 - 1890 - 4cc7 - 8fa0 - 2c23a756aeb5} Interface GUID
DEFINE_GUID(GUID_Keyboard, 0x3bfa5575, 0x1890, 0x4cc7, 0x8f, \
	0xa0, 0x2c, 0x23, 0xa7, 0x56, 0xae, 0xb5);
