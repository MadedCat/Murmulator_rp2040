#pragma once
#include <inttypes.h>
#include <stdio.h>
#include <pico.h>
#include <hardware/pio.h>
#include "kb_u_codes.h"

#ifndef PIN_PS2_CLK
#ifdef PS2KBD_GPIO_FIRST
#define PIN_PS2_CLK (PS2KBD_GPIO_FIRST)
#else
#define PIN_PS2_CLK (0)
#endif
#endif

#ifndef PIN_PS2_DATA
#ifdef PS2KBD_GPIO_FIRST
#define PIN_PS2_DATA (PS2KBD_GPIO_FIRST + 1)
#else
#define PIN_PS2_DATA (1)
#endif
#endif

#define KB_PS_2_CMD_RESET (0xFF)
#define KB_PS_2_CMD_SET_LED (0xED)

#define KB_PS_2_SCROLL_LOCK_ON (1)
#define KB_PS_2_NUM_LOCK_ON    (2)
#define KB_PS_2_CAPS_LOCK_ON  (4)
#define KB_PS_2_KANA_LOCK_ON  (8)


#define PIO_PS2 (pio0)
#define SM_PS2 (0)

#define ps2kbd_wrap_target 0
#define ps2kbd_wrap 6

void  ps2_proc(uint8_t val);
extern uint8_t* zx_keyboard_state;
extern kb_u_state kb_st_ps2;
//static inline uint8_t get_scan_code(void);
bool decode_PS2();
void start_PS2_capture();
static void set_pin(int PIN, bool data);
void set_PS2_data(uint8_t data);

/* _ps2mode status flags */
#define _PS2_BUSY        0x80
#define _TX_MODE         0x40
#define _BREAK_KEY       0x20
#define _WAIT_RESPONSE   0x10
#define _E0_MODE         0x08
#define _E1_MODE         0x04
#define _LAST_VALID      0x02

/* _tx_ready flags */
#define _HANDSHAKE       0x80
#define _COMMAND         0x01

/* Key Repeat defines */
#define _NO_BREAKS       0x08
#define _NO_REPEATS      0x80


/* Command or response */
#define PS2_KC_RESEND   0xFE
#define PS2_KC_ACK      0xFA
#define PS2_KC_ECHO     0xEE
/* Responses */
#define PS2_KC_BAT      0xAA
// Actually buffer overrun
#define PS2_KC_OVERRUN  0xFF
// Below is general error code
#define PS2_KC_ERROR    0xFC
#define PS2_KC_KEYBREAK 0xF0
#define PS2_KC_EXTEND1  0xE1
#define PS2_KC_EXTEND   0xE0
/* Commands */
#define PS2_KC_RESET    0xFF
#define PS2_KC_DEFAULTS 0xF6
#define PS2_KC_DISABLE  0xF5
#define PS2_KC_ENABLE   0xF4
#define PS2_KC_RATE     0xF3
#define PS2_KC_READID   0xF2
#define PS2_KC_SCANCODE 0xF0
#define PS2_KC_LOCK     0xED

//макросы сокращения клавиатурных команд
#define KBD_A				kb_st_ps2.u[0]&KB_U0_A
#define KBD_B				kb_st_ps2.u[0]&KB_U0_B
#define KBD_C				kb_st_ps2.u[0]&KB_U0_C
#define KBD_D				kb_st_ps2.u[0]&KB_U0_D
#define KBD_E				kb_st_ps2.u[0]&KB_U0_E
#define KBD_F				kb_st_ps2.u[0]&KB_U0_F
#define KBD_G				kb_st_ps2.u[0]&KB_U0_G
#define KBD_H				kb_st_ps2.u[0]&KB_U0_H
#define KBD_I				kb_st_ps2.u[0]&KB_U0_I
#define KBD_J				kb_st_ps2.u[0]&KB_U0_J
#define KBD_K				kb_st_ps2.u[0]&KB_U0_K
#define KBD_L				kb_st_ps2.u[0]&KB_U0_L
#define KBD_M				kb_st_ps2.u[0]&KB_U0_M
#define KBD_N				kb_st_ps2.u[0]&KB_U0_N
#define KBD_O				kb_st_ps2.u[0]&KB_U0_O
#define KBD_P				kb_st_ps2.u[0]&KB_U0_P
#define KBD_Q				kb_st_ps2.u[0]&KB_U0_Q
#define KBD_R				kb_st_ps2.u[0]&KB_U0_R
#define KBD_S				kb_st_ps2.u[0]&KB_U0_S
#define KBD_T				kb_st_ps2.u[0]&KB_U0_T
#define KBD_U				kb_st_ps2.u[0]&KB_U0_U
#define KBD_V				kb_st_ps2.u[0]&KB_U0_V
#define KBD_W				kb_st_ps2.u[0]&KB_U0_W
#define KBD_X				kb_st_ps2.u[0]&KB_U0_X
#define KBD_Y				kb_st_ps2.u[0]&KB_U0_Y
#define KBD_Z				kb_st_ps2.u[0]&KB_U0_Z

#define KBD_SEMICOLON		kb_st_ps2.u[0]&KB_U0_SEMICOLON
#define KBD_QUOTE			kb_st_ps2.u[0]&KB_U0_QUOTE
#define KBD_COMMA			kb_st_ps2.u[0]&KB_U0_COMMA
#define KBD_PERIOD			kb_st_ps2.u[0]&KB_U0_PERIOD
#define KBD_LEFT_BR			kb_st_ps2.u[0]&KB_U0_LEFT_BR
#define KBD_RIGHT_BR		kb_st_ps2.u[0]&KB_U0_RIGHT_BR


#define KBD_0				kb_st_ps2.u[1]&KB_U1_0
#define KBD_1				kb_st_ps2.u[1]&KB_U1_1
#define KBD_2				kb_st_ps2.u[1]&KB_U1_2
#define KBD_3				kb_st_ps2.u[1]&KB_U1_3
#define KBD_4				kb_st_ps2.u[1]&KB_U1_4
#define KBD_5				kb_st_ps2.u[1]&KB_U1_5
#define KBD_6				kb_st_ps2.u[1]&KB_U1_6
#define KBD_7				kb_st_ps2.u[1]&KB_U1_7
#define KBD_8				kb_st_ps2.u[1]&KB_U1_8
#define KBD_9				kb_st_ps2.u[1]&KB_U1_9

#define KBD_ENTER			kb_st_ps2.u[1]&KB_U1_ENTER
#define KBD_SLASH			kb_st_ps2.u[1]&KB_U1_SLASH
#define KBD_MINUS			kb_st_ps2.u[1]&KB_U1_MINUS
#define KBD_EQUALS			kb_st_ps2.u[1]&KB_U1_EQUALS
#define KBD_BACKSLASH		kb_st_ps2.u[1]&KB_U1_BACKSLASH
#define KBD_CAPS_LOCK		kb_st_ps2.u[1]&KB_U1_CAPS_LOCK
#define KBD_TAB				kb_st_ps2.u[1]&KB_U1_TAB
#define KBD_BACK_SPACE		kb_st_ps2.u[1]&KB_U1_BACK_SPACE
#define KBD_ESC				kb_st_ps2.u[1]&KB_U1_ESC
#define KBD_TILDE			kb_st_ps2.u[1]&KB_U1_TILDE
#define KBD_MENU			kb_st_ps2.u[1]&KB_U1_MENU
#define KBD_L_SHIFT			kb_st_ps2.u[1]&KB_U1_L_SHIFT
#define KBD_L_CTRL			kb_st_ps2.u[1]&KB_U1_L_CTRL
#define KBD_L_ALT			kb_st_ps2.u[1]&KB_U1_L_ALT	
#define KBD_L_WIN			kb_st_ps2.u[1]&KB_U1_L_WIN	
#define KBD_R_SHIFT			kb_st_ps2.u[1]&KB_U1_R_SHIFT
#define KBD_R_CTRL			kb_st_ps2.u[1]&KB_U1_R_CTRL
#define KBD_R_ALT			kb_st_ps2.u[1]&KB_U1_R_ALT
#define KBD_R_WIN			kb_st_ps2.u[1]&KB_U1_R_WIN
#define KBD_SPACE			kb_st_ps2.u[1]&KB_U1_SPACE


#define KBD_NUM_0			kb_st_ps2.u[2]&KB_U2_NUM_0
#define KBD_NUM_1			kb_st_ps2.u[2]&KB_U2_NUM_1
#define KBD_NUM_2			kb_st_ps2.u[2]&KB_U2_NUM_2
#define KBD_NUM_3			kb_st_ps2.u[2]&KB_U2_NUM_3
#define KBD_NUM_4			kb_st_ps2.u[2]&KB_U2_NUM_4
#define KBD_NUM_5			kb_st_ps2.u[2]&KB_U2_NUM_5
#define KBD_NUM_6			kb_st_ps2.u[2]&KB_U2_NUM_6
#define KBD_NUM_7			kb_st_ps2.u[2]&KB_U2_NUM_7
#define KBD_NUM_8			kb_st_ps2.u[2]&KB_U2_NUM_8
#define KBD_NUM_9			kb_st_ps2.u[2]&KB_U2_NUM_9
#define KBD_NUM_ENTER		kb_st_ps2.u[2]&KB_U2_NUM_ENTER
#define KBD_NUM_SLASH		kb_st_ps2.u[2]&KB_U2_NUM_SLASH
#define KBD_NUM_MINUS		kb_st_ps2.u[2]&KB_U2_NUM_MINUS
#define KBD_NUM_PLUS		kb_st_ps2.u[2]&KB_U2_NUM_PLUS
#define KBD_NUM_MULT		kb_st_ps2.u[2]&KB_U2_NUM_MULT
#define KBD_NUM_PERIOD		kb_st_ps2.u[2]&KB_U2_NUM_PERIOD
#define KBD_NUM_LOCK		kb_st_ps2.u[2]&KB_U2_NUM_LOCK

#define KBD_DELETE			kb_st_ps2.u[2]&KB_U2_DELETE
#define KBD_SCROLL_LOCK		kb_st_ps2.u[2]&KB_U2_SCROLL_LOCK
#define KBD_PAUSE_BREAK		kb_st_ps2.u[2]&KB_U2_PAUSE_BREAK
#define KBD_INSERT			kb_st_ps2.u[2]&KB_U2_INSERT
#define KBD_HOME			kb_st_ps2.u[2]&KB_U2_HOME
#define KBD_PAGE_UP			kb_st_ps2.u[2]&KB_U2_PAGE_UP
#define KBD_PAGE_DOWN		kb_st_ps2.u[2]&KB_U2_PAGE_DOWN
#define KBD_PRT_SCR			kb_st_ps2.u[2]&KB_U2_PRT_SCR
#define KBD_END				kb_st_ps2.u[2]&KB_U2_END
#define KBD_UP				kb_st_ps2.u[2]&KB_U2_UP
#define KBD_DOWN			kb_st_ps2.u[2]&KB_U2_DOWN
#define KBD_LEFT			kb_st_ps2.u[2]&KB_U2_LEFT
#define KBD_RIGHT			kb_st_ps2.u[2]&KB_U2_RIGHT

#define KBD_F1				kb_st_ps2.u[3]&KB_U3_F1
#define KBD_F2				kb_st_ps2.u[3]&KB_U3_F2
#define KBD_F3				kb_st_ps2.u[3]&KB_U3_F3
#define KBD_F4				kb_st_ps2.u[3]&KB_U3_F4
#define KBD_F5				kb_st_ps2.u[3]&KB_U3_F5
#define KBD_F6				kb_st_ps2.u[3]&KB_U3_F6
#define KBD_F7				kb_st_ps2.u[3]&KB_U3_F7
#define KBD_F8				kb_st_ps2.u[3]&KB_U3_F8
#define KBD_F9				kb_st_ps2.u[3]&KB_U3_F9
#define KBD_F10				kb_st_ps2.u[3]&KB_U3_F10
#define KBD_F11				kb_st_ps2.u[3]&KB_U3_F11
#define KBD_F12				kb_st_ps2.u[3]&KB_U3_F12

#define KBD_PRESS_STATE		(kb_st_ps2.state&0x80)
#define KBD_RELEASE_STATE	(kb_st_ps2.state&0x08)

#define KBD_PRESS			(kb_st_ps2.u[0]!=0)||(kb_st_ps2.u[1]!=0)||(kb_st_ps2.u[2]!=0)||(kb_st_ps2.u[3]!=0)
#define KBD_RELEASE			(kb_st_ps2.u[0]==0)&&(kb_st_ps2.u[1]==0)&&(kb_st_ps2.u[2]==0)&&(kb_st_ps2.u[3]==0)