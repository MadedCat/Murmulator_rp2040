#pragma once
#include "stdio.h"
#include "screen_util.h"
#include <config.h>

#define TRDOS_COMPILE

#define FW_VERSION "v"SOFT_VERSION
#define FW_AUTHOR "TecnoCat"

#define SCREEN_H (240)
#define SCREEN_W (320)
#define V_BUF_SZ (SCREEN_H*SCREEN_W/2)

#define ZX_RAM_PAGE_SIZE 0x4000
#define ZX_RAM_PAGES 8
#define DIRS_DEPTH (10)
#define MAX_FILES  (400)
#define SD_BUFFER_SIZE 0x4000  //Размер буфера для работы с файлами
#define FILE_NAME_LEN (13) //name=8+ext=3+dot=1+ftype=1

#define BOOT_LOGO		0x00
#define MENU_MAIN		0x01
#define MENU_JOY_MAIN	0x02
#define MENU_HELP		0x03
#define MENU_KEYBOARD	0x04
#define MENU_SETTINGS	0x05
#define MENU_SELDRIVE	0x06
#define MENU_POKE		0x07
#define MENU_POKE_FILE	0x08
#define EMULATION		0xFF

#define MAX_JOY_MODE 5
#define MAX_KBD_MODE 5

extern char buf[10];			//временный буфер
extern char header_buf[87];	// буфер для чтения заголовка

extern char temp_msg[60]; // Буфер для вывода строк
extern uint8_t save_slots[11];
extern bool show_slots;

extern uint8_t now_joy1_mode;
extern uint8_t now_joy2_mode;
extern uint8_t now_kbd_mode;

extern bool kbd_lock;



#define TEMP_BUFF_SIZE_X 0x0400
#define TEMP_BUFF_SIZE_Y 0x0A00

//#define NUM_V_BUF (3)
#ifdef NUM_V_BUF
	extern  bool is_show_frame[NUM_V_BUF];
	extern int draw_vbuf_inx;
	extern int show_vbuf_inx;
#endif
extern bool zx_screen_refresh;
//extern bool int_en;
extern uint8_t graph_buf[];
extern uint8_t hud_line[];
extern uint8_t color_zx[16];

extern uint8_t temp_buffer_x[TEMP_BUFF_SIZE_X];
extern uint8_t temp_buffer_y[TEMP_BUFF_SIZE_Y];

extern uint8_t sd_buffer[SD_BUFFER_SIZE];

int	null_printf(const char *str, ...);

/*
#define G_PRINTF  printf
#define G_PRINTF_INFO  printf
#define G_PRINTF_DEBUG  printf
#define G_PRINTF_ERROR  printf
*/

#define FAST_FUNC __not_in_flash_func

uint32_t my_millis();
