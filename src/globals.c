#include "pico.h"
#include "pico/time.h"
#include "globals.h"
#include <stdalign.h> //Выравнивание массивов в памяти
#include "zx_emu/zx_machine.h"
char buf[10];			//временный буфер
char header_buf[87];	// буфер для чтения заголовка

char temp_msg[60]; // Буфер для вывода строк
uint8_t save_slots[11];
bool show_slots = false;

uint8_t now_joy1_mode=0;
uint8_t now_joy2_mode=0;
uint8_t now_kbd_mode=0;

bool kbd_lock=false;

bool zx_screen_refresh;
//bool int_en=true;

#ifdef NUM_V_BUF
    uint8_t graph_buf[V_BUF_SZ*NUM_V_BUF];
    bool is_show_frame[NUM_V_BUF];
    int draw_vbuf_inx=0;
    int show_vbuf_inx=0;

#else
    alignas(1024) uint8_t graph_buf[V_BUF_SZ+128];
    uint8_t hud_line[SCREEN_W+10];
#endif


#ifndef DEBUG_DISABLE_LOADERS
    ///__scratch_x("temp_data_x")
     uint8_t temp_buffer_x[TEMP_BUFF_SIZE_X];
    ///__scratch_y("temp_data_y")
     uint8_t temp_buffer_y[TEMP_BUFF_SIZE_Y];
#endif


//uint8_t* temp_buffer_x = &RAM[0];
//uint8_t* temp_buffer_y = &RAM[TEMP_BUFF_SIZE_X];

uint8_t sd_buffer[SD_BUFFER_SIZE];

int	null_printf(const char *str, ...){return 0;};

uint32_t my_millis(){
	return us_to_ms(time_us_32());
}


