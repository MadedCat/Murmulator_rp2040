#include <pico.h>

// #define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)

//#define DEBUG_DELAY

//#define DEBUG_BLINK

//#define DEBUG_DISABLE_LOADERS

//#define AY_SAMPLE_BASE 220500

//#define AY_SAMPLE_MUL  10
//#define AY_SAMPLE_RATE 22050

//#define AY_SAMPLE_MUL  7
//#define AY_SAMPLE_RATE 30310




#define SAMPLE_MUL  7
#define SAMPLE_RATE 30310


#define AY_SAMPLE_MUL  (SAMPLE_MUL)
#define AY_SAMPLE_RATE (SAMPLE_RATE)

//ayhz:43470 aymul:5 - upper
//ayhz:44100 aymul:5 - lower

//ayhz:35720 aymul:6 - upper 
//ayhz:35800 aymul:6 - lower

//ayhz:31250 aymul:7 - upper * //1000 герц с минимальным джиттером
//ayhz:30310 aymul:7 - lower *

//ayhz:27770 aymul:8 - upper
//ayhz:27030 aymul:8 - lower

//ayhz:24390 aymul:9 - upper
//ayhz:23810 aymul:9 - lower

//ayhz:22220 aymul:10 - upper
//ayhz:21740 aymul:10 - lower

#define TIMER_PERIOD 500 //ms

#ifndef PIN_ZX_LOAD
#define PIN_ZX_LOAD (22)
#endif

#ifndef ZX_AY_PWM_PIN0
#define ZX_AY_PWM_PIN0 (26)
#endif
#ifndef ZX_AY_PWM_PIN1
#define ZX_AY_PWM_PIN1 (27)
#endif
#ifndef ZX_BEEP_PIN
#define ZX_BEEP_PIN (28)
#endif

#ifndef WORK_LED_PIN
#define WORK_LED_PIN (25)
#endif

//#define TST_PIN (29)

#define AY_MODE
#define SHOW_SCREEN_DELAY 1000 //в милисекундах

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
(byte & 0x80 ? '1' : '0'), \
(byte & 0x40 ? '1' : '0'), \
(byte & 0x20 ? '1' : '0'), \
(byte & 0x10 ? '1' : '0'), \
(byte & 0x08 ? '1' : '0'), \
(byte & 0x04 ? '1' : '0'), \
(byte & 0x02 ? '1' : '0'), \
(byte & 0x01 ? '1' : '0') 


#include <pico.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/bootrom.h>
#include <pico/rand.h>
#include <pico/stdio_usb.h>
#include <hardware/sync.h>
#include <hardware/irq.h>
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "hardware/structs/systick.h"
#include "hardware/pwm.h"
#include "hardware/vreg.h"
#include <math.h>
#include <string.h>

//#include <pico/flash.h>


#include "globals.h"
#include "../lib/joysticks/Joystics.h"

#ifdef VGA_HDMI
	#include "../lib/video/video.h"
#endif
#ifdef COMPOSITE_TV
	#include "../lib/tv_out/tv_out.h"
#endif
#ifdef SOFT_COMPOSITE_TV
	#include "../lib/tv_out_soft/tv_out.h"
#endif

extern bool graphics_begin_screen;

#include "screen_util.h"
#include "iface.h"
#include "util_sd.h"
#ifndef DEBUG_DISABLE_LOADERS
#include "util_z80.h"
#include "util_sna.h"
#include "util_tap.h"
#include "util_pok.h"
#include "util_scr.h"
#endif

#include "wd1793.h"
#include "util_i2c_kbd.h" // i2c keyboard
#include "../lib/sound/i2s.h"

extern void i2s_out(int16_t l_out,int16_t r_out);

#include "hud_func.h"


// #include "util_Wii_Joy.h"
// extern bool WII_Init;
// extern struct WIIController Wii_joy;
// extern uint8_t WII_Nes;
//extern uint8_t WII_Nes_Old;

#include "small_logo.h"
#include "mur_logo.h"
//#include "mur_logo5.h"
#include "mur_logo6.h"
#include "anim_stripes.h"
#include "anim_screens.h"
#include "anim_eyes.h"
#include "animation.h"
#include "utf_handle.h"
#include "help.h"
#include "fast_menu.h"
#include "ps2.h"
#include "zx_emu/zx_machine.h"
#include "zx_emu/aySoundSoft.h"
#include "zx_emu/PinSerialData_595.h"
#include "util_cfg.h"
#include "util_cfg_menu.h"
#include "util_power.h"

//bool hw_zx_get_bit_LOAD();
//void hw_zx_set_beep_out(uint8_t val);

extern uint8_t cfg_boot_scr;
extern uint8_t cfg_hud_enable;
extern uint8_t cfg_tap_load_mode;
extern uint8_t cfg_tape_load_pin;
extern uint8_t cfg_def_joy1_mode;
extern uint8_t cfg_def_joy2_mode;
extern uint8_t cfg_def_kbd_mode;
extern uint8_t cfg_res_before_mode;
extern uint8_t cfg_sound_out_mode;
extern uint8_t cfg_sound_mode;
extern short int cfg_volume;
extern uint8_t cfg_tspin_mode;
extern uint8_t cfg_tsspeed_mode;
extern uint8_t cfg_tschip_order;
extern uint8_t cfg_video_out;
extern uint8_t cfg_frame_rate;
extern uint8_t cfg_lcd_video_out;
extern uint8_t cfg_rotate;
extern uint8_t cfg_inversion;
extern uint8_t cfg_pixels;
extern uint8_t cfg_brightness;
extern uint8_t cfg_mobile_mode;



/*Forward declarations*/
void process_input(void);
void clear_input(void);
/*Forward declarations*/

extern bool im_z80_stop;
extern bool im_ready_loading;
extern bool covox_mode;				// режим записи в ЦАП covox железного турбосаунда
extern ZX_Input_t* zx_write_buffer;
extern ZX_Input_t* zx_read_buffer;


bool flip_led = false;

uint32_t last_action = 0;
uint32_t disk_action = 0;
uint32_t tape_action = 0;
bool scroll_lfn = false;
uint32_t scroll_action = 0;
uint16_t scroll_pos =0;


extern uint32_t free_time_ticks;
uint32_t freetime_action = 0;
#define FREE_TIME_DELAY 500 //в милисекундах

bool init_fs=false;
short int settings_index=0;
short int settings_lines=0;
short int menu_inc_dec=0;

bool is_pause_mode=false;
bool i2cKbdMode=false;
bool keyPressed;
uint8_t i2cBtnPressed = 0;

bool read_dir = true;
short int shift_file_index=0;
short int cur_dir_index=0;
short int cur_file_index=0;
short int cursor_index_old[DIRS_DEPTH];
short int display_file_index=0;
short int N_files=0;
short int sel_files=0;
short int del_files=0;
short int err_files=0;
char current_lfn[200];
//uint64_t current_time = 0;
char icon[2];
uint8_t sound_reg_pause[36];
	
short int lineStart=0;	
short int fast_menu_index=0;
short int old_menu_index=0;


uint8_t current_settings=0;
uint8_t current_drive=0;
char save_file_name_image[25];

short int fr =-1;

bool need_reset_after_menu=false;

uint8_t fast_mode[5]={0,0,0,0,0};
uint8_t fast_mode_ptr=0;

uint8_t menu_mode[5]={0,0,0,0,0};
uint8_t menu_ptr=0;

uint8_t current_video_out=0;
uint8_t current_frame_rate=0;
uint8_t current_rotate=0;
uint8_t current_inversion=0;
uint8_t current_pixels=0;
uint8_t current_pin=0;
uint8_t current_sound_out=0;
uint8_t current_mobile_mode=0;


#define TAPE_STATE_NULL			0x00
#define TAPE_STATE_STOP			0x01
#define TAPE_STATE_START 		0x02
#define TAPE_STATE_REWIND		0x03
#define TAPE_STATE_PREV_BLOCK	0x04
#define TAPE_STATE_NEXT_BLOCK	0x05
#define TAPE_STATE_EJECT		0x06

uint8_t tape_load_pin=0;
uint8_t tape_cmd=0;
extern uint8_t TapeStatus;

//функция ввода загрузки спектрума
extern uint32_t tapeFileSize;
extern uint32_t tapeTotByteCount;


short int tap_block_percent = 0;


#define NUM_SHOW_FILES 27

bool is_new_screen=false;
bool need_redraw=false;

bool all_FD_empty=true;
bool show_screen=true;

/*ZX_Input_t* zx_read_buffer;
ZX_Input_t* zx_write_buffer;
ZX_Input_t zx_input[2];*/



/*
bool show_battery_ind=false;
bool show_vol_ind=false;
bool show_bri_ind=false;
bool show_kbl_ind=false;
*/



extern bool ack_input;
uint8_t old_volume = 0;
bool lock_display_off=false;
//char vol_ind[12];

#ifndef DEBUG_DISABLE_LOADERS
extern POKE_LINE* pokes_buff;
short int poke_count=0;
short int btn_pos=0;
#endif

/*battery monitor*/
#define BATT_TEXT_LINE_LEN 5
char batt_text[BATT_TEXT_LINE_LEN+2];
extern uint8_t battery_power_percent;
extern bool battery_power_charge;
/*battery monitor*/

/*
extern uint8_t* zx_cpu_ram[4];

uint8_t read_zx_mem(uint16_t addr){
	if (addr<16384) return zx_cpu_ram[0][addr];
	if (addr<32768) return zx_cpu_ram[1][addr-16384];
	if (addr<49152) return zx_cpu_ram[2][addr-32768];
	return zx_cpu_ram[3][addr-49152];
}

void write_zx_mem(uint16_t addr, uint8_t val){
	if (addr<16384) return;//запрещаем писать в ПЗУ
	if (addr<32768) {zx_cpu_ram[1][addr-16384]=val;return;};
	if (addr<49152) {zx_cpu_ram[2][addr-32768]=val;return;};
	zx_cpu_ram[3][addr-49152]=val;
}
*/

void software_reset(){
	watchdog_enable(1, 1);
	while(1);
}

void ZXThread(){

	zx_machine_init();
	zx_machine_main_loop_start();
	////printf("END spectrum emulation\n");
	//return NULL;
}

void inInit(uint gpio){
	gpio_init(gpio);
	gpio_set_dir(gpio,GPIO_IN);
	gpio_pull_up(gpio);
}


bool FAST_FUNC(zx_flash_callback)(repeating_timer_t *rt) {
	zx_machine_flashATTR();
	return true;
};


//Joy joy1 = {2, 5, 4, 0, 0, 0};

#define D_JOY_HAT_UNLOCK	(D_JOY_SELECT|D_JOY_START)

#define HAT_UP		(1<<0)
#define HAT_DOWN	(1<<1)
#define HAT_LEFT	(1<<2)
#define HAT_RIGHT	(1<<3)
#define HAT_A		(1<<4)
#define HAT_B		(1<<5)
#define HAT_SELECT	(1<<6)
#define HAT_START	(1<<7)

volatile uint8_t hat_switch=0;
volatile uint8_t hat_locked=0;


/*
const char __in_flash() *joy_text[MAX_JOY_MODE]={
	"ExtJoy>Kempst",
	"ExtJoy>Cursor",
	"ExtJoy>Sincl1",
	"ExtJoy>Sincl2",
	"ExtJoy>QAOPM ",
	" *KBD>Kempstn",
	" *KBD>Cursor ",
	" *KBD>Sincl 1",
	" *KBD>Sincl 2",
	" *KBD>QAOPM  "
};
*/


//#define D_JOY_DATA_PIN  (16)
//#define D_JOY_CLK_PIN   (14)
//#define D_JOY_LATCH_PIN (15)

#define data_joy_2 (data_joy>>16)

bool joy_pressed 	= false;
bool joy_connected	= false;

uint8_t data_joy1=0;							//данные первого джойстика
uint8_t data_joy2=0;							// данные второго джойстика
uint8_t data_ext_joy1=0;						// добавочные кнопки первого джойстика
uint8_t data_ext_joy2=0;						// добавочные кнопки второго джойстика
uint8_t active_type_joystick = NES_joy1_2;				//  тип активного подключенного джойстика

uint32_t data_joy=0;
uint32_t old_data_joy=0;
uint32_t rel_data_joy=0;

// uint8_t d_joy_get_data(){
// 	uint8_t data=0;
// 	gpio_put(D_JOY_LATCH_PIN,1);
// 	//gpio_put(D_JOY_CLK_PIN,1);
// 	busy_wait_us(48);//12//24
	
// 	gpio_put(D_JOY_LATCH_PIN,0);
// 	busy_wait_us(24);//6//12
// 	for(int i=0;i<8;i++){   
// 		gpio_put(D_JOY_CLK_PIN,0);  
// 		busy_wait_us(40);//10//20
// 		data<<=1;
// 		data|=gpio_get(D_JOY_DATA_PIN);
// 		busy_wait_us(40);//10//20
// 		gpio_put(D_JOY_CLK_PIN,1); 
// 		busy_wait_us(40);//10//20
// 		tight_loop_contents();
// 	}
// 	data=(data&0x0f)|((data>>2)&0x30)|((data<<3)&0x80)|((data<<1)&0x40);
// 	return data;
// };

// bool decode_joy(){
// 	if(joy_connected){
// 		data_joy=~d_joy_get_data();
// 		if (data_joy!=old_data_joy){
// 			if(data_joy>0){
// 				//printf("joy_pressed [%d][%d][%d]\n",data_joy,old_data_joy,rel_data_joy);
// 				joy_pressed=true;
// 				old_data_joy=data_joy;
// 				rel_data_joy=data_joy;
// 			} else {
// 				//printf("!joy_pressed [%d][%d][%d]\n",data_joy,old_data_joy,rel_data_joy);
// 				rel_data_joy=old_data_joy;
// 				old_data_joy=0;
// 				joy_pressed=false;
// 			}
// 			return joy_pressed;
// 		}
// 		return joy_pressed;
// 	}
// 	return false;
// }

// void d_joy_init(){
// 	gpio_init(D_JOY_CLK_PIN);
// 	gpio_set_dir(D_JOY_CLK_PIN,GPIO_OUT);
// 	gpio_init(D_JOY_LATCH_PIN);
// 	gpio_set_dir(D_JOY_LATCH_PIN,GPIO_OUT);
	
// 	gpio_init(D_JOY_DATA_PIN);
// 	gpio_set_dir(D_JOY_DATA_PIN,GPIO_IN);
// 	//gpio_pull_up(D_JOY_DATA_PIN);
// 	gpio_pull_down(D_JOY_DATA_PIN);
// 	gpio_put(D_JOY_LATCH_PIN,0);	
// }

/*process input*/
void reset_kmouse(){
	zx_write_buffer->kempston=0;
	zx_write_buffer->kempston_mouse_whl=0xF0;
	zx_write_buffer->kempston_mouse_btn=0xFF;
	zx_write_buffer->kempston_mouse_x=0x80;
	zx_write_buffer->kempston_mouse_y=0x80;
}

void map_kmouse(struct WIIController* Wii_joy, ZX_Input_t* zx_input){
	if((Wii_joy->RightX>7)||(Wii_joy->RightX<-7)){zx_input->kempston_mouse_x+=(uint8_t)(Wii_joy->RightX>>5);}
	if((Wii_joy->RightY>7)||(Wii_joy->RightY<-7)){zx_input->kempston_mouse_y+=(uint8_t)(Wii_joy->RightY>>5);}
	if (Wii_joy->ButtonR)  zx_input->kempston_mouse_btn&=~1; else zx_input->kempston_mouse_btn|=1;
	if (Wii_joy->ButtonL)  zx_input->kempston_mouse_btn&=~2; else zx_input->kempston_mouse_btn|=2;
	if (Wii_joy->ButtonZR) zx_input->kempston_mouse_btn&=~4; else zx_input->kempston_mouse_btn|=4;
	if (Wii_joy->ButtonZL) zx_input->kempston_mouse_btn&=~8; else zx_input->kempston_mouse_btn|=8;		
	if((Wii_joy->LeftY>7)||(Wii_joy->LeftY<-7)){
		if(Wii_joy->LeftY>0) zx_input->kempston_mouse_whl-=(uint8_t)(1<<4);
		if(Wii_joy->LeftY<0) zx_input->kempston_mouse_whl+=(uint8_t)(1<<4);
		//printf("kempston_mouse_whl:%02X \n",zx_input->kempston_mouse_whl);
		zx_input->kempston_mouse_btn&=0x0F;
		zx_input->kempston_mouse_btn|=zx_input->kempston_mouse_whl;
		//printf("kempston_mouse_btn:%02X \n",zx_input->kempston_mouse_btn);
	}
	
}

void process_input(){
	if(i2cKbdMode){
		keyPressed = i2c_decode_kbd();
	} else {
		keyPressed = decode_PS2();
	}
		
	data_joy_input(active_type_joystick);
								
	uint32_t temp_data_joy;
	temp_data_joy = active_joystick_data(active_type_joystick);		// приведение данных с джойстиков к формату мурмулятора

	data_joy1 = (uint8_t)((temp_data_joy>>8)&0x000000ff);
	data_joy2 = (uint8_t)((temp_data_joy>>24)&0x000000ff);
	data_ext_joy1 = (uint8_t)((temp_data_joy)&0x000000ff);
	data_ext_joy2 = (uint8_t)((temp_data_joy>>16)&0x000000ff);

	data_joy1=(data_joy1&0x0f)|((data_joy1>>2)&0x30)|((data_joy1<<3)&0x80)|((data_joy1<<1)&0x40); 
	data_joy2=(data_joy2&0x0f)|((data_joy2>>2)&0x30)|((data_joy2<<3)&0x80)|((data_joy2<<1)&0x40);

	data_joy = data_joy1|(data_ext_joy1<<8)|(data_joy2<<16)|(data_ext_joy2<<24);

	//printf("JR[%08X]\tJ1[%02X]>[%02X]\tJ2[%02X]>[%02X]\tCJ[%08X]\n",temp_data_joy,data_joy1,data_ext_joy1,data_joy2,data_ext_joy2,data_joy);

	if(Joystics.joy_pressed) {joy_pressed = true;} else{joy_pressed = false;}
	if(Joystics.Present_WII_joy){map_kmouse(&Wii_joy_data,zx_write_buffer);}	
}

void clear_input(void){
	//printf("Clear input\n");
	kb_st_ps2.u[0]=0;
	kb_st_ps2.u[1]&=(KB_U1_L_SHIFT|KB_U1_L_CTRL|KB_U1_L_ALT|KB_U1_R_SHIFT|KB_U1_R_CTRL|KB_U1_R_ALT);
	kb_st_ps2.u[2]=0;
	kb_st_ps2.u[3]=0;
	//memset(kb_st_ps2.u,0,sizeof(kb_st_ps2.u));
	data_joy=0;
	old_data_joy=0;
	rel_data_joy=0;
	joy_pressed=false;
	// if(WII_Init){
	// 	Wii_clear_old();
	// }
}

bool wait_kbdjoy_emu(void){
	process_input();
	if((joy_pressed)||(KBD_PRESS)){
		//printf("key pressed %d "BYTE_TO_BINARY_PATTERN"\n", joy_pressed, BYTE_TO_BINARY(data_joy));
		return true;
	}
	clear_input();
	//printf("no key pressed\n");
	return false;
}

bool wait_kbdjoy_menu(void){
	process_input();
	if((joy_pressed)||(KBD_PRESS)){
		////printf("key pressed\n");
		return true;
	}
	////printf("no key pressed\n");
	return false;
}
/*process input*/





/*-------Graphics--------*/

void draw_mur_logo(){
	//return;
	draw_text_len((PREVIEW_POS_X+(PREVIEW_WIDTH/2)-((9*FONT_W)/2)),70,"NO SIGNAL",COLOR_ITEXT,COLOR_TEXT,9);
	for(uint8_t y=0;y<83;y++){
		for(uint8_t x=0;x<84;x++){
			uint8_t pixel = mur_logo[x+(y*84)];
			draw_pixel((PREVIEW_POS_X+(PREVIEW_WIDTH/2)-(84/2))+x,90+y,pixel);
		}
	}
}
/*
Border
	X+12+60
	Y+15+45
Screen
	X+21+43
	Y+21+33
Eye1
	X+99+3
	Y+27+3
Eye2
	X+111+3
	Y+27+3
*/

uint16_t animation_frame = 0;
uint32_t animation_action =0;

void draw_mur_logo_anim(short int xPos,short int yPos,uint8_t stripe_frame,uint8_t screen_frame,uint8_t eye_frame){
	//return;
	uint8_t pixel = 0xFF;
	for(uint8_t y=0;y<123;y++){
		for(uint8_t x=0;x<138;x++){
			pixel = mur_logo6[x+(y*138)];
			if(stripe_frame>0)
			if((x>11)&&(x<72)){
				if((y>14)&&(y<60)){
					pixel = mur_logo_stripes[stripe_frame-1][y-15];
				}
			}
			if(screen_frame>0)
			if((x>20)&&(x<64)){
				if((y>20)&&(y<53)){
					pixel = mur_logo_screens[screen_frame-1][(x-21)+((y-21)*42)];
				}
			}
			if(eye_frame>0){
				if((x>98)&&(x<102)){
					if((y>26)&&(y<30)){
						pixel = mur_logo_eyes[eye_frame-1][(x-99)+((y-27)*3)];
					}
				}
				if((x>110)&&(x<114)){
					if((y>26)&&(y<30)){
						pixel = mur_logo_eyes[eye_frame-1][(x-111)+((y-27)*3)];
					}
				}
			}
			if (pixel<0xFF){
				draw_pixel(xPos+x,yPos+y,pixel);
			}
		}
	}
}

void draw_mur_logo_big(short int xPos,short int yPos,uint8_t help){ //x-155 y-60
	//return;
	//for(uint8_t y=0;y<110;y++){
	draw_mur_logo_anim(xPos,yPos,0,0,0);
	if(help==1){
		/*
		draw_text_len(xPos+FONT_W+2,yPos+102+FONT_H*2,"   F1 - HELP   ",CL_BLUE,CL_WHITE,15);
		draw_text_len(xPos+FONT_W+2,yPos+102+FONT_H*3,"WIN,HOME-RETURN",CL_BLUE,CL_WHITE,15);	
		if(joy_connected){
			draw_text_len(xPos+FONT_W+2,yPos+102+FONT_H*4,"JOY[START]-EXIT",CL_BLUE,CL_WHITE,15);
		}
		*/
		draw_text_len(xPos+FONT_W+2,yPos+109+FONT_H*2,"   F1 - HELP   ",CL_BLUE,CL_WHITE,15);
		draw_text_len(xPos+FONT_W+2,yPos+109+FONT_H*3,"WIN,HOME-RETURN",CL_BLUE,CL_WHITE,15);	
		if(joy_connected){
			draw_text_len(xPos+FONT_W+2,yPos+109+FONT_H*4,"JOY[START]-EXIT",CL_BLUE,CL_WHITE,15);
		}

	} else 
	if(help==2){
		memset(temp_msg,0,sizeof(temp_msg));
		sprintf(temp_msg,"%-s %s",FW_VERSION,FW_AUTHOR);
		/*
		draw_text_len(xPos+FONT_W+2,yPos+102+FONT_H*2,temp_msg,CL_BLUE,CL_WHITE,15);
		memset(temp_msg, 0, sizeof(temp_msg));
		draw_text_len(xPos+FONT_W+2,yPos+102+FONT_H*4," WIN,HOME-MENU ",CL_BLUE,CL_WHITE,15);	
		draw_text_len(xPos+FONT_W+2,yPos+102+FONT_H*5,"   F1 - HELP   ",CL_BLUE,CL_WHITE,15);
		if(joy_connected){
			draw_text_len(xPos+FONT_W+2,yPos+102+FONT_H*6,"JOY[START]-MENU",CL_BLUE,CL_WHITE,15);
		}
		*/

		draw_text_len(xPos+1,yPos+110+FONT_H*2,temp_msg,CL_BLUE,CL_WHITE,17);
		memset(temp_msg, 0, sizeof(temp_msg));
		draw_text_len(xPos+FONT_W+2,yPos+110+FONT_H*4," WIN,HOME-MENU ",CL_BLUE,CL_WHITE,15);	
		draw_text_len(xPos+FONT_W+2,yPos+110+FONT_H*5,"   F1 - HELP   ",CL_BLUE,CL_WHITE,15);
		if(joy_connected){
			draw_text_len(xPos+FONT_W+2,yPos+110+FONT_H*6,"JOY[START]-MENU",CL_BLUE,CL_WHITE,15);
		}
		
	}
	
}

void draw_help_text(short int startLine,uint8_t lang){
	//printf("[%08X][%08X][%08X]\n",help_text,help_text_rus,&help_text[0]);
	for(uint8_t y=0;y<SCREEN_HELP_LINES;y++){
		memset(temp_msg,0,sizeof(temp_msg));
		////printf("%s\n",help_text[y]);
		if((startLine+y)>HELP_LINES){
			//draw_text_len(8,20+(y*FONT_H),"									 ",CL_BLACK,CL_WHITE,37);
			draw_text5x7_len(9,20+(y*FONT_5x7_H),"														   ",CL_BLACK,CL_WHITE,59);
			continue;
		} else {
			if (conv_utf_cp866(help_text[lang][startLine+y], temp_msg, strlen(help_text[lang][startLine+y]))>0){
				draw_text5x7_len(9,20+(y*FONT_5x7_H),temp_msg,CL_BLACK,CL_WHITE,59);
			}
		}
		////printf("%s\n",temp_msg);
		//memset(temp_msg, 0, sizeof(temp_msg));
	}
}

void MessageBox(char *header,char *message,uint8_t colorFG,uint8_t colorBG,uint8_t delay){
	if(menu_mode[menu_ptr]==EMULATION){
		zx_machine_enable_vbuf(false);
		busy_wait_ms(20);
	}	
	uint8_t max_len= strlen(header)>strlen(message)?strlen(header):strlen(message);
	if(max_len<10){max_len=10;}
	uint8_t left_x =(SCREEN_W/2)-((max_len/2)*FONT_W);
	uint8_t left_y = strlen(message)==0 ? (SCREEN_H/2)-(FONT_H/2):(SCREEN_H/2)-FONT_H;
	uint8_t height = FONT_H*2; //uint8_t height = strlen(message)>0 ? FONT_H*2+5:FONT_H+5;

	draw_rect(left_x-1,left_y,(max_len*FONT_W)+2,height+1,COLOR_MAIN_RAMK,true);//Основная рамка
	draw_rect(left_x,left_y+FONT_H,(max_len*FONT_W),FONT_H,colorBG,true);//Фон главного окна
	draw_stripes(left_x+((max_len-5)*FONT_W),left_y);

	if (strlen(message)>0){
		draw_text(left_x,left_y,header,colorFG,CL_EMPTY);
		draw_text(left_x,left_y+FONT_H,message,colorFG,CL_EMPTY);
	} else {
		draw_text(left_x,left_y+FONT_H,header,colorFG,CL_EMPTY);
	}
	////printf("X:%d,Y:%d\n",left_x,left_y);
	switch (delay){
		case 1:
			busy_wait_ms(3000);
			break;
		case 2:
			busy_wait_ms(750);
			break;
		case 3:
			busy_wait_ms(250);
			break;
		case 4:
			busy_wait_ms(1000);
			break;
		case 0xFF:
			while(!wait_kbdjoy_menu()){
				busy_wait_ms(30);	
			}
			break;			
		default:
		break;
	}
	if(menu_mode[menu_ptr]==EMULATION) zx_machine_enable_vbuf(true);
}

uint8_t DialogBox(char *header,char *message,uint8_t colorFG,uint8_t colorBG,uint8_t di_id){
	volatile uint8_t max_len = strlen(header)>strlen(message)?strlen(header):strlen(message);
	if(max_len<10){max_len=10;}
	volatile uint8_t left_x =(SCREEN_W/2)-((max_len/2)*FONT_W);
	volatile uint8_t left_y = strlen(message)==0 ? (SCREEN_H/2)-(FONT_H/2):(SCREEN_H/2)-FONT_H;
	volatile uint8_t height = FONT_H*3; //uint8_t height = strlen(message)>0 ? FONT_H*2+5:FONT_H+5;

	draw_rect(left_x-1,left_y,(max_len*FONT_W)+2,height+1,COLOR_MAIN_RAMK,true);//Основная рамка
	draw_rect(left_x,left_y+FONT_H,(max_len*FONT_W),FONT_H*2,colorBG,true);//Фон главного окна
	draw_stripes(left_x+((max_len-5)*FONT_W),left_y);

	if (strlen(message)>0){
		draw_text(left_x,left_y,header,colorFG,CL_EMPTY);
		draw_text(left_x,left_y+FONT_H,message,colorFG,CL_EMPTY);
	} else {
		draw_text(left_x,left_y+FONT_H,header,colorFG,CL_EMPTY);
	}
	////printf("X:%d,Y:%d\n",left_x,left_y);
	if(menu_mode[menu_ptr]==EMULATION){
		zx_machine_enable_vbuf(false);
		busy_wait_ms(20);
	}
	/*
#define DLG_RES_NONE	0x00
#define DLG_RES_CANCEL	0xFF
#define DLG_RES_OK		0x01
#define DLG_RES_RET		0x02
#define DLG_RES_ABR		0x03	
	*/
	short int dia_pos=0;
	uint8_t dia_res=0;
	need_redraw=true;
	while(true){
		process_input();
		busy_wait_us(500);
		if((KBD_LEFT)||(data_joy==D_JOY_LEFT)){
			dia_pos--;
			if(dia_pos<0){dia_pos=2;}
			while(strlen(iface_btn[di_id][dia_pos])<2){
				if(dia_pos<0){dia_pos=2;break;}
				dia_pos--;				
			}
			busy_wait_ms(150);
			need_redraw=true;
		}
		if((KBD_RIGHT)||(data_joy==D_JOY_RIGHT)){
			dia_pos++;
			while(strlen(iface_btn[di_id][dia_pos])<2){
				if(dia_pos>2){dia_pos=0;break;};
				dia_pos++;
			}
			if(dia_pos>2){dia_pos=0;};
			busy_wait_ms(150);
			need_redraw=true;
		}
		
		if((KBD_ENTER)||(data_joy==D_JOY_A)){
			dia_res = iface_res[di_id][dia_pos];
			busy_wait_ms(150);
			break;
		}
		if((KBD_ESC)||(data_joy&D_JOY_START)){
			dia_res=DLG_RES_NONE;
			need_redraw=true;
			busy_wait_ms(150);
			break;
		}
		if(need_redraw){
			short int pos = left_x+(max_len*FONT_W);
			short int max_btn = 2;
			do{
				if(strlen(iface_btn[di_id][max_btn])>1){
					pos-=strlen(iface_btn[di_id][max_btn])*FONT_W;
					memset(temp_msg,0,sizeof(temp_msg));
					strcpy(temp_msg,iface_btn[di_id][max_btn]);
					if(dia_pos==max_btn){
						temp_msg[0]=0x10;
						temp_msg[strlen(iface_btn[di_id][max_btn])-1]=0x11;
					}					
					draw_text(pos,left_y+(FONT_H*2),temp_msg,COLOR_TEXT,dia_pos==max_btn?COLOR_CURRENT_BG:COLOR_BACKGOUND);
				}
				max_btn--;
			} while(max_btn>=0);
			need_redraw=false;
		}
	}
	if(menu_mode[menu_ptr]==EMULATION) zx_machine_enable_vbuf(true);
	clear_input();
	return dia_res;
}

uint8_t EditDialogBox(char *header,char *message,char *value,uint8_t colorFG,uint8_t colorBG,uint8_t di_id,bool in_type){
	volatile uint8_t max_len = strlen(header)>strlen(message)?strlen(header):strlen(message);
	if(max_len<10){max_len=10;}
	volatile uint8_t left_x =(SCREEN_W/2)-((max_len/2)*FONT_W);
	volatile uint8_t left_y = strlen(message)==0 ? (SCREEN_H/2)-(FONT_H/2):(SCREEN_H/2)-FONT_H;
	volatile uint8_t height = FONT_H*4; //uint8_t height = strlen(message)>0 ? FONT_H*2+5:FONT_H+5;

	draw_rect(left_x-1,left_y,(max_len*FONT_W)+2,height+1,COLOR_MAIN_RAMK,true);//Основная рамка
	draw_rect(left_x,left_y+FONT_H,(max_len*FONT_W),FONT_H*2,colorBG,true);//Фон главного окна
	draw_stripes(left_x+((max_len-5)*FONT_W),left_y);

	volatile uint8_t value_len = strlen(value);
	volatile short int max_pos = strlen(value)+3;
	if (strlen(message)>0){
		draw_text(left_x,left_y,header,colorFG^0x0F,CL_EMPTY);
		draw_text(left_x,left_y+FONT_H,message,colorFG,CL_EMPTY);
		draw_text_len(left_x,left_y+(FONT_H*2),value,colorFG,CL_GRAY,value_len);
	} else {
		draw_text(left_x,left_y+FONT_H,header,colorFG^0x0F,CL_EMPTY);
		draw_text_len(left_x,left_y+(FONT_H*2),value,colorFG,CL_GRAY,value_len);
	}
	////printf("X:%d,Y:%d\n",left_x,left_y);
	if(menu_mode[menu_ptr]==EMULATION){
		zx_machine_enable_vbuf(false);
		busy_wait_ms(20);
	}
	/*
#define DLG_RES_NONE	0x00
#define DLG_RES_CANCEL	0xFF
#define DLG_RES_OK		0x01
#define DLG_RES_RET		0x02
#define DLG_RES_ABR		0x03	
	*/
	short int dia_pos=0;
	uint8_t dia_res=0;
	uint8_t temp=0;
	need_redraw=true;
	while(true){
		process_input();
		busy_wait_us(500);
		if((KBD_LEFT)||(data_joy==D_JOY_LEFT)){
			dia_pos--;
			if(dia_pos<0){dia_pos=max_pos;}
			if(dia_pos>value_len){
				while(iface_res[di_id][dia_pos-value_len]<DLG_RES_OK){
					dia_pos--;
					//printf("dia_pos1:%04d\n",dia_pos);
					if(dia_pos<0){dia_pos=max_pos;break;}
				}
			}
			busy_wait_ms(150);
			need_redraw=true;
		}
		if((KBD_RIGHT)||(data_joy==D_JOY_RIGHT)){
			dia_pos++;
			if(dia_pos>value_len){
				while(iface_res[di_id][dia_pos-value_len]<DLG_RES_OK){
					dia_pos++;
					//printf("dia_pos2:%04d\n",dia_pos);
					if(dia_pos>max_pos){dia_pos=0;break;};
				}
			}
			if(dia_pos>max_pos){dia_pos=0;};
			busy_wait_ms(150);
			need_redraw=true;
		}
		if(dia_pos<value_len){
			if((KBD_UP)||(data_joy==D_JOY_UP)){
				value[dia_pos]++;
				if(value[dia_pos]>0x39) value[dia_pos]=0x30;
				busy_wait_ms(150);
				need_redraw=true;
			}
			if((KBD_DOWN)||(data_joy==D_JOY_DOWN)){
				value[dia_pos]--;
				if(value[dia_pos]<0x30) value[dia_pos]=0x39;
				busy_wait_ms(150);
				need_redraw=true;
			}
			if(KBD_PRESS){
				uint8_t chr = convert_kb_u_to_char(kb_st_ps2,in_type);
				if(chr>0){				
					value[dia_pos]=chr;
					dia_pos++;
					busy_wait_ms(150);
					need_redraw=true;
				}
			}

		}
		
		if((KBD_ENTER)||(data_joy==D_JOY_A)){
			if(dia_pos>=value_len){
				dia_res = iface_res[di_id][dia_pos-value_len];
				//printf("dia_res:%02X\n",dia_res);
				busy_wait_ms(150);
				break;
			}
		}
		if((KBD_ESC)||(data_joy&D_JOY_START)){
			dia_res=DLG_RES_NONE;
			need_redraw=true;
			busy_wait_ms(150);
			break;
		}
		if(need_redraw){
			short int pos = left_x+(max_len*FONT_W);
			short int max_btn = 2;
			do{
				if(strlen(iface_btn[di_id][max_btn])>1){
					pos-=strlen(iface_btn[di_id][max_btn])*FONT_W;
					draw_text(
						pos,
						left_y+(FONT_H*3),
						iface_btn[di_id][max_btn],
						COLOR_TEXT,
						(dia_pos-value_len)==max_btn?COLOR_CURRENT_BG:COLOR_BACKGOUND
					);
				}
				max_btn--;
			} while(max_btn>=0);
			draw_text_len(left_x,left_y+(FONT_H*2),value,colorFG,CL_GRAY,value_len);
			if(dia_pos<=value_len){
				draw_text_len(left_x+(FONT_W*dia_pos),left_y+(FONT_H*2),&value[dia_pos],colorFG,COLOR_CURRENT_BG,1);
			}
			need_redraw=false;
			//printf("dia_pos:%03d\n",dia_pos);
		}
	}
	if(menu_mode[menu_ptr]==EMULATION) zx_machine_enable_vbuf(true);
	clear_input();
	return dia_res;
}


void draw_main_window(){
	draw_rect(0,0,SCREEN_W,SCREEN_H,COLOR_FULLSCREEN,true);//Заливаем экран //
	draw_rect(FONT_W-1,FONT_H,(SCREEN_W-(FONT_W*2))+2,(SCREEN_H-(FONT_H*2))+1,COLOR_MAIN_RAMK,true);//Основная рамка
	draw_rect(FONT_H,(FONT_W*2),SCREEN_W-(FONT_W*2),SCREEN_H-(FONT_H*3),COLOR_BACKGOUND,true);//Фон главного окна
	draw_logo_header(((SCREEN_W-FONT_W)-SPEC_LOGO_W)-(10*FONT_W),FONT_H);
	memset(temp_msg,0,sizeof(temp_msg));
	sprintf(temp_msg,"%s %s",FW_VERSION,FW_AUTHOR);
	draw_text_len(FONT_W,FONT_H,temp_msg,COLOR_BACKGOUND,CL_EMPTY,20);
	//memset(temp_msg, 0, sizeof(temp_msg));
}

void draw_file_window(){
	draw_rect(PREVIEW_POS_X,SCREEN_H-(FONT_H*4),PREVIEW_WIDTH,(FONT_H*3),COLOR_BACKGOUND,true);//Фон отображения информации о файле //COLOR_BACKGOUND
	draw_rect(PREVIEW_POS_X-FONT_W,PREVIEW_POS_Y-1,FONT_W,SCREEN_H-(FONT_H*3)+2,COLOR_MAIN_RAMK,false);  //Рамка полосы прокрутки //COLOR_BORDER
	
	//draw_rect(7,SCREEN_H-FONT_H-8,9+FONT_W*FILE_NAME_LEN,FONT_H+1,COLOR_BORDER,false);//панель подсказок под файлами
	//draw_text_len(8,SCREEN_H-FONT_H-7,"F1-HLP,HOME-RET",COLOR_TEXT,COLOR_BACKGOUND,15);//get_file_from_dir("0:/z80",i)
	
	//*draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,SCREEN_H-25,COLOR_BACKGOUND,true);  
	//draw_rect(PREVIEW_POS_X+11,16+11,160,120,0x0,true);
	//*draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,192,COLOR_BACKGOUND,true);
	//*draw_line(PREVIEW_POS_X,209,17+FONT_W*37,209,COLOR_PIC_BG);
	
}

void draw_fast_menu(uint8_t xPos,uint8_t yPos,bool drawbg,uint8_t menu,uint8_t active){
	////printf("xPos:%d  yPos:%d  width:%d  height:%d\n",xPos,yPos,width,height);
	//draw_rect(0,0,SCREEN_W,SCREEN_H,COLOR_FULLSCREEN,true);//Заливаем экран 
	uint8_t lines = 0;
	for(uint8_t i=0;i<FAST_MENU_LINES;i++){
		if(*fast_menu[menu][i]==0){
			lines=i;
			break;
		}
	}
	uint8_t width =(16*FONT_W)+2;
	uint8_t height =(lines*FONT_H)+FONT_H+1;
	////printf("xPos:%d  yPos:%d  width:%d  height:%d lines:%d\n",xPos,yPos,width,height,lines);
	if(drawbg){
		draw_rect(xPos-1,yPos,width,height,COLOR_MAIN_RAMK,true);//Основная рамка
		draw_rect(xPos,yPos+FONT_H,width-2,height-9,COLOR_BACKGOUND,true);//Фон главного окна
		draw_logo_header(xPos+2,yPos);
	}
	for(uint8_t y=0;y<lines;y++){
		//memset(temp_msg,0,sizeof(temp_msg));
		////printf("%s\n",help_text[y]);
		/*if((startLine+y)>HELP_LINES){
			draw_text_len(8,20+(y*8),"									 ",CL_BLACK,CL_WHITE,37);
			continue;
		}*/
		/*if (convert_utf8_to_windows1251(help_text[startLine+y], temp_msg, strlen(help_text[startLine+y]))>0){
			
		};*/
		if((menu==0)&&(!init_fs)&&((y==0)||(y==2)||(y==3)||(y==4))){
			draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),fast_menu[menu][y],COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,16);	
		} else{
			if (y==7){
				memset(temp_msg,0,sizeof(temp_msg));
				sprintf(temp_msg,fast_menu[menu][y],hat_locked>0?"*":" ");
				draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),temp_msg,COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,16);
			} else 			
			draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),fast_menu[menu][y],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,16);
		}
		if(menu==1){
			draw_text_len(xPos+FONT_W,(yPos+FONT_H)+(y*FONT_H),"*",save_slots[y]==1?CL_RED:CL_GREEN,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,1);
		}
		if(menu==2){
			draw_text_len(xPos+FONT_W,(yPos+FONT_H)+(y*FONT_H),"*",save_slots[y]==1?CL_GREEN:CL_RED,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,1);
		}
	}
	//memset(temp_msg, 0, sizeof(temp_msg));
}

void draw_config_menu(uint8_t xPos,uint8_t yPos,bool drawbg,uint8_t active){
	uint8_t lines = 0;
	for(uint8_t i=0;i<CONFIG_MENU_ITEMS;i++){
		if(*config_menu[i]==0){
			lines=i;
			break;
		}
	}
	uint8_t width =(27*FONT_W)+2;
	uint8_t height =(lines*FONT_H)+FONT_H+1;
	////printf("xPos:%d  yPos:%d  width:%d  height:%d lines:%d\n",xPos,yPos,width,height,lines);
	if(drawbg){
		draw_rect(xPos-1,yPos,width,height,COLOR_MAIN_RAMK,true);//Основная рамка
		draw_rect(xPos,yPos+FONT_H,width-2,height-9,COLOR_BACKGOUND,true);//Фон главного окна
		draw_logo_header((xPos+width)-(16*FONT_W)+(FONT_W-2),yPos);
		draw_text_len(xPos,yPos,"[SETTINGS]",COLOR_ITEXT,CL_EMPTY,10);
		
	} 		
	for(uint8_t y=0;y<lines;y++){
		#ifdef VGA_HDMI
		if((g_out)cfg_video_out<g_out_TFT_ST7789){
			if((y>16)&&(y<(settings_lines-4))){
				draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),config_menu[y],COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,27);
				continue;
			}
		}
		#endif
		if((!init_fs)&&((y==settings_lines-4)||(y==settings_lines-3)||(y==settings_lines-2))){
			draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),config_menu[y],COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,27);
		} else {
			draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),config_menu[y],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,27);			
		}
		if(y==0){
			draw_text_len(xPos+(14*FONT_W),(yPos+FONT_H)+(y*FONT_H),boot_scr_config[cfg_boot_scr],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,12);
		}	
		if(y==1){
			draw_text_len(xPos+(22*FONT_W),(yPos+FONT_H)+(y*FONT_H),HUD_config[cfg_hud_enable],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,4);
		}		
		if(y==2){
			draw_text_len(xPos+(10*FONT_W),(yPos+FONT_H)+(y*FONT_H),tap_load_config[cfg_tap_load_mode],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,16);
		}
		if(y==3){
			draw_text_len(xPos+(20*FONT_W),(yPos+FONT_H)+(y*FONT_H),ext_tape_load_config[cfg_tape_load_pin],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,6);
		}
		if(y==4){
			draw_text_len(xPos+(14*FONT_W),(yPos+FONT_H)+(y*FONT_H),joy_config[cfg_def_joy1_mode],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,12);
		}
		if(y==5){
			draw_text_len(xPos+(14*FONT_W),(yPos+FONT_H)+(y*FONT_H),joy_config[cfg_def_joy2_mode],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,12);
		}
		if(y==6){
			draw_text_len(xPos+(14*FONT_W),(yPos+FONT_H)+(y*FONT_H),kbd_config[cfg_def_kbd_mode],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,12);
		}
		if(y==7){
			draw_text_len(xPos+(23*FONT_W),(yPos+FONT_H)+(y*FONT_H),yes_no[cfg_res_before_mode],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,3);
		}
		if(y==8){
			draw_text_len(xPos+(10*FONT_W),(yPos+FONT_H)+(y*FONT_H),sound_config[cfg_sound_mode],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,15);
		}
		if(y==9){
			if((cfg_sound_mode>NO_SOUND)&&(cfg_sound_mode<HARDWARE_TS)){
				draw_text_len(xPos+(23*FONT_W),(yPos+FONT_H)+(y*FONT_H),sound_out_config[cfg_sound_out_mode],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,3);
			} else {
				draw_text_len(xPos+(23*FONT_W),(yPos+FONT_H)+(y*FONT_H),sound_out_config[cfg_sound_out_mode],COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,3);
			}
		}
		if(y==10){
			if((cfg_sound_mode>NO_SOUND)&&(cfg_sound_mode<HARDWARE_TS)){
				draw_text_len(xPos+(16*FONT_W),(yPos+FONT_H)+(y*FONT_H),gaudge[(cfg_volume/25)],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,10);
			} else {
				draw_text_len(xPos+(16*FONT_W),(yPos+FONT_H)+(y*FONT_H),gaudge[(cfg_volume/25)],COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,10);
			}
		}
		if(y==11){
			draw_text_len(xPos+(20*FONT_W),(yPos+FONT_H)+(y*FONT_H),sound_clock_config[cfg_tspin_mode],(cfg_sound_mode==HARDWARE_TS)?COLOR_TEXT:COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,6);
		}
		if(y==12){
			draw_text_len(xPos+(19*FONT_W),(yPos+FONT_H)+(y*FONT_H),sound_speed_config[cfg_tsspeed_mode],(cfg_sound_mode==HARDWARE_TS)?COLOR_TEXT:COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,7);
		}
		if(y==13){
			draw_text_len(xPos+(18*FONT_W),(yPos+FONT_H)+(y*FONT_H),ts_chip_config[cfg_tschip_order],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,8);
		}
		if(y==14){
			draw_text_len(xPos+(16*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_out_config[cfg_video_out],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,10);
		}
		if(y==15){
			if(graphics_try_framerate((g_out)cfg_video_out,(fr_rate)cfg_frame_rate,false)){
				draw_text_len(xPos+(20*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_freq_config[cfg_frame_rate],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,6);
			} else {
				draw_text_len(xPos+(20*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_freq_config[cfg_frame_rate],COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,6);
			}
		}
		if(y==16){
			draw_text_len(xPos+(23*FONT_W),(yPos+FONT_H)+(y*FONT_H),yes_no[cfg_mobile_mode],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,3);
		}		
		#ifdef VGA_HDMI
		if((g_out)cfg_video_out>g_out_HDMI){
			if(y==17){
				draw_text_len(xPos+(16*FONT_W),(yPos+FONT_H)+(y*FONT_H),gaudge[cfg_brightness],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,10);
			}
			if(y==18){
				draw_text_len(xPos+(16*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_out_rotate[cfg_rotate],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,10);
			}
			if(y==19){
				draw_text_len(xPos+(16*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_out_inversion[cfg_inversion],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,10);
			}		
			if(y==20){
				draw_text_len(xPos+(21*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_out_pixels[cfg_pixels],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,5);
			}		
		} else {

		}
		#endif
	}
	////printf("xPos:%d  yPos:%d  width:%d  height:%d\n",xPos,yPos,width,height);
	//draw_rect(0,0,SCREEN_W,SCREEN_H,COLOR_FULLSCREEN,true);//Заливаем экран 
	//memset(temp_msg, 0, sizeof(temp_msg));
}

void send_keystroke(uint8_t stroke){
	////printf("Stroke:%d   length:%d >>> %s\n",stroke,strlen(kbd_fcmd[stroke]),kbd_fcmd[stroke]);
	memset(zx_write_buffer->kb_data,0,8);

	for (uint8_t key_idx=0; key_idx<strlen(kbd_fcmd[stroke]);key_idx++){
		uint8_t key = (uint8_t)kbd_fcmd[stroke][key_idx];
		////printf("key>%02X\n",key);
		//if (key==0x01){	zx_write_buffer->kb_data[0]|=(1<<0);}
		//if (key==0x02){	zx_write_buffer->kb_data[7]|=(1<<1);}
		if((key>0x00)&&(key<0xFF)){
			////printf("search\n");
			for (uint8_t x=0;x<10;x++){
				for (uint8_t y=1;y<5;y++){
					////printf("s>%02X >%d>%d\n",kbd_chr[y][x],x,y);
					if(key==kbd_chr[y][x]){
						////printf("f>%d>%d\n",x,y);
						zx_write_buffer->kb_data[kbd_idx[y][x]]|=kbd_codes[x];
						break;
					}
				}
			}
		}
		if(key==0xFF){
			zx_machine_input_set();
			busy_wait_ms(50);
			memset(zx_write_buffer->kb_data,0,8);
			zx_machine_input_set();
			busy_wait_ms(50);
		}
		if(key==0){
			memset(zx_write_buffer->kb_data,0,8);
			zx_machine_input_set();
			busy_wait_ms(50);
			return;
		}
	}
}

/*
void draw_keyboard(uint8_t xPos,uint8_t yPos){ //x:33 y:84
	int img_first_line=kbd_full_yfpos[kbd_row];
	int img_last_line=kbd_full_ytpos[kbd_row];
	uint8_t pixel = 0xFF;
	for(uint8_t y=0;y<(img_last_line-img_first_line);y++){
		for(uint8_t x=0;x<254;x++){
			pixel = kbd_img[x+((img_first_line+y)*254)];
			if((x>=kbd_xfpos[kbd_row][kbd_col])&&(x<=kbd_xtpos[kbd_row][kbd_col])){
				if(pixel==0x07){pixel=0x04;}; //0x0F
			} else {
				if(((img_first_line+y)>=kbd_btn_yfpos[kbd_row])&&((img_first_line+y)<kbd_btn_ytpos[kbd_row])){
					pixel = kbd_img[x+((img_first_line+y)*254)];
				} else{
					pixel = 0xFF;
				}
			}
			
			if((kbd_cshift)&&(kbd_row==4)){
				if((x>=kbd_xfpos[kbd_row][0])&&(x<=kbd_xtpos[kbd_row][0])){
					if(pixel==0x07){pixel=0x06;}; //0x0F
				}
			}
			if((kbd_sshift)&&(kbd_row==4)){
				if((x>=kbd_xfpos[kbd_row][8])&&(x<=kbd_xtpos[kbd_row][8])){
					if(pixel==0x07){pixel=0x01;}; //0x0F
				}
			}			
			if (pixel<0xFF)	draw_pixel(xPos+x,(yPos+y)+kbd_row_full_shift[kbd_row],pixel);
			////printf("X:%d Y:%d C:[%02X]\n",x,y,pixel);
		}
		/if((kbd_vis==false)&&(y>(kbd_ytpos[1]+FONT_H))){
			return;
		}/
	}
}
*/
/*
void draw_keyboard(uint8_t xPos,uint8_t yPos){ //x:33 y:84
	for(uint8_t y=0;y<160;y++){
		for(uint8_t x=0;x<254;x++){
			uint8_t pixel = kbd_img[x+(y*254)];
			if((x>=kbd_xfpos[kbd_row][kbd_col])&&(x<=kbd_xtpos[kbd_row][kbd_col])){
				if((y>=kbd_yfpos[kbd_row])&&(y<=kbd_ytpos[kbd_row])){
					if(pixel==0x07){pixel=0x04;}; //0x0F
				}
			}
			if(kbd_cshift){
				if((x>=kbd_xfpos[4][0])&&(x<=kbd_xtpos[4][0])){
					if((y>=kbd_yfpos[4])&&(y<=kbd_ytpos[4])){
						if(pixel==0x07){pixel=0x06;}; //0x0F
					}
				}
			}
			if(kbd_sshift){
				if((x>=kbd_xfpos[4][8])&&(x<=kbd_xtpos[4][8])){
					if((y>=kbd_yfpos[4])&&(y<=kbd_ytpos[4])){
						if(pixel==0x07){pixel=0x01;}; //0x0F
					}
				}
			}			
			if (pixel<0xFF)	draw_pixel(xPos+x,yPos+y,pixel);
			////printf("X:%d Y:%d C:[%02X]\n",x,y,pixel);
		}
		if((kbd_vis==false)&&(y>(kbd_ytpos[1]+FONT_H))){
			return;
		}
	}
}
*/

bool LoadTxt(char *file_name){
#ifndef DEBUG_DISABLE_LOADERS	
	short int res =0;
	size_t bytesRead;
	size_t bytesToRead;
	size_t FileSize;

	memset(temp_buffer_y, 0, TEMP_BUFF_SIZE_Y);

	res = sd_open_file(&sd_file,file_name,FA_READ);
	////printf("sd_open_file=%d\n",res);
	if (res!=FR_OK){sd_close_file(&sd_file);return false;}
   	FileSize = sd_file_size(&sd_file);
	printf("text Filesize %u bytes\n", FileSize);
	
	/*uint16_t ptr=0;
	do{
		//printf("[%04X]",ptr);
		for (uint8_t col=0;col<16;col++){
			//printf("\t%02X",sd_buffer[ptr]);
			ptr++;
		}
		//printf("\n");
	} while(ptr<sizeof(sd_buffer));
	//printf("\n");*/


	memset(temp_msg, 0, sizeof(temp_msg));
	sprintf(temp_msg,"File size:%dk",(short int)(FileSize/1024));
	draw_text_len(18+FONT_W*FILE_NAME_LEN,216, temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);

	res = sd_read_file(&sd_file,temp_buffer_y,TEMP_BUFF_SIZE_Y-5,&bytesRead);
	if (res!=FR_OK){sd_close_file(&sd_file);return false;}
	draw_text_len(18+FONT_W*FILE_NAME_LEN,16,"File contents:",COLOR_TEXT,COLOR_BACKGOUND,14);
	uint16_t ptr=0;
	uint8_t len=0;
	for (uint8_t i = 0; i < (PREVIEW_HEIGHT/FONT_5x7_H); i++){
		memset(temp_msg, 0, sizeof(temp_msg));
		for (uint8_t j = 0; j < (PREVIEW_WIDTH/FONT_5x7_W); j++){
			if((temp_buffer_y[ptr+j]==0x0A)){
				len=j+1;
				break;
			}
			if(temp_buffer_y[ptr+j]==0x00){
				len=j;	
				break;
			}
			len=j;
		}
		//printf("ptr:%d len:%d \n",ptr,len);
		memcpy(temp_msg,&temp_buffer_y[ptr],len);
		ptr+=len;
		if(len>0){
			draw_text5x7_len(PREVIEW_POS_X,25+(FONT_5x7_H)*i,temp_msg,COLOR_TEXT,COLOR_BACKGOUND,(PREVIEW_WIDTH/FONT_5x7_W));
		}
		if(len==0){
			uint8_t k=0;
			while((temp_buffer_y[ptr+k]!=0x0A)||(temp_buffer_y[ptr+k]!=0x00)){
				k++;
			}
			ptr+=k;
		}
		if(ptr>=TEMP_BUFF_SIZE_Y){break;}
		if(ptr>=FileSize){break;}
	}
	sd_close_file(&sd_file);
	return true;
#endif
}

/*-------Graphics--------*/
/*-------Soundrive--------*/
uint16_t SoundLeft=0;
uint16_t SoundRight=0;
uint8_t SoundLeft_A=0;
uint8_t SoundLeft_B=0;
uint8_t SoundRight_A=0;
uint8_t SoundRight_B=0;  

void FAST_FUNC(Soundrive)(uint8_t port,uint8_t val){

  if (port == 0xFB) {  SoundRight = (uint16_t)val;SoundLeft = SoundRight;return;}        //COVOX MONO
                                                //SounDrive пока 8 бит хотелось бы 10 -12
  if (port == 0x0F) {SoundLeft_A = val; SoundLeft = (SoundLeft_A * SoundLeft_B)/256;return;}
  if (port == 0x1F) {SoundLeft_B = val; SoundLeft = (SoundLeft_A * SoundLeft_B)/256;return;}
  if (port == 0x4F) {SoundRight_A = val;SoundRight = (SoundRight_A * SoundRight_B)/256;return;}  
  if (port == 0x5F) {SoundRight_B = val;SoundRight = (SoundRight_A * SoundRight_B)/256;}            
}
/*-------Soundrive--------*/
/*-------AY inits--------*/


repeating_timer_t soft_sound_timer;
bool ay_timer_enabled;
bool ts_595_enabled;

//функции вывода звука спектрума
//uint8_t AY_DATA_R=0;
//uint8_t AY_DATA_L=0;

//uint8_t* AY_data0 = NULL;
extern uint8_t outs[6];

static uint8_t beep_data;
static uint8_t beep_data_old;
static bool bepper_out = false;
volatile uint8_t tape_data;
volatile uint8_t tape_data_old;
static bool ldout = false;

static int outL=0;
static int outR=0; 

static int outL_old=0;  
static int outR_old=0; 

static int tape_signed=0;
static int beeper_signed=0;
static int outL_signed=0;
static int outR_signed=0;

static long mixL=0;
static long mixR=0;

//bool __scratch_y("sound_load") hw_zx_get_bit_LOAD()
bool FAST_FUNC(hw_zx_get_bit_LOAD)(){
#ifndef DEBUG_DISABLE_LOADERS	
	tape_data_old = tape_data;
	if ((tap_loader_active&TAPE_INTERNAL_AUTO)||(tap_loader_active&TAPE_INTERNAL_MANU)){ // Переменная определена в util_tap.h
		if((TapeStatus==TAPE_LOADING)){
			tape_data = TAP_Read(); // Переменная определена в util_tap.h
		} else {
			tape_data = 0;
		}
	} 
	if(tap_loader_active&TAPE_EXTERNAL){
		tape_data = gpio_get(tape_load_pin);
	};
	ldout^=(tape_data==tape_data_old)?0:1;
	if ((tap_loader_active&TAPE_INTERNAL_AUTO)||(tap_loader_active&TAPE_INTERNAL_MANU)||(tap_loader_active&TAPE_EXTERNAL)){ 
		if(cfg_sound_mode == 4){
			AY_to595Beep(ldout);
		} else {
			if(cfg_sound_out_mode==OUT_PWM){
				pwm_set_gpio_level(ZX_BEEP_PIN,(((ldout*254)*(cfg_volume))/100));
			}
			if(cfg_sound_out_mode==OUT_PCM){
				if(((tape_data!=0)||(tape_data_old!=0))) { //&&(tape_signed==0)
					mixL = 1;
					mixR = 1;
					tape_signed = (bool)ldout?128:0;
					mixL = (2*(mixL+tape_signed))-((mixL*tape_signed)/128)-128;
					mixR = (2*(mixR+tape_signed))-((mixR*tape_signed)/128)-128;
					i2s_out(((int)(mixL)*4)*(cfg_volume/8),((int)(mixR)*4)*(cfg_volume/8));					
				}
				
			}
		}
	}
	return ldout;
#endif
};

void FAST_FUNC(hw_zx_set_beep_out)(uint8_t val){
	beep_data_old=beep_data;
	if(cfg_sound_mode>0){
		beep_data=val>>3;
		//printf(BYTE_TO_BINARY_PATTERN" - "BYTE_TO_BINARY_PATTERN"\n",BYTE_TO_BINARY(beep_data),BYTE_TO_BINARY(val));
	} else {
		beep_data=0;
	}
	bepper_out^=(beep_data==beep_data_old)?false:true;
	if(cfg_sound_mode == 4){
		AY_to595Beep(bepper_out);
	} else {
		if(cfg_sound_out_mode==OUT_PWM){
			pwm_set_gpio_level(ZX_BEEP_PIN,(((bepper_out*255)*(cfg_volume))/100));
		}
		if(cfg_sound_out_mode==OUT_PCM){
			//if(beep_data!=beep_data_old) gpio_put(WORK_LED_PIN,1);
			if((beep_data!=0)||(beep_data_old!=0)){
				mixL = 1;
				mixR = 1;
				beeper_signed = (bool)bepper_out?128:0;
				mixL = (2*(mixL+beeper_signed))-((mixL*beeper_signed)/128)-128;
				mixR = (2*(mixR+beeper_signed))-((mixR*beeper_signed)/128)-128;
				i2s_out(((int)(mixL)*4)*(cfg_volume/8),((int)(mixR)*4)*(cfg_volume/8));
			}
		}
	};
}


//#pragma GCC push_options
//#pragma GCC optimize("-Ofast")
//bool __scratch_y("sound_ayemu") AY_timer_callback(repeating_timer_t *rt)
bool FAST_FUNC(AY_timer_callback)(repeating_timer_t *rt){
	outL_old=outL;
	outR_old=outR;
	outL=(((2*(uint16_t)outs[0])+(2*(uint16_t)outs[3])+(uint16_t)outs[1]+(uint16_t)outs[4])+(SoundLeft));
	outR=(((2*(uint16_t)outs[2])+(2*(uint16_t)outs[5])+(uint16_t)outs[1]+(uint16_t)outs[4])+(SoundRight));
	if(cfg_sound_out_mode==OUT_PWM){
		pwm_set_gpio_level(ZX_AY_PWM_PIN0,(uint8_t)((outR*cfg_volume)/100)); // Право
		pwm_set_gpio_level(ZX_AY_PWM_PIN1,(uint8_t)((outL*cfg_volume)/100)); // Лево
	}
	if(cfg_sound_out_mode==OUT_PCM){
		mixL=1;
		mixR=1;
		if((outL!=outL_old)||(outR!=outR_old)){
			mixL = (2*(mixL+outL))-((mixL*outL)/128)-128;
			mixR = (2*(mixR+outR))-((mixR*outR)/128)-128;
			i2s_out((int)(((int)(mixR)*4)*(cfg_volume/CFG_VOLUME_STEP)),(int)(((int)(mixL)*4)*(cfg_volume/CFG_VOLUME_STEP)));
		}
	}
	if(!covox_mode){
		//AY_data0=get_AY_Out(0,AY_SAMPLE_MUL);
		//AY_data0=get_AY_Out(1,AY_SAMPLE_MUL);
		get_AY_Out(0,AY_SAMPLE_MUL);
		get_AY_Out(1,AY_SAMPLE_MUL);
	} 
	//outL=(((2*(uint16_t)AY_data0[0])+(2*(uint16_t)AY_data0[3])+(uint16_t)AY_data0[1]+(uint16_t)AY_data0[4])+((uint16_t)SoundLeft));//-(((2*AY_data0[0])*(2*AY_data0[3])*AY_data0[1]*AY_data0[4])*(SoundLeft));
	//outR=(((2*(uint16_t)AY_data0[2])+(2*(uint16_t)AY_data0[5])+(uint16_t)AY_data0[1]+(uint16_t)AY_data0[4])+((uint16_t)SoundRight));//-(((2*AY_data0[2])*(2*AY_data0[5])*AY_data0[1]*AY_data0[4])*(SoundRight));
	return true;
}
//#pragma GCC pop_options

/*
	/void __scratch_x("sound_beep") hw_zx_set_snd_out(bool val)
void FAST_FUNC(hw_zx_set_snd_out)(bool val){
	static bool out;
	beep_data_old=beep_data;
	if(cfg_sound_mode>0){
		beep_data=(beep_data&0b01)|(val<<1);
	} else {
		beep_data=0;
	}
	out^=(beep_data==beep_data_old)?0:1;	
	if(cfg_sound_mode == 4){
		AY_to595Beep(out);
	} else {
		pwm_set_gpio_level(ZX_BEEP_PIN,(((out*254)*(cfg_volume*OVERDRIVE_VOLUME_MODE))/100)); // Право
	};

};


//void __scratch_y("sound_save") hw_zx_set_save_out(bool val)
void FAST_FUNC(hw_zx_set_save_out)(bool val){
	static bool out;
	beep_data_old=beep_data;
	if(cfg_sound_mode>0){
		beep_data=(beep_data&0b10)|(val<<0);
	} else {
		beep_data=0;
	}
	out^=(beep_data==beep_data_old)?0:1;
	if(cfg_sound_mode == 4){
		AY_to595Beep(out);
	} else {
		pwm_set_gpio_level(ZX_BEEP_PIN,(((out*254)*(cfg_volume*OVERDRIVE_VOLUME_MODE))/100)); // Право
	};
};
*/

void PWM_init_pin(uint pinN){
	gpio_set_function(pinN, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(pinN);
	
	pwm_config c_pwm=pwm_get_default_config();
	pwm_config_set_clkdiv(&c_pwm,1.0);
	pwm_config_set_wrap(&c_pwm,255);//MAX PWM value
	pwm_init(slice_num,&c_pwm,true);
}

static void PWM_Deinit_pin(uint pinN){
	if(gpio_get_function(pinN)==GPIO_FUNC_PWM){	
		uint slice_num = pwm_gpio_to_slice_num(pinN);
		////printf("ay deslice_num:%u\n",slice_num);
		pwm_set_enabled (slice_num, false);
		pwm_config c_pwm=pwm_get_default_config();
		pwm_init(slice_num,&c_pwm,false);
		gpio_deinit(pinN);
	}
}

bool Init_Soft_AY(){
	if(!ay_timer_enabled){
		//printf("Init Soft_AY\n");
		ay_timer_enabled=true;
		if(cfg_sound_out_mode==OUT_PWM){
			printf("Set OUT PWM\n");
			PWM_init_pin(ZX_AY_PWM_PIN0);
			PWM_init_pin(ZX_AY_PWM_PIN1);
			PWM_init_pin(ZX_BEEP_PIN);
		}
		if(cfg_sound_out_mode==OUT_PCM){
			printf("Set OUT I2S\n");
			gpio_init(ZX_AY_PWM_PIN0);
			gpio_init(ZX_AY_PWM_PIN1);
			gpio_init(ZX_BEEP_PIN);
			gpio_set_dir(ZX_AY_PWM_PIN0,GPIO_OUT);
			gpio_set_dir(ZX_AY_PWM_PIN1,GPIO_OUT);
			gpio_set_dir(ZX_BEEP_PIN,GPIO_OUT);
			i2s_init();
		}
		////printf("Init PWM\n");
		//gpio_init(TST_PIN);
		//gpio_set_dir(TST_PIN,GPIO_OUT);
		//gpio_init(ZX_BEEP_PIN);
		
		//gpio_set_dir(ZX_BEEP_PIN,GPIO_OUT);
		////printf("Init BEEP\n");
		short int ayhz = AY_SAMPLE_RATE;
		if (!add_repeating_timer_us(-1000000 / ayhz, AY_timer_callback, NULL, &soft_sound_timer)) {
			////printf("Failed to add timer\n");
			return false;
		}
		////printf("Init TIMER\n");
	}
	return true;
}

void Deinit_Soft_AY(){
	if(ay_timer_enabled){
		////printf("DeInit Soft_AY\n");
		cancel_repeating_timer(&soft_sound_timer);
		if(cfg_sound_out_mode==OUT_PWM){
			PWM_Deinit_pin(ZX_AY_PWM_PIN0);
			PWM_Deinit_pin(ZX_AY_PWM_PIN1);
			PWM_Deinit_pin(ZX_BEEP_PIN);
		}
		//gpio_deinit(TST_PIN);
		if(cfg_sound_out_mode==OUT_PCM){
			i2s_deinit();
		}		
		ay_timer_enabled=false;
	}
}


/*
bool FAST_FUNC(trd50hz_callback)(repeating_timer_t *rt){
	if(trd_50hz_fired){
	}
	return true;
}
*/

uint32_t fire_millis(){
	return us_to_ms(time_us_32());
}

static uint32_t autofireX=0;
static uint32_t autofireY=0;
static uint32_t autofire=0;
static bool autofire_flipX=false;
static bool autofire_flipY=false;


void enter_pause(void){
	//old_show_hud = show_hud;
	//show_hud = false;
	show_slots=false;
	im_ready_loading=false;
	im_z80_stop = true;
	while (im_z80_stop){
		busy_wait_ms(10);
		if (im_ready_loading){
			busy_wait_ms(10);
			for (uint8_t i=0;i<16;i++){
				AY_select_reg(i);
				sound_reg_pause[i] = AY_get_reg(0);
				AY_set_reg(0);
			}
			sound_reg_pause[16]=beep_data_old;
			sound_reg_pause[17]=beep_data;
			//resetAY();
			AY_reset(cfg_sound_mode);
			break;
		}
	}
}

void resume_pause(void){
	AY_reset(cfg_sound_mode);
	for (uint8_t i=0;i<16;i++){
		AY_select_reg(i);
		AY_set_reg(sound_reg_pause[i]);
	}	
	beep_data_old=sound_reg_pause[16];
	beep_data = sound_reg_pause[17];
	im_z80_stop = false;
}

#ifndef DEBUG_DISABLE_LOADERS
void load_z80_file(char* file_name,short int slot){
	im_z80_stop = true;
	im_ready_loading = false;
	while (im_z80_stop){ //Quick Save
		busy_wait_ms(10);
		if (im_ready_loading){
			zx_machine_reset(false);
			AY_reset(cfg_sound_mode);// сбросить AY
			memset(temp_msg,0,sizeof(temp_msg));
			if(slot>0){
				sprintf(temp_msg," Loading slot# %d ",slot);
			} else {
				sprintf(temp_msg," Loading QUICKSAVE ");
			}
			if (load_image_z80(file_name)){
				MessageBox("LOAD",temp_msg,CL_WHITE,CL_BLUE,2);
			} else {
				MessageBox("Error!!!",temp_msg,CL_YELLOW,CL_LT_RED,1);
			}
			//AY_reset();// сбросить AY
			clear_input();
			im_z80_stop = false;
			im_ready_loading = false;
			break;
		}
	}
}

void save_z80_file(char* file_name,short int slot){
	im_ready_loading = false;
	im_z80_stop = true;
	while (im_z80_stop){
		busy_wait_ms(10);
		if (im_ready_loading){
			busy_wait_ms(10);
			short int file_descr = sd_mkdir("0:/save");
			if ((file_descr!=FR_OK)&&(file_descr!=FR_EXIST) ){
				memset(temp_msg,0,sizeof(temp_msg));
				sprintf(temp_msg," Error creating Save folder ");
				MessageBox("SAVE",temp_msg,CL_LT_YELLOW,CL_RED,1);
				break;	
			}
			memset(temp_msg,0,sizeof(temp_msg));
			if(slot>0){
				sprintf(temp_msg," Saving slot# %d ",slot);
			} else {
				sprintf(temp_msg," Saving QUICKSAVE ");
			}			
			if(save_image_z80(file_name)){
				MessageBox("SAVE",temp_msg,CL_WHITE,CL_BLUE,2);
			} else {
				sprintf(temp_msg," Saving %s ",file_name);
				MessageBox("Error!!!",temp_msg,CL_YELLOW,CL_LT_RED,1);
			}
			clear_input();
			im_z80_stop = false;					   
			im_ready_loading = false;
			break;
		}
	}
}
#endif

void get_saveslots(){
	memset(save_slots, 0, sizeof(save_slots));
	sprintf(save_file_name_image,"0:/save/QSAVE.Z80 ");
	fr = sd_open_file(&sd_file,save_file_name_image,FA_READ);
	if(fr==FR_OK){save_slots[0]=1;}
	sd_close_file(&sd_file);
	for(uint8_t i=1;i<11;i++){
		sprintf(save_file_name_image,"0:/save/__F%d.Z80 ",i);
		fr = sd_open_file(&sd_file,save_file_name_image,FA_READ);
		if(fr==FR_OK){save_slots[i]=1;}
		sd_close_file(&sd_file);
	}
}

bool tape_start(){
#ifndef DEBUG_DISABLE_LOADERS	
	if (tapeFileSize==0) {
		MessageBox("WARNING!!!","Tape file not loaded!",CL_CYAN,CL_LT_YELLOW,1);
		return false;
	} else {
		//printf("1>tap_loader_active:%02x \n",tap_loader_active);
		tap_block_percent = 0;
		//last_action = time_us_32();
		//clear_input();
		if(cfg_tap_load_mode==AUTO_LOAD_TAP){
			//printf("TAP autoplay ON\n");
			//printf("TAP Start\n");
			busy_wait_ms(150);
			clear_input();
			tap_loader_active |= TAPE_INTERNAL_AUTO;
			TapeStatus=TAPE_STOPPED;
			TAP_Play();
			return true;
		} else if(cfg_tap_load_mode==NORM_LOAD_TAP){
			//printf("TAP autoplay by Time\n");
			//printf("TAP Start\n");
			busy_wait_ms(150);
			clear_input();
			tap_loader_active |= TAPE_INTERNAL_MANU;
			TapeStatus=TAPE_STOPPED;
			TAP_Play();
			return true;
		} 
		//printf("2>tap_loader_active:%02x \n",tap_loader_active);
	}
	return false;
#endif
}

bool tape_stop(){
#ifndef DEBUG_DISABLE_LOADERS	
	if (tapeFileSize==0) {
		MessageBox("WARNING!!!","Tape file not loaded!",CL_CYAN,CL_LT_YELLOW,1);
		return false;
	} else {
		if(TapeStatus!=TAPE_STOPPED)
		MessageBox("TAPE","Stopped.",CL_GREEN,CL_WHITE,2);
		////printf("TAP Stop\n");
		TapeStatus=TAPE_STOPPED;
		////printf("tape_autoload_status:%d \n",tape_autoload_status);
		tap_loader_active = TAPE_OFF;
		busy_wait_ms(150);
		clear_input();
		return true;
	}
	return false;
#endif
}

bool tape_rewind(){
#ifndef DEBUG_DISABLE_LOADERS
	if (tapeFileSize==0) {
		MessageBox("WARNING!!!","Tape file not loaded!",CL_CYAN,CL_LT_YELLOW,1);
		return false;
	} else {
		TAP_Rewind();
		MessageBox("TAPE","Rewind.",CL_GREEN,CL_WHITE,2);
		tap_block_percent = 0;
		tap_loader_active = TAPE_OFF;
		busy_wait_ms(150);
		clear_input();
		return true;
	}
	return false;
#endif
}

bool tape_prev_block(){
#ifndef DEBUG_DISABLE_LOADERS
	if (tapeFileSize==0) {
		MessageBox("WARNING!!!","Tape file not loaded!",CL_CYAN,CL_LT_YELLOW,1);
		return false;
	} else {
		if(TAP_PrevBlock()){
			//MessageBox("TAPE","Previous block.",CL_GREEN,CL_WHITE,2);
			tap_block_percent = round((tapeTotByteCount*100)/tapeFileSize);
			tap_loader_active = TAPE_OFF;
			busy_wait_ms(150);
			clear_input();
			return true;
		}
		return false;
	}
	return false;
#endif
}

bool tape_next_block(){
#ifndef DEBUG_DISABLE_LOADERS
	if (tapeFileSize==0) {
		MessageBox("WARNING!!!","Tape file not loaded!",CL_CYAN,CL_LT_YELLOW,1);
	} else {
		if(TAP_NextBlock()){
			//MessageBox("TAPE","Next block.",CL_GREEN,CL_WHITE,2);
			tap_block_percent = round((tapeTotByteCount*100)/tapeFileSize);
			tap_loader_active = TAPE_OFF;
			busy_wait_ms(150);
			clear_input();
			return true;
		}
		return false;
	}
	return false;
#endif
}

bool tape_eject(){
#ifndef DEBUG_DISABLE_LOADERS
	if (tapeFileSize==0) {
		MessageBox("WARNING!!!","Tape file not loaded!",CL_CYAN,CL_LT_YELLOW,1);
		return false;
	} else {
		MessageBox("TAPE","Eject",CL_GREEN,CL_WHITE,2);
		tap_block_percent = 0;
		tap_loader_active = TAPE_OFF;
		TAP_Eject();
		tape_disp = 0;
		busy_wait_ms(150);
		clear_input();
		return true;
	}
	return false;
#endif
}

void input_init(){
	printf("Joysticks Init:\n");

	active_type_joystick = joy_start();            // инициализация джойстиков	
	if(Joystics.joy_connected) {
		joy_connected = true;
		if(Joystics.Present_i2c_PCF_NES_joy1){printf("PCF_NES_joy1\t");}
		if(Joystics.Present_i2c_PCF_NES_joy2){printf("PCF_NES_joy2\t");}
		if(Joystics.Present_NES_joy1){printf("NES_joy1\t");}
		if(Joystics.Present_NES_joy2){printf("NES_joy2\t");}
		printf(" Connected \n");
	}else{
		joy_connected = false;
	}
#ifndef MURM2	
	if (i2c_kbd_start()){i2cKbdMode = true;}else{i2cKbdMode = false;}

	short int i2c_state = i2c_kbd_data_in();
	//printf ("i2c_state: %d\n",i2c_state);
#endif
	if(!i2cKbdMode) {
#ifndef MURM2	
		//printf ("i2c Keyboard not connected\n");
		i2c_kbd_deinit();
#endif
		busy_wait_ms(100);
		start_PS2_capture();
		printf ("PS/2 Keyboard Started\n");
	} else {
		printf ("I2C Keyboard Started\n");
	}	
}

uint8_t hat_switch_process(uint16_t data_joy){
	uint8_t result=0;
	if(!kbd_lock){
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_UP)){result|=HAT_UP;result|=HAT_START;}
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_DOWN)){result|=HAT_DOWN;result|=HAT_START;}
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_LEFT)){result|=HAT_LEFT;result|=HAT_START;}
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_RIGHT)){result|=HAT_RIGHT;result|=HAT_START;}
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_A)){result|=HAT_A;result|=HAT_START;}
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_B)){result|=HAT_B;result|=HAT_START;}
		if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_UP)){result|=HAT_UP;result|=HAT_SELECT;}
		if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_DOWN)){result|=HAT_DOWN;result|=HAT_SELECT;}
		if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_LEFT)){result|=HAT_LEFT;result|=HAT_SELECT;}
		if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_RIGHT)){result|=HAT_RIGHT;result|=HAT_SELECT;}
	}
	if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_A)){result|=HAT_A;result|=HAT_SELECT;}
	if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_B)){result|=HAT_B;result|=HAT_SELECT;}
	return result;
}

void get_battery_stats(){
	monitor_battery_voltage();
	battery_status = round(battery_power_percent/10);
	if(battery_status>10)battery_status=10;
	if (battery_power_charge = batt_management_usb_power_detected()){
		battery_status_ico++;
		if(battery_status_ico>3){battery_status_ico=1;};
		battery_status|=battery_status_ico<<4;
	} else {
		battery_status&=~0xF0;
	}
	//printf("bs:[%02X]\tbpp:%d\tbpc:%d\n",battery_status,battery_power_percent,battery_power_charge);

	/*battery_status = battery_power_percent;
	if(battery_status>100)battery_status=100;
	memset(batt_text,0,sizeof(batt_text));
	sprintf(batt_text," %03d%%",battery_status);
	if(batt_management_usb_power_detected()){batt_text[0]=0x18;};
	*/
}

FileRec* file=NULL;

uint32_t timer_update =0;
uint32_t main_loop =0;
uint32_t main_loop1 =0;

uint8_t help_lng;

/* HAPPY NEW YEAR 2025
int16_t sin16_C( uint16_t theta ){
	static const uint16_t base[] =
	{ 0, 6393, 12539, 18204, 23170, 27245, 30273, 32137 };
	static const uint8_t slope[] =
	{ 49, 48, 44, 38, 31, 23, 14, 4 };

	uint16_t offset = (theta & 0x3FFF) >> 3; // 0..2047
	if( theta & 0x4000 ) offset = 2047 - offset;

	uint8_t section = offset / 256; // 0..7
	uint16_t b   = base[section];
	uint8_t  m   = slope[section];

	uint8_t secoffset8 = (uint8_t)(offset) / 2;

	uint16_t mx = m * secoffset8;
	int16_t  y  = mx + b;

	if( theta & 0x8000 ) y = -y;

	return y;
}
*/
/* HAPPY NEW YEAR 2025
#define FLAKE_SIZE (20)
typedef struct flakea{
   int x;
   int y;
   uint8_t r;
} __attribute__((packed)) flakeb;
flakeb flakes[FLAKE_SIZE];

void draw_flake(int x,int y, uint8_t size){
	if((x<SCREEN_W)&&(y<SCREEN_H)){
		if(size==1){
			draw_pixel(x,y,CL_WHITE);
		}
		if(size==2){
			draw_pixel(x,y,CL_WHITE);
			draw_pixel(x+1,y,CL_WHITE);
			draw_pixel(x,y+1,CL_WHITE);
			draw_pixel(x+1,y+1,CL_WHITE);
		}
	}
}


volatile uint16_t flake_angle;

void update_flakes(){
  flake_angle += 1;
  for(uint8_t i = 0; i < FLAKE_SIZE; i++){
	flakes[i].x = (get_rand_32()>>23);
	flakes[i].y = (get_rand_32()>>24);
    //flakes[i].y += (sin16_C(flake_angle+flakes[i].r)) + 1 + flakes[i].r/2;
    //flakes[i].x += sin16_C(flake_angle)*2;

    //Sending flakes back from the top when it exits
    //Lets make it a bit more organic and let flakes enter from the left and right also.
    /*if(flakes[i].x > SCREEN_W+5 || flakes[i].x < -5 || flakes[i].y > SCREEN_H){
      if(i%3 > 0) //66.67% of the flakes
      {
		flakes[i].x = (get_rand_32()>>23);
		flakes[i].y = -10;
      }
      else
      {
        //If the flake is exitting from the right
        if(sin16_C(flake_angle) > 1024){
          //Enter from the left
		  flakes[i].x = -5;
		  flakes[i].y = (get_rand_32()>>24);
        }
        else
        {
		  flakes[i].x = SCREEN_W+5;
		  flakes[i].y = (get_rand_32()>>24);
        }
      }
    }/
  }
}

void draw_flakes(){
  for(uint8_t i = 0; i < FLAKE_SIZE; i++){
	draw_flake(flakes[i].x,flakes[i].y, flakes[i].r);
  }
  update_flakes();
}
*/

#ifndef PICO_RP2040
#include <hardware/regs/qmi.h>
#include <hardware/structs/qmi.h>
void __not_in_flash_func(flash_timings)(int mhz) {
        const int max_flash_freq = 66 * MHZ;
        const int clock_hz = mhz * MHZ;
        int divisor = (clock_hz + max_flash_freq - 1) / max_flash_freq;
        if (divisor == 1 && clock_hz > 100000000) {
            divisor = 2;
        }
        int rxdelay = divisor;
        if (clock_hz / divisor > 100000000) {
            rxdelay += 1;
        }
        qmi_hw->m[0].timing = 0x60007000 |
                            rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
                            divisor << QMI_M0_TIMING_CLKDIV_LSB;
}
#endif

int main(void){
	//vreg_set_voltage(VREG_VOLTAGE_1_20);//def
	vreg_set_voltage(VREG_VOLTAGE_1_30);
	//vreg_set_voltage(VREG_VOLTAGE_1_25);
	busy_wait_ms(100);
#if !PICO_RP2040
	#ifdef VGA_HDMI
	    flash_timings(315);
	#else
		#if COMPOSITE_TV||SOFT_COMPOSITE_TV
			flash_timings(378);
		#endif
	#endif
#endif
	/*
		вот эти пробуй:280, 288, 290400, 296, 297, 300
		дальше такие:302400, 303, 304800, 306, 307, 308, 309600, 312, 314400, 315, 316, 316800
	*/
	
	//set_sys_clock_khz(300000, true);
	//set_sys_clock_khz(284000, false);
	//set_sys_clock_khz(290400, false);
	//set_sys_clock_khz(504000, false);
	//set_sys_clock_khz(252000, false);//def
	//set_sys_clock_khz(297600, false);
	/*
	#if VIDEO_VGA
	//set_sys_clock_khz(378000, true);
	set_sys_clock_khz(315000, false);
	#endif
	*/
	#ifdef VGA_HDMI
		set_sys_clock_khz(315000, false);
	#endif
	#if COMPOSITE_TV||SOFT_COMPOSITE_TV
		set_sys_clock_khz(378000, false);
		//set_sys_clock_khz(360000, false);
	#endif
	
	busy_wait_ms(10);
	
	stdio_init_all();

	busy_wait_ms(100);

	gpio_init(WORK_LED_PIN);
	gpio_set_dir(WORK_LED_PIN,GPIO_OUT);

	#ifdef DEBUG_DELAY
	while (!stdio_usb_connected()) {
		gpio_put(WORK_LED_PIN, 1);
		busy_wait_ms(50);
		gpio_put(WORK_LED_PIN, 0);
		busy_wait_ms(500);
	}
	busy_wait_ms(500);
	#endif
	
	printf("MurMulator %s by %s \n\n",FW_VERSION,FW_AUTHOR);
	printf("Main Program Start!!!\n");
	printf("CPU clock=%ld Hz\n",(long)clock_get_hz(clk_sys));
	
	//пин ввода звука
	
	//пины вывода звука
	//PWM_init_pin(ZX_AY_PWM_PIN0);
	//PWM_init_pin(ZX_AY_PWM_PIN1);
	
	
	//gpio_init(TST_PIN);
	//gpio_set_dir(TST_PIN,GPIO_OUT);
	
	//gpio_init(ZX_BEEP_PIN);
	//gpio_set_dir(ZX_BEEP_PIN,GPIO_OUT);
	#ifndef DEBUG_DISABLE_LOADERS
	tap_loader_active = TAPE_OFF;
	#endif

	printf("init FileSystem\n");
	//int fs=vfs_init();
	init_fs=init_filesystem();
	printf("init FS:%d\n",init_fs);
	
	
	printf("Load config\n");
	settings_index = 0;
	settings_lines = 0;
	for(uint8_t i=0;i<CONFIG_MENU_ITEMS;i++){
		if(*config_menu[i]==0){
			settings_lines=i-1;
			break;
		}
	}	
	if(init_fs){
		if(!config_read()){
			if(!config_write_defaults()){
				printf("Error saving %s\n",CONFIG_FILENAME);
			}
		} /*else {
			cfg_sound_mode=3;
		}*/
		if(cfg_boot_scr>MAX_CFG_BOOT_SCR_MODE){
			cfg_boot_scr=DEF_CFG_BOOT_SCR_MODE;
		}		
		if(cfg_hud_enable>MAX_CFG_HUD_MODE){
			cfg_hud_enable=MAX_CFG_HUD_MODE;
		}		
		if(cfg_tap_load_mode>MAX_CFG_TAP_MODE){
			cfg_tap_load_mode=DEF_CFG_LOAD_TAP;
		}
		if(cfg_def_joy1_mode>MAX_CFG_JOY_MODE){
			cfg_def_joy1_mode=DEF_CFG_JOY_MODE;
		}
		if(cfg_def_joy2_mode>MAX_CFG_JOY_MODE){
			cfg_def_joy2_mode=DEF_CFG_JOY_MODE;
		}
		if(cfg_def_kbd_mode>MAX_CFG_KBD_MODE){
			cfg_def_kbd_mode=DEF_CFG_KBD_MODE;
		}
		if(cfg_res_before_mode>MAX_CFG_RES_MODE){
			cfg_res_before_mode=DEF_CFG_RES_MODE;
		}
		if(cfg_sound_mode>MAX_CFG_SND_MODE){
			cfg_sound_mode=DEF_CFG_SND_MODE;
		}
		if(cfg_sound_out_mode>MAX_CFG_OUT_MODE){
			cfg_sound_out_mode=DEF_CFG_OUT_MODE;
		}
		if(cfg_volume>MAX_CFG_VOLUME_MODE){
			cfg_volume=DEF_CFG_VOLUME_MODE;
		}
		if(cfg_tspin_mode>MAX_CFG_TSPIN_MODE){
			cfg_tspin_mode=DEF_CFG_TSPIN_MODE;
		}
		if(cfg_tschip_order>MAX_CFG_TSORDER_MODE){
			cfg_tschip_order=DEF_CFG_TSORDER_MODE;
		}
		if(cfg_video_out>MAX_CFG_VIDEO_MODE){
			cfg_video_out=DEF_CFG_VIDEO_MODE;
		}
		if(cfg_frame_rate>MAX_CFG_VIDEO_FREQ_MODE){
			cfg_frame_rate=DEF_CFG_VIDEO_FREQ_MODE;
		}
		if(cfg_mobile_mode>MAX_CFG_MOBILE_MODE){
			cfg_mobile_mode=DEF_CFG_MOBILE_MODE;
		}	
		if((cfg_lcd_video_out>MAX_CFG_LCD_VIDEO_MODE)||(cfg_lcd_video_out<MIN_CFG_LCD_VIDEO_MODE)){
			cfg_lcd_video_out=DEF_CFG_LCD_VIDEO_MODE;
		}
		if(cfg_brightness>MAX_CFG_BRIGHT_MODE){
			cfg_brightness=DEF_CFG_BRIGHT_MODE;
		}
		if(cfg_rotate>MAX_CFG_ROTATE){
			cfg_rotate=DEF_CFG_ROTATE;
		}	
		if(cfg_inversion>MAX_CFG_INVERSION){
			cfg_inversion=DEF_CFG_INVERSION;
		}
		if(cfg_pixels>MAX_CFG_PIXELS){
			cfg_pixels=DEF_CFG_PIXELS;
		}
		if(cfg_tape_load_pin>MAX_CFG_TAPELOAD_PIN){
			cfg_tape_load_pin=DEF_CFG_TAPELOAD_PIN;
		}		
	}  
	if(!init_fs){
		cfg_sound_mode=3;
		cfg_sound_out_mode=OUT_PCM;//DEF_CFG_OUT_MODE;//OUT_PCM;
		cfg_volume=DEF_CFG_VOLUME_MODE;
		cfg_lcd_video_out=DEF_CFG_LCD_VIDEO_MODE;
		cfg_brightness=DEF_CFG_BRIGHT_MODE;
		cfg_rotate=DEF_CFG_ROTATE;
		cfg_pixels=DEF_CFG_PIXELS;
		cfg_tape_load_pin=DEF_CFG_TAPELOAD_PIN;
		cfg_hud_enable=DEF_CFG_HUD_MODE;
		cfg_mobile_mode=DEF_CFG_MOBILE_MODE;
	}

	
	//printf("*Settings read: %s\n",video_out_config[cfg_lcd_video_out]);

	current_frame_rate	= 	cfg_frame_rate;
	current_video_out	= 	cfg_video_out;
	current_rotate		=	cfg_rotate;
	current_inversion	=	cfg_inversion;
	current_pixels		=	cfg_pixels;
	current_pin			=   cfg_tape_load_pin;
	current_sound_out	=	cfg_sound_out_mode;
	current_hud_mode	=	cfg_hud_enable;
	current_mobile_mode	=	cfg_mobile_mode;


	tape_load_pin = load_pins[cfg_tape_load_pin];
	printf("Init Tape In at GPIO:%d\n",tape_load_pin);
	inInit(tape_load_pin);

	//cfg_sound_mode=0;
	#ifdef DEBUG_DELAY
		printf("Real config:\n");
		printf("  + cfg_tap_load_mode:%d\n", cfg_tap_load_mode);
		printf("  + cfg_def_joy1_mode:%d\n", cfg_def_joy1_mode);
		printf("  + cfg_def_joy2_mode:%d\n", cfg_def_joy2_mode);
		printf("  + cfg_def_kbd_mode:%d\n", cfg_def_kbd_mode);
		printf("  + cfg_res_before_mode:%d\n", cfg_res_before_mode);
		printf("  + cfg_sound_mode:%d\n", cfg_sound_mode);
		printf("  + cfg_sound_out_mode:%d\n", cfg_sound_out_mode);
		printf("  + cfg_hud_enable:%d\n", cfg_hud_enable);
		printf("  + cfg_frame_rate:%d\n", cfg_frame_rate);
	#endif
	
	// ----------Init Sound------------
	ay_timer_enabled=false;
	ts_595_enabled=false;
	printf("Init Sound - ");
	if(cfg_sound_mode == HARDWARE_TS){
		Deinit_Soft_AY();
		Init_PWM_175(cfg_tspin_mode);				//--------------------------для ногодрыга 595
		printf("HardAY\n");
	} else {
		//пины вывода звука
		Deinit_PWM_175(cfg_tspin_mode);
		//printf("595 - disabled\n");
		if(Init_Soft_AY()){
			printf("SoftAY - OK\n");
		} else {
			printf("SoftAY - FAIL\n");
		}
		//printf("Soft - enabled\n");
	}
	printf("AY Reset\n");
	busy_wait_ms(50);
	AY_reset(cfg_sound_mode);
	//printf("Sound out:%s\n",sound_out_config[cfg_sound_out_mode]);

	printf("Init flash timer\n");
	repeating_timer_t zx_flash_timer;
	short int hz=2;
	if (!add_repeating_timer_us(-1000000 / hz, zx_flash_callback, NULL, &zx_flash_timer)) {
		//printf("Failed to add zx flash timer\n");
		return 1;
	}

	printf("Read root folder\n");
	if (init_fs){
		N_files = read_select_dir(cur_dir_index);
		if(N_files==0){
			for (uint8_t i = 0; i < 5; i++){
				N_files = read_select_dir(cur_dir_index);
				if(N_files>0) break;
				printf("Read dir fail - retry\n");	
				busy_wait_ms(250);
			}
		}
		printf("Init FS Success:%d files\n",N_files);
		//N_files = read_select_dir(cur_dir_index);
		//printf("DIR:%s cur_dir_index:%d\n",dir_path,cur_dir_index);
		//printf("init_fs>N_files:%d\n",N_files);
	}  else printf("Init FS Fail\n");

	printf("Init graphics buffer\n");

	init_screen(graph_buf,SCREEN_W,SCREEN_H);

	bool (*handler_ptr)(short);

	/*
	for(uint8_t idx;idx<10;idx++){
		hud[idx].active=false;
	}
	*/
	printf("Begin video init: ");
	#ifdef VGA_HDMI
		/*busy_wait_ms(100);
		startVGA(cfg_frame_rate);
		printf("VGA Started\n");*/
		busy_wait_ms(100);
		uint8_t c_video=cfg_video_out;
		if((g_out)c_video==g_out_AUTO){
			printf("Autodetect:");
			c_video = (uint8_t)graphics_test_output();
			if((g_out)c_video>g_out_HDMI){
				c_video=cfg_lcd_video_out;
			}
			printf(" - found %s\n",video_out_config[c_video]);
			printf("Settings:%s@%s\n",video_out_config[c_video],video_freq_config[cfg_frame_rate]);
			//cfg_video_out=c_video;
		} else {
			//c_video=cfg_video_out;
			printf("Settings:%s@%s\n",video_out_config[cfg_video_out],video_freq_config[cfg_frame_rate]);
			
		}

		graphics_set_mode(g_mode_320x240x4bpp);
		if((g_out)c_video>g_out_HDMI){
			printf("LCD Params init:");
			graphics_set_rotate((rotate)cfg_rotate);
			graphics_set_inversion((bool)cfg_inversion);
			graphics_set_pixels(cfg_pixels);
			printf(" %s %s %s\n",video_out_rotate[cfg_rotate],video_out_inversion[cfg_inversion],video_out_pixels[cfg_pixels]);
		}

		graphics_set_buffer(graph_buf);

		graphics_set_hud_buffer(hud_line);
		memset(hud_line,0x11,SCREEN_W);
		graphics_set_hud_handler(NULL);

		//graphics_set_textbuffer(txt_buf,txt_buf_color);

		graphics_init((g_out)c_video,(fr_rate)cfg_frame_rate);

		if((g_out)c_video<g_out_TFT_ST7789){
			for(int i=0;i<16;i++){
				int I=(i>>3)&1;
				int G=(i>>2)&1;
				int R=(i>>1)&1;
				int B=(i>>0)&1;
				uint32_t RGB=((R?I?255:170:0)<<16)|((G?I?255:170:0)<<8)|((B?I?255:170:0)<<0);
				//printf("VGA> %d>[%08lX]\n",i,RGB);
				graphics_set_palette(i,RGB);
			}
		}
		if((g_out)c_video>g_out_HDMI){
			printf("LCD Palette init\n");
			for(int i=0;i<16;i++){
				int I=(i>>3)&1;
				int G=(i>>2)&1;
				int R=(i>>1)&1;
				int B=(i>>0)&1;
				//uint32_t RGB=((R?I?255:170:0)<<16)|((G?I?255:170:0)<<8)|((B?I?255:170:0)<<0);
				//uint32_t RGB=((R?I?31:20:0)<<11)|((G?I?63:41:0)<<5)|((B?I?31:20:0)<<0);
				//uint32_t RGB=((R?I?31:27:0)<<11)|((G?I?63:57:0)<<5)|((B?I?31:27:0)<<0);
				uint32_t RGB=((R?I?31:17:0)<<11)|((G?I?63:33:0)<<5)|((B?I?31:17:0)<<0);
				//printf("TFT> %d>[%08lX]\n",i,RGB);
				graphics_set_palette(i,RGB);
			}	
			gpio_set_function(TFT_LED_PIN, GPIO_FUNC_PWM);
			uint slice_num = pwm_gpio_to_slice_num(TFT_LED_PIN);		
			pwm_config led_pwm=pwm_get_default_config();
			pwm_config_set_clkdiv(&led_pwm,1.0);
			pwm_config_set_wrap(&led_pwm,256);//MAX PWM value
			pwm_init(slice_num,&led_pwm,true);
			pwm_set_gpio_level(TFT_LED_PIN,(TFT_MIN_BRIGHTNESS+(cfg_brightness*10)));			//уровень подсветки TFT
			printf("LCD Level:%d\n",cfg_brightness);
		}		

		//startVGA(cfg_frame_rate);
		printf("Video Out Started\n");
	#endif

	#ifdef COMPOSITE_TV
		graphics_set_buffer(graph_buf);
		graphics_set_hud_buffer(hud_line);
		memset(hud_line, 0x11, SCREEN_W);
		graphics_set_mode(g_mode_320x240x4bpp);
		graphics_init(cfg_video_out == g_out_PAL ? g_TV_OUT_PAL : g_TV_OUT_NTSC);
		for(int i=0;i<16;i++){
			int I=(i>>3)&1;
			int G=(i>>2)&1;
			int R=(i>>1)&1;
			int B=(i>>0)&1;
			uint32_t RGB=((R?I?255:170:0)<<16)|((G?I?255:170:0)<<8)|((B?I?255:170:0)<<0);
			graphics_set_palette(i,RGB);
		}
	#endif
	#ifdef SOFT_COMPOSITE_TV
		graphics_set_buffer(graph_buf);
		#ifndef SOFT_CVBS_NO_HUD
		graphics_set_hud_buffer(hud_line);
		memset(hud_line,0x11,SCREEN_W);
		graphics_set_hud_handler(&hud_prepare);
		#endif
		graphics_init();
		tv_out_mode_t g_mode=graphics_get_default_mode((g_out)cfg_video_out);
		printf("Found %s\n",video_out_config[cfg_video_out]);
		graphics_set_mode(g_mode);
		for(int i=0;i<16;i++){
			int I=(i>>3)&1;
			int G=(i>>2)&1;
			int R=(i>>1)&1;
			int B=(i>>0)&1;
			uint32_t RGB=((R?I?255:170:0)<<16)|((G?I?255:170:0)<<8)|((B?I?255:170:0)<<0);
			// uint32_t RGB=((R?I?255:200:0)<<16)|((G?I?255:200:0)<<8)|((B?I?255:200:0)<<0);
			graphics_set_palette(i,RGB);
		}
		printf("Soft Composite Started\n");
	#endif

	hud_timer=0;
	hud_ptr = NULL;
	current_hud_mode=cfg_hud_enable;
	old_hud_mode=current_hud_mode;

	#ifdef VGA_HDMI
		if(cfg_mobile_mode==MOBILE_MURM_ON){
			hud_ptr = &hud_battery;
			current_hud_mode|=HM_SHOW_BATTERY;
			battery_status = 0;
		}	
		//-------Init Battery Check--------
		if(current_hud_mode&HM_SHOW_BATTERY){
			printf("Battery monitor init\n");
			if(batt_management_init()){
				printf("Battery monitor started\n");
			}
			busy_wait_ms(100);
			get_battery_stats();
			//monitor_battery_voltage();
			//monitor_battery_voltage();
		}
		//-------Init Battery Check--------	
	#endif

	printf("Reset Kempston mouse\n");
	reset_kmouse();
	now_joy1_mode = cfg_def_joy1_mode;
	now_joy2_mode = cfg_def_joy2_mode;
	now_kbd_mode  = cfg_def_kbd_mode;

	input_init();

	//multicore_launch_core1( start_PS2_capture);
	
	//ZXThread();
	void (*ZXThread_ptr)();
	ZXThread_ptr = ZXThread;
	multicore_launch_core1(ZXThread_ptr);
	busy_wait_ms(50);

	
	//printf("HM0>[%04X]\n",current_hud_mode);

	//convert_kb_u_to_kb_zx(&kb_st_ps2,zx_write_buffer->kb_data);
	
	//inInit(PIN_ZX_LOAD);
	
	cur_dir_index=0;
	
	for(uint8_t i=0;i<DIRS_DEPTH;i++){
		cursor_index_old[i]=-1;
	}
	
	
	menu_ptr=0;
	menu_mode[menu_ptr]=EMULATION;
	menu_ptr++;
	switch (cfg_boot_scr){
		case BOOT_SCR_LOGO: menu_mode[menu_ptr]=BOOT_LOGO; break;
		case BOOT_SCR_FILE_MAN: menu_mode[menu_ptr]=MENU_MAIN; break;
		case BOOT_SCR_EMULATION: menu_mode[menu_ptr]=EMULATION; break;
		default: menu_mode[menu_ptr]=BOOT_LOGO; break;
	}
	
	//TRDOS_disabled=true;
	main_loop = 0;
	int32_t ticker = 0;

	/* HNY 2025
	for(uint8_t i = 0; i < FLAKE_SIZE; i++){
		flakes[i].x = (get_rand_32()>>23);
		flakes[i].y = (get_rand_32()>>24);
		flakes[i].r = (get_rand_32()>>30);
  	}
	*/


	/*
	graphics_set_hud_handler(NULL);
	#ifndef SOFT_CVBS_NO_HUD
	graphics_set_hud_handler(&hud_prepare);
	graphics_set_hud_handler(&hud_prepare_kbd);
	graphics_set_hud_handler(&hud_prepare_scale);
	#ifndef DEBUG_DISABLE_LOADERS
	graphics_set_hud_handler(&hud_prepare_tape);
	#endif
	graphics_set_hud_handler(NULL);
	#endif
	*/

	//printf("[%08X][%08X][%08X]\n",help_ptr,help_text_rus,help_text_eng);
	help_lng=0;
	//printf("[%08X][%08X][%08X]\n",help_ptr,help_text_rus,help_text_eng);
	printf("starting main loop %d>[%02X]\n",menu_ptr,menu_mode[menu_ptr]);
	do{ //BEGIN GLOBAL LOOP
		graphics_set_hud_handler(hud_ptr);
		//BOOT SCREEN
		if(menu_mode[menu_ptr]==BOOT_LOGO){
			
			//boot logo
			zx_machine_enable_vbuf(false);
			draw_rect(0,0,SCREEN_W,SCREEN_H,COLOR_FULLSCREEN,true);//Заливаем экран 
			draw_mur_logo_big(SCREEN_W/2-69,SCREEN_H/2-75,2);
			animation_action=my_millis();
			animation_frame=0;
			ticker=0;
			uint8_t repeat=0;
			printf("Enter Boot Screen Mode\n");
			while(true){
				while(!wait_kbdjoy_emu()){
					#ifdef VGA_HDMI
					//graphics_update_screen();
					#endif
					ticker++;
					/*if ((ticker%2048)==0){
						//i2s_out((int)4096,(int)4096);
					}
					if ((ticker%4096)==0){
						//i2s_out((int)0,(int)0);
					}*/

					if ((ticker%8192)==0){
						if(current_hud_mode&HM_SHOW_BATTERY){
							get_battery_stats();
						}

					}
					/* HNY 2025
					if(repeat==1){
						draw_flakes();
						if((ticker%16384)==0){
							repeat=2;
							printf("repeat:%d\n",repeat);
						}
						continue;
					}						
					if(repeat==2){
						repeat=0;
						printf("repeat:%d\n",repeat);
						draw_rect(0,0,SCREEN_W,SCREEN_H,COLOR_FULLSCREEN,true);//Заливаем экран 
						draw_mur_logo_big(SCREEN_W/2-69,SCREEN_H/2-75,2);
					}
					*/
					//busy_wait_ms(50);
					if(my_millis()>(animation_action+(mur_logo_animation[animation_frame][0]*10))){
						animation_action=my_millis();
						//if(repeat<1) 
							//draw_mur_logo_anim(SCREEN_W/2-69,SCREEN_H/2-75,mur_logo_animation[animation_frame][1],mur_logo_animation[animation_frame][2],mur_logo_animation[animation_frame][3]);
							draw_mur_logo_anim(SCREEN_W/2-69,SCREEN_H/2-75,mur_logo_animation[animation_frame][1],mur_logo_animation[animation_frame][2],0);
						animation_frame++;
						
						if(animation_frame>=MAX_ANIM_FRAME){
							animation_frame=0;
							/* HNY 2025
							repeat++;
							printf("repeat:%d\n",repeat);
							*/
						};
						
					}
					if(ticker>131070){ticker=0;}
				}
				if((KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
					menu_mode[menu_ptr]=MENU_MAIN;
					last_action = 0;
					clear_input();
					busy_wait_ms(100);
					break;
				}
				if (KBD_F1) {
					menu_mode[menu_ptr]=MENU_HELP;
					need_redraw=true;
					clear_input();
					busy_wait_ms(100);
					break;
				}
				if(((data_joy&D_JOY_START)||(data_joy_2&D_JOY_START))||((((KBD_L_CTRL)||(KBD_R_CTRL)))&&(KBD_F11))){
					menu_mode[menu_ptr]=MENU_JOY_MAIN;
					clear_input();
					busy_wait_ms(100);
					break;
				}
				if((KBD_PRESS)||(joy_pressed)){
					menu_ptr--;
					clear_input();
					busy_wait_ms(100);
					break;
				}
			}
			//convert_kb_u_to_kb_zx(&kb_st_ps2,zx_write_buffer->kb_data);
			joy_pressed=false;
		}
		//BOOT SCREEN
		//BEGIN MENU LOOP
		//printf("1 menu_ptr:%d menu_mode:%d\n",menu_ptr,menu_mode[menu_ptr]);
		if(menu_mode[menu_ptr]<EMULATION){
			old_hud_mode=current_hud_mode;
			current_hud_mode&=~HM_SHOW_VOLUME;
			current_hud_mode&=~HM_SHOW_BRIGHT;
			current_hud_mode&=~HM_MAIN_HUD;
			current_hud_mode&=~HM_TAPE_HUD;
			current_hud_mode&=~HM_KBD_HUD;
			if(current_hud_mode&HM_SHOW_BATTERY){
				handler_ptr=&hud_battery;
				graphics_set_hud_handler(handler_ptr);
			} else {
				graphics_set_hud_handler(NULL);
			};
			zx_machine_enable_vbuf(false);
			is_new_screen=true;
			printf("Enter Menu Mode\n");
			ticker=0;
			do{// main menu loop
				#ifdef VGA_HDMI
				//graphics_update_screen();
				#endif
				ticker++;
				if ((ticker%32700)==0){
					if(current_hud_mode&HM_SHOW_BATTERY){
						get_battery_stats();
					}
				}				
				if((us_to_ms(time_us_32())-timer_update)>TIMER_PERIOD){
					flip_led^=1;
					if(!flip_led){
						timer_update=us_to_ms(time_us_32())+2500;
						gpio_put(WORK_LED_PIN,1);
					} else {
						timer_update=us_to_ms(time_us_32())+5000;
						gpio_put(WORK_LED_PIN,0);
					}
					// printf("Time is:%d \n",my_millis());
					//sleep_ms(50);
					//gpio_put(WORK_LED_PIN,0);
					//busy_wait_ms(50);
				};			
				if(menu_mode[menu_ptr]==EMULATION){break;}
				process_input();
				if((data_joy>>16)>0){
					data_joy=(data_joy>>16);
				}
				
				//printf("data_joy:[%08X]\n",data_joy);
				/*if(((Joystics.Present_WII_joy)||(Joystics.Present_i2c_PCF_16_buttons))&&(data_joy!=0)){
					busy_wait_ms(150); //WII joystick delay
				}else*/
				if ((KBD_PRESS)||(joy_pressed)){ //like a speccy keypress sound
					hw_zx_set_beep_out(0x00);
					busy_wait_us(3500);
					hw_zx_set_beep_out(0x08);
					busy_wait_us(3500);
					hw_zx_set_beep_out(0x00);
					/*
					busy_wait_ms(3);
					hw_zx_set_beep_out(0x08);
					busy_wait_ms(3);
					hw_zx_set_beep_out(0x00);
					*/
				}

				if(data_joy!=0){
					busy_wait_ms(110); //joystick delay
				}						
				
				//busy_wait_us(500);
				/*--HARD reset--*/
				if (((KBD_L_SHIFT)||(KBD_R_SHIFT))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_DELETE)){
					software_reset();
				}
				/*--HARD reset--*/
				/*--DEF reset--*/
				if (((KBD_L_CTRL)||(KBD_R_CTRL))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_BACK_SPACE)){ 
					config_write_defaults();
					busy_wait_ms(100);
					software_reset();
				}
				/*--DEF reset--*/				
				/*--HELP--*/
				if ((KBD_F1)&&(menu_mode[menu_ptr]!=MENU_HELP)){
					menu_ptr++;
					menu_mode[menu_ptr]=MENU_HELP;
					is_new_screen=true;
					need_redraw=true;
					last_action=0;
				}
				/*--HELP--*/				
				//Controllers speed fix, joy is faster than keyboard
				/*if((WII_Init)&&(data_joy!=0)){
					busy_wait_ms(130); //WII joystick delay
				}else
				if(data_joy!=0){
					busy_wait_ms(100); //joystick delay
				} else {
					busy_wait_ms(50);
				}*/
				//Controllers speed fix, joy is faster than keyboard
				////printf("kbd[0][%08X]   kbd[1][%08X]   kbd[2][%08X]   kbd[2][%08X]\n",kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3]);
				/*if((data_joy!=0)||(KBD_PRESS)){
					//printf("menu_mode[menu_ptr]>%d   menu_ptr>%d   fast_mode[fast_mode_ptr]>%d   fast_mode_ptr>%d   fast_menu_index>%d   old_menu_index>%d\n",menu_mode[menu_ptr],menu_ptr,fast_mode[fast_mode_ptr],fast_mode_ptr,fast_menu_index,old_menu_index);
				}*/
				//printf("4 menu_ptr:%d menu_mode:%d\n",menu_ptr,menu_mode[menu_ptr]);
				//printf("5 fast_mode_ptr:%d fast_mode:%d\n",fast_mode_ptr,fast_mode[fast_mode_ptr]);
				/*if ((ticker%8)==0){
					printf("M> joy_pressed:%d   data_joy:[%02X]   old_data_joy:[%02X]   rel_data_joy:[%02X]   hat_switch:[%02X] \n",joy_pressed,data_joy,old_data_joy,rel_data_joy,hat_switch);
				}*/
				if((!joy_pressed)&&(hat_switch>0)){
					hat_switch=0;
					clear_input();
					continue;
				}				
				switch (menu_mode[menu_ptr]){
					case MENU_MAIN:
						if(is_new_screen){
							draw_main_window();					
							if(init_fs){
								////printf("is_new_screen>N_files:%d\n",N_files);
								draw_file_window();
								display_file_index=-1;
								draw_mur_logo_big(PREVIEW_POS_X+((PREVIEW_WIDTH/2)-(138/2)),60,1);
							}
							is_new_screen=false;
							need_redraw=true;
							clear_input();
						}
						/*--Return from Menu--*/
						if((KBD_ESC)||((data_joy&D_JOY_START)&&(hat_switch==0))||(KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
							fast_menu_index=old_menu_index;
							fast_mode_ptr=0;
							menu_ptr--;
							old_menu_index=0;
							is_new_screen=true;
							busy_wait_ms(150);
							//clear_input();
							hat_switch=0x80;
							continue;
						}	
						/*--Return from Menu--*/
						/*--Settings Menu--*/
						if(KBD_F12){
							menu_ptr++;
							menu_mode[menu_ptr]=MENU_SETTINGS;
							is_new_screen=true;
							last_action=0;
							continue;
						}
						/*--Settings Menu--*/
						if(init_fs==false){
							if(need_redraw){
								MessageBox("SD Card not found!!!","	Please REBOOT   ",CL_LT_YELLOW,CL_RED,0);
								need_redraw=false;
								busy_wait_ms(100);
								clear_input();
								break;
							}
						} else {
							if(KBD_PRT_SCR){
								show_screen^=1;
								last_action = (my_millis()+SHOW_SCREEN_DELAY)-150;
							}
							if((KBD_ENTER)||(data_joy==D_JOY_A)){
								need_redraw=true;
								//printf("file_type:%x\n",files[cur_file_index][(FILE_NAME_LEN-1)]);
								file = (FileRec*)&files[cur_file_index];
								//printf("fname: %s\n",file->filename);
								if (file->attr&AM_DIR){ //выбран каталог
									//printf("cur_file_index:%d  dir_index:%d\n",cur_file_index,cur_dir_index);
									if (cur_file_index==0){//на уровень выше
										if (cur_dir_index>0) {
											cur_dir_index--;
											N_files = read_select_dir(cur_dir_index);
											cur_file_index=0;
											if(cursor_index_old[cur_dir_index]>=0){
												cur_file_index=cursor_index_old[cur_dir_index];
												////printf("restore index:%d\n",cur_file_index);
											}													
											draw_rect(FONT_W,FONT_H,(FONT_W*20),FONT_H,COLOR_MAIN_RAMK,true);
											//busy_wait_ms(200);
											last_action = my_millis();
										};
									} else
									if (cur_dir_index<(DIRS_DEPTH-2)){//выбор каталога
										//printf("store index:%d>%d\n",cur_file_index,cur_dir_index);
										cursor_index_old[cur_dir_index]=cur_file_index;
										cur_dir_index++;
										strncpy(dirs[cur_dir_index],files[cur_file_index],(FILE_NAME_LEN-1));
										N_files = read_select_dir(cur_dir_index);
										cur_file_index=0;
										display_file_index=cur_file_index;
										//shift_file_index=0;
										last_action = my_millis();
										busy_wait_ms(200);
									}
								} else {  // выбран файл
									if(sizeof(sd_buffer)<ZX_RAM_PAGE_SIZE){
										need_reset_after_menu=false;
									}
									memset(afilename,0,FILE_NAME_LEN);
									strncpy(afilename,files[cur_file_index],(FILE_NAME_LEN-1));

									strcpy(activefilename,dir_path);
									strcat(activefilename,"/");
									strcat(activefilename,afilename);

									const char* ext = get_file_extension(afilename);
	
									#ifndef DEBUG_DISABLE_LOADERS
									if(strcasecmp(ext, "z80") == 0) {
										//G_PRINTF_DEBUG("current file select=%s\n",activefilename); 
										//load_image_z80(activefilename);
										im_z80_stop = true;
										while (im_z80_stop){
											busy_wait_ms(10);
											if (im_ready_loading){
												//busy_wait_ms(10);
												if(cfg_res_before_mode>0){
													zx_machine_reset(false);
												}
												AY_reset(cfg_sound_mode);// сбросить AY
												memset(temp_msg,0,sizeof(temp_msg));
												sprintf(temp_msg," Loading file:%s",afilename);
												MessageBox("Z80",temp_msg,CL_WHITE,CL_BLUE,0);
												if (load_image_z80(activefilename)){
													activefilename[0]=0;
													im_z80_stop = false;					   
													im_ready_loading = false;
													menu_mode[menu_ptr]=EMULATION;
													printf("load_image_z80 - OK\n");
													continue;
												} else {
													AY_reset(cfg_sound_mode);// сбросить AY
													MessageBox("Error loading snapshot!!!",afilename,CL_YELLOW,CL_LT_RED,1);
													////printf("load_image_z80 - ERROR\n");
													last_action = my_millis();
													draw_file_window();
													im_z80_stop = false;
													im_ready_loading = false;
													continue;
												}
												//AY_reset();// сбросить AY
											}
										}							
										continue;
									} else
									if(strcasecmp(ext, "sna") == 0) {
										//G_PRINTF_DEBUG("current file select=%s\n",activefilename); 
										//load_image_z80(activefilename);
										im_z80_stop = true;
										while (im_z80_stop){
											busy_wait_ms(10);
											if (im_ready_loading){
												if(cfg_res_before_mode>0){
													zx_machine_reset(false);
												}
												AY_reset(cfg_sound_mode);// сбросить AY
												memset(temp_msg,0,sizeof(temp_msg));
												sprintf(temp_msg," Loading file:%s",afilename);
												MessageBox("SNA",temp_msg,CL_WHITE,CL_BLUE,0);
												if (load_image_sna(activefilename)){
													activefilename[0]=0;
													im_z80_stop = false;					   
													im_ready_loading = false;
													menu_mode[menu_ptr]=EMULATION;
													////printf("load_image_sna - OK\n");
													continue;
												} else {
													AY_reset(cfg_sound_mode);// сбросить AY
													MessageBox("Error loading snapshot!!!",afilename,CL_YELLOW,CL_LT_RED,1);
													////printf("load_image_sna - ERROR\n");
													last_action = my_millis();
													draw_file_window();
													im_z80_stop = false;
													im_ready_loading = false;
													continue;
												}
												//AY_reset();// сбросить AY
											}
										}							
										continue;
									} else
									#endif
									if(strcasecmp(ext, "scr") == 0) {
										//G_PRINTF_DEBUG("current file select=%s\n",activefilename); 
										if(LoadScreenshot(activefilename,true)){
											menu_mode[menu_ptr]=EMULATION;
											continue;
										} else {
											MessageBox("Error loading screen!!!",afilename,CL_YELLOW,CL_LT_RED,1);
											////printf("LoadScreenshot - ERROR\n");
											break;
										}
									} else 
									#ifndef DEBUG_DISABLE_LOADERS
									if(strcasecmp(ext, "tap") == 0) {
										tap_loader_active = TAPE_OFF;
										//printf("TAP prepare\n");
										if(cfg_res_before_mode>0){
											zx_machine_reset(false);
										}
										AY_reset(cfg_sound_mode);// сбросить AY
										////printf("cfg_fast_load_tap>%d\n",cfg_fast_load_tap);
										if(cfg_tap_load_mode==FAST_LOAD_TAP){
											if(FastOpenTAP(activefilename)){
												sprintf(temp_msg," Loading:%s ",afilename);
												MessageBox("TAPE",temp_msg,CL_WHITE,CL_BLUE,0);
												tap_loader_active|=TAPE_INTERNAL_ROM;
												memset(temp_msg,0,sizeof(temp_msg));
												tape_disp=1;
												menu_mode[menu_ptr]=EMULATION;
												continue;
											} else {
												activefilename[0] = 0;
												MessageBox("ERROR","Loading tape!!!",CL_LT_YELLOW,CL_LT_RED,1);
												//printf("Tap ERROR\n");
												im_z80_stop = false;
												im_ready_loading = false;
												//zx_machine_reset();
												tape_disp=0;
												continue;
											}
										} else {
											if(TAP_Load(activefilename)){
												//printf("TAP loaded\n");
												memset(temp_msg,0,sizeof(temp_msg));
												sprintf(temp_msg," Loading:%s ",afilename);
												MessageBox("TAPE",temp_msg,CL_WHITE,CL_BLUE,0);
												memset(temp_msg,0,sizeof(temp_msg));
												menu_mode[menu_ptr]=EMULATION;
												tape_disp=1;
												continue;
											} else {
												activefilename[0] = 0;
												MessageBox("ERROR","Loading tape!!!",CL_LT_YELLOW,CL_LT_RED,1);
												//printf("Tap ERROR\n");
												im_z80_stop = false;
												im_ready_loading = false;
												//zx_machine_reset();
												tape_disp=0;
												continue;
											}
										}
										continue;
										//busy_wait_ms(5);
									} else
									#endif
									if(strcasecmp(ext, "rom") == 0) {
										menu_mode[menu_ptr]=EMULATION;
										//printf("ROM prepare\n");
										zx_machine_reset(false);
										continue;
									}								
									#ifdef TRDOS_COMPILE
										else
										if(strcasecmp(ext, "trd") == 0){
											//G_PRINTF_DEBUG("current file select=%s\n",activefilename);
											menu_mode[menu_ptr]=MENU_SELDRIVE;
											last_action=0;
											settings_index = 0;
											is_new_screen=true;
											clear_input();
											continue;
										}
									#endif
									#ifndef DEBUG_DISABLE_LOADERS
 									else
									if(strcasecmp(ext, "pok") == 0) {
										if(poke_count=load_pok_captions(activefilename)){
											menu_ptr++;
											menu_mode[menu_ptr]=MENU_POKE_FILE;
											last_action=0;
											settings_index = 0;
											lineStart = 0;
											is_new_screen=true;
											clear_input();
										} else {
											MessageBox("Error loading pokes!!!",afilename,CL_YELLOW,CL_LT_RED,1);
											last_action = my_millis();
											draw_file_window();
										}
										continue;
									}
									#endif		
								}
							}
							if(((KBD_INSERT)||(KBD_SPACE)||(data_joy==D_JOY_B))&&(cur_file_index>0)){
								file = (FileRec*)&files[cur_file_index];
								if(file->attr&~TOP_DIR_ATTR){
									file->attr^=SELECTED_FILE_ATTR;	
								}
								cur_file_index++;
								need_redraw=true;
								last_action = my_millis();
							}
							if((((KBD_L_SHIFT)||(KBD_R_SHIFT))&&(KBD_DELETE))&&(cur_file_index>0)){
								sel_files=0;
								for(short int i=0;i<N_files+1;i++){
									file = (FileRec*)&files[i];
									if(file->attr&SELECTED_FILE_ATTR){
										sel_files++;
									}
								}
								if(sel_files>0){
									memset(temp_msg,0,sizeof(temp_msg));
									sprintf(temp_msg,"DELETE [%d] SELECTED FILES?",sel_files);
									if(DialogBox("[DELETE]",temp_msg,CL_RED,COLOR_BACKGOUND,DIALOG_OK_CAN)==DLG_RES_OK){
										del_files=0;
										err_files=0;
										for(short int i=0;i<N_files+1;i++){
											file = (FileRec*)&files[i];
											if(file->attr&SELECTED_FILE_ATTR){
												memset(afilename,0,FILE_NAME_LEN);
												strncpy(afilename,file->filename,(FILE_NAME_LEN-1));
												strcpy(activefilename,dir_path);
												strcat(activefilename,"/");
												strcat(activefilename,afilename);
												if(file->attr&AM_DIR){
													//printf("Delete node:%s\n",activefilename);
													file_descr=sd_delete_node(activefilename, sizeof activefilename / sizeof activefilename[0], &sd_file_info);
													if (file_descr==FR_OK){
														del_files++;
														//printf("OK\n");
													} else {
														err_files++;
														//printf("ERROR\n");
													}
												}else{
													printf("Delete file>%s\n",activefilename);
													file_descr=sd_delete(activefilename);
													if (file_descr==FR_OK){
														del_files++;
														//printf("OK\n");
													}else{
														err_files++;
														//printf("ERROR\n");
													}
												}
												file->attr&=~SELECTED_FILE_ATTR;
											}
										}
										memset(temp_msg,0,sizeof(temp_msg));
										sprintf(temp_msg,"[%d]/ERR[%d] OF [%d] FILES DELETED!!!",del_files,err_files,sel_files);
										DialogBox("[DELETE]",temp_msg,CL_LT_BLUE,COLOR_BACKGOUND,DIALOG_OK);
										is_new_screen=true;
										N_files = read_select_dir(cur_dir_index);
										continue;
									} else {
										for(short int i=0;i<N_files+1;i++){
											file = (FileRec*)&files[i];
											if(file->attr&SELECTED_FILE_ATTR){
												file->attr^=SELECTED_FILE_ATTR;
											}
										}										
										is_new_screen=true;
										continue;
									}
								}
							}
							//стрелки вверх вниз
							if(((KBD_DOWN)||(data_joy==D_JOY_DOWN))&&(cur_file_index<(N_files))){ 
								cur_file_index++;
								clear_input();
								need_redraw=true;
								last_action = my_millis();
							}
							if(((KBD_UP)||(data_joy==D_JOY_UP))&&(cur_file_index>0)){
								cur_file_index--;
								clear_input();
								need_redraw=true;
								last_action = my_millis();
							}
							//начало и конец списка
							if((KBD_LEFT)){
								cur_file_index=0;
								shift_file_index=0;
								clear_input();
								need_redraw=true;
								last_action = my_millis();
							}
							if((KBD_RIGHT)){
								cur_file_index=N_files;
								shift_file_index=(N_files>=NUM_SHOW_FILES)?N_files-NUM_SHOW_FILES:0;
								clear_input();
								need_redraw=true;
								last_action = my_millis();
							}
							//PAGE_UP PAGE_DOWN
							if(((KBD_PAGE_DOWN)||(data_joy==D_JOY_RIGHT))&&(cur_file_index<(N_files))){
								cur_file_index+=NUM_SHOW_FILES;
								need_redraw=true;
								last_action = my_millis();
							}
							if(((KBD_PAGE_UP)||(data_joy==D_JOY_LEFT))&&(cur_file_index>0)){
								cur_file_index-=NUM_SHOW_FILES;
								need_redraw=true;
								last_action = my_millis();
							}
							//Возврат на уровень выше по BACKSPACE
							if((KBD_BACK_SPACE)||(data_joy==D_JOY_SELECT)){
								if (cur_dir_index==0){
									if (cur_file_index==0) cur_file_index=1;//не можем выбрать каталог вверх
									if (shift_file_index==0) shift_file_index=1;//не отображаем каталог вверх
									read_select_dir(cur_dir_index);
								} else {
									cur_dir_index--;
									N_files = read_select_dir(cur_dir_index);
									cur_file_index=0;
									//draw_text_len(FONT_W,FONT_H,"					",COLOR_BACKGOUND,COLOR_BORDER,20);
									draw_rect(FONT_W,FONT_H,(FONT_W*20),FONT_H,COLOR_MAIN_RAMK,true);
									cur_file_index = 0;
									shift_file_index = 0;
								}
								need_redraw=true;
								last_action = my_millis();
							}	
							if(need_redraw){
								//last_action = my_millis();
								scroll_lfn=false;
								if (cur_file_index<0) cur_file_index=0;
								if (cur_file_index>=N_files) cur_file_index=N_files;
								for (short int i=NUM_SHOW_FILES; i--;){
									if ((cur_file_index-shift_file_index)>=(NUM_SHOW_FILES)) shift_file_index++;
									if ((cur_file_index-shift_file_index)<0) shift_file_index--;
								}
								//ограничения корневого каталога
								if (cur_dir_index==0){
									if (cur_file_index==0) cur_file_index=1;//не можем выбрать каталог вверх
									if (shift_file_index==0) shift_file_index=1;//не отображаем каталог вверх
								}
								//printf("cur_dir_index:%d  cur_file_index:%d shift_file_index:%d\n",cur_dir_index,cur_file_index,shift_file_index);
								//прорисовка
								//заголовок окна - текущий каталог		
								//draw_text_len(FONT_W,FONT_H-1,dir_path+2,COLOR_TEXT,COLOR_FULLSCREEN,20);
								//sprintf(save_file_name_image,"0:/save/__F%d.Z80 ",inx_f1);
								////printf("Dir:%s",dir_path);
								if (strlen(dir_path+2)>0){
									draw_text_len(FONT_W,FONT_H,dir_path+2,COLOR_ITEXT,COLOR_MAIN_RAMK,20);
								} /*else {
									draw_text_len(FONT_W,FONT_H-1,"					",COLOR_BACKGOUND,COLOR_BORDER,20);
								}*/
								for(short int i=0;i<NUM_SHOW_FILES;i++){
									//busy_wait_us(150);
									//если файлов меньше, чем отведено экрана - заполняем пустыми строками
									if ((i>N_files)||((cur_dir_index==0)&&(i>(N_files-1)))){
										draw_rect(FONT_W,(i+2)*FONT_H,(FONT_W*FILE_NAME_LEN),SCREEN_H-((FONT_H*3)+(i*FONT_H)),COLOR_BACKGOUND,true);
										//draw_text_len(FONT_W,2*FONT_H+i*FONT_H," ",color_text,color_bg,14);
										//last_action = my_millis();
										scroll_lfn=false;
										clear_input();
										is_new_screen=false;
										continue;
									}									
									uint8_t color_text=COLOR_TEXT;
									uint8_t color_bg=COLOR_BACKGOUND;
									icon[0] = 0x20;
									icon[1] = 0x00;
									file = (FileRec*)&files[i+shift_file_index];
           							uint8_t len = strlen(file->filename)<(FILE_NAME_LEN)?strlen(file->filename):(FILE_NAME_LEN-1);

									if (i==(cur_file_index-shift_file_index)){
										color_text=COLOR_CURRENT_TEXT;
										color_bg=COLOR_CURRENT_BG;
										strncpy(current_lfn,get_lfn_from_dir(dir_path,file),200);
									}
									if (file->attr&AM_DIR){
										icon[0] = 0xF8; //folder
										draw_text_len(FONT_W,2*FONT_H+i*FONT_H,icon,CL_YELLOW,color_bg,1);
										//draw_text_len(FONT_W,2*FONT_H+i*FONT_H,"/",color_text,color_bg,1);
									} else {
										icon[0] = 0xF7; //file
										draw_text_len(FONT_W,2*FONT_H+i*FONT_H,icon,CL_GRAY,color_bg,1);
										//draw_text_len(FONT_W,2*FONT_H+i*FONT_H," ",color_text,color_bg,1);
									}
									memset(temp_msg,0x00,sizeof(temp_msg));
									strncpy(&temp_msg[FILE_NAME_LEN+20],file->filename,FILE_NAME_LEN-1);
									const char* ext = get_file_extension(&temp_msg[FILE_NAME_LEN+20]);
									//printf(">0 %s   %s   %d   %d\n",temp_msg,ext,len,strlen(ext));
									if(strlen(ext)>0){
										memset(temp_msg,0x20,FILE_NAME_LEN);
										strncpy(temp_msg,file->filename,(len-(strlen(ext)+1)));
										//printf(">1 %s   %s   %d   %d\n",temp_msg,ext,len,strlen(ext));
										strncpy(&temp_msg[(FILE_NAME_LEN-4)],ext,3);
										//printf(">2 %s   %s   %d   %d\n",temp_msg,ext,len,strlen(ext));
										temp_msg[(FILE_NAME_LEN-5)]=0x2E;
										//printf(">3 %s   %s   %d   %d\n",temp_msg,ext,len,strlen(ext));										
									} else {
										strncpy(temp_msg,file->filename,FILE_NAME_LEN-1);
									}
									//printf("> %s   %s   %d   %d\n",temp_msg,ext,len,strlen(ext));

									if(file->attr&AM_RDO)temp_msg[(FILE_NAME_LEN-5)]=0xB0;//(FILE_NAME_LEN-3)
									if(file->attr&AM_HID)temp_msg[(FILE_NAME_LEN-5)]=0xB1;//(FILE_NAME_LEN-3)
									if(file->attr&AM_SYS)temp_msg[(FILE_NAME_LEN-5)]=0xB2;//(FILE_NAME_LEN-3)
									if(file->attr&SELECTED_FILE_ATTR){
										color_text=COLOR_SELECT_TEXT;
									}
									draw_text_len(2*FONT_W,2*FONT_H+i*FONT_H,temp_msg,color_text,color_bg,(FILE_NAME_LEN-1));//get_file_from_dir("0:/z80",i)
									////printf("> %s %d, %d\n",files[i+shift_file_index],cur_file_index,display_file_index);
								}
								
								
								short int file_inx=cur_file_index-1;
								if (file_inx==-1) file_inx=0;
								if (file_inx==N_files) file_inx+=1;
								short int shft=208*(file_inx)/(N_files<=1?1:N_files-1); 
								draw_rect((PREVIEW_POS_X-FONT_W)+1,PREVIEW_POS_Y,FONT_W-2,SCREEN_H-(FONT_H*3),COLOR_BACKGOUND,true);  //Заливка фона полосы прокрутки
								draw_rect((PREVIEW_POS_X-FONT_W)+2,shft+(PREVIEW_POS_Y+1),4,5,COLOR_MAIN_RAMK,true);  //указатель полосы прокрутки

								//if(strcasecmp(files[cur_file_index], "..")==0) {
								file = (FileRec*)&files[cur_file_index];
								if(file->attr&~TOP_DIR_ATTR){
									display_file_index=cur_file_index;
									//draw_mur_logo();
								}
								/*if ((cur_file_index>0)&&(display_file_index==-1)){
									last_action = my_millis();
								}*/
								need_redraw=false;
							}
							clear_input();								
							//break;
						}
					break;
					case MENU_JOY_MAIN:
						if(is_new_screen){
							//printf("Joy new screen\n");
							graphics_set_hud_handler(NULL);
							if((!zx_machine_get_vbuf_en())){
								if(im_z80_stop){im_z80_stop=false;}
								busy_wait_ms(20);
								while(!zx_screen_refresh){
									zx_machine_enable_vbuf(true);
									busy_wait_ms(10);
								};
								busy_wait_ms(20);
								enter_pause();
							}
							busy_wait_ms(10);
							zx_machine_enable_vbuf(false);							
							//printf("Joy draw screen\n");
							fast_menu_index = 0;
							draw_fast_menu((SCREEN_W/2)-(9*FONT_W),(SCREEN_H/2)-(5*FONT_H),true,fast_mode[fast_mode_ptr],fast_menu_index);
							is_new_screen=false;
							busy_wait_ms(150);
							clear_input();
							last_action=0;
							scroll_lfn=false;
						}
						/*--Return from Menu--*/
						if((KBD_F12)||(KBD_ESC)||((data_joy&D_JOY_START)&&(hat_switch==0))||(KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
							hat_switch=0x80;
							if((fast_mode[fast_mode_ptr]==FAST_MENU_SAVE)||(fast_mode[fast_mode_ptr]==FAST_MENU_LOAD)||(fast_mode[fast_mode_ptr]==FAST_MENU_TAPE)){
								fast_menu_index=old_menu_index;
								fast_mode_ptr--;
								menu_ptr--;
								old_menu_index=0;
								is_new_screen=true;
							} else{
								menu_mode[menu_ptr]=EMULATION;
								fast_mode_ptr=0;
								fast_menu_index=0;
								//show_hud=old_show_hud;
								menu_ptr--;
								old_menu_index=0;
								is_new_screen=true;
							}
							busy_wait_ms(250);
							//clear_input();
							continue;
						}						
						/*--Return from Menu--*/
						if(((KBD_DOWN)||(data_joy==D_JOY_DOWN))&&(fast_menu_index<(short int)fast_menu_lines[(uint8_t)fast_mode[(uint8_t)fast_mode_ptr]])){ fast_menu_index++; need_redraw=true;}
						if(((KBD_UP)||(data_joy==D_JOY_UP))&&(fast_menu_index>=0)){fast_menu_index--;need_redraw=true;}
						//начало и конец списка
						if(((KBD_PAGE_DOWN)||(data_joy==D_JOY_RIGHT))&&(fast_menu_index<(short int)fast_menu_lines[(uint8_t)fast_mode[(uint8_t)fast_mode_ptr]])){fast_menu_index+=3;need_redraw=true;}
						if(((KBD_PAGE_UP)||(data_joy==D_JOY_LEFT))&&(fast_menu_index>0)){fast_menu_index-=3;need_redraw=true;}
						if (fast_menu_index<0) fast_menu_index=(short int)fast_menu_lines[(uint8_t)fast_mode[(uint8_t)fast_mode_ptr]]-1;
						if (fast_menu_index>=(short int)fast_menu_lines[(uint8_t)fast_mode[(uint8_t)fast_mode_ptr]]) fast_menu_index=0;
						if((KBD_ENTER)||(data_joy==D_JOY_A)){
							need_redraw=true;
							if(fast_mode[fast_mode_ptr]==FAST_MENU_MAIN){
								switch (fast_menu_index){
									case FAST_MAIN_MANAGER: //Main menu
										menu_ptr++;
										menu_mode[menu_ptr]=MENU_MAIN;
										fast_mode_ptr=0;
										fast_menu_index=2;
										is_new_screen=true;
										continue;
										break;
									case FAST_MAIN_KEYBOARD: //KEYBOARD
										menu_ptr++;
										old_menu_index = fast_menu_index;
										menu_mode[menu_ptr]=MENU_KEYBOARD;
										is_new_screen=true;
										continue;
										break;
									case FAST_MAIN_SAVE: //Save
										menu_ptr++;
										menu_mode[menu_ptr]=MENU_JOY_MAIN;
										old_menu_index = fast_menu_index;
										fast_mode_ptr++;
										fast_mode[fast_mode_ptr]=FAST_MENU_SAVE;
										fast_menu_index=0;
										get_saveslots();
										is_new_screen=true;
										continue;
										break;
									case FAST_MAIN_LOAD: //Load
										menu_ptr++;
										menu_mode[menu_ptr]=MENU_JOY_MAIN;
										old_menu_index = fast_menu_index;
										fast_mode_ptr++;
										fast_mode[fast_mode_ptr]=FAST_MENU_LOAD;
										fast_menu_index=0;
										get_saveslots();
										is_new_screen=true;
										continue;
										break;
									case FAST_MAIN_TAPE: //Tape
										menu_ptr++;
										menu_mode[menu_ptr]=MENU_JOY_MAIN;
										old_menu_index = fast_menu_index;
										fast_mode_ptr++;
										fast_mode[fast_mode_ptr]=FAST_MENU_TAPE;
										fast_menu_index=0;
										is_new_screen=true;
										continue;
										break;
									case FAST_MAIN_POKE: //Poke
										menu_ptr++;
										old_menu_index = fast_menu_index;
										menu_mode[menu_ptr]=MENU_POKE;
										is_new_screen=true;
										clear_input();
										continue;
										break;										
									case FAST_MAIN_HELP: //HELP
										menu_ptr++;
										old_menu_index = fast_menu_index;
										menu_mode[menu_ptr]=MENU_HELP;
										is_new_screen=true;
										continue;
										break;
									case FAST_MAIN_LOCKJOY: //SETTINGS
										if(hat_locked==0){hat_locked=0xFF;} else {hat_locked=0x00;};
										is_new_screen=true;
										continue;
										break;

									case FAST_MAIN_SETTINGS: //SETTINGS
										menu_ptr++;
										old_menu_index = fast_menu_index;
										menu_mode[menu_ptr]=MENU_SETTINGS;
										is_new_screen=true;
										continue;
										break;
									case FAST_MAIN_SOFT_RESET: //Soft reset
										resume_pause();
										zx_machine_reset(false);
										AY_reset(cfg_sound_mode);
										menu_mode[menu_ptr]=EMULATION;
										continue;
										break;
									case FAST_MAIN_HARD_RESET: //Hard reset
										software_reset();
										break;
									default:
										break;
								}
							}
							#ifndef DEBUG_DISABLE_LOADERS
							if(fast_mode[fast_mode_ptr]==FAST_MENU_LOAD){
								if(fast_menu_index==0){
									sprintf(save_file_name_image,"0:/save/QSAVE.Z80 ");
									load_z80_file(save_file_name_image,0);
									menu_ptr=0;
									menu_mode[menu_ptr]=EMULATION;
									continue;
								} else {
									sprintf(save_file_name_image,"0:/save/__F%d.Z80 ",fast_menu_index);
									load_z80_file(save_file_name_image,fast_menu_index);
									fast_mode_ptr=0;
									menu_ptr=0;
									menu_mode[menu_ptr]=EMULATION;	
									continue;
								}
							}							
							if(fast_mode[fast_mode_ptr]==FAST_MENU_SAVE){
								if(fast_menu_index==0){
									sprintf(save_file_name_image,"0:/save/QSAVE.Z80 ");
									save_z80_file(save_file_name_image,0);
									menu_ptr=0;
									menu_mode[menu_ptr]=EMULATION;
									continue;
								} else {
									sprintf(save_file_name_image,"0:/save/__F%d.Z80 ",fast_menu_index);
									save_z80_file(save_file_name_image,fast_menu_index);
									fast_mode_ptr=0;
									menu_ptr=0;
									menu_mode[menu_ptr]=EMULATION;	
									continue;
								}
							}
							#endif
							if(fast_mode[fast_mode_ptr]==FAST_MENU_TAPE){
								switch (fast_menu_index){
									case 0: tape_cmd = TAPE_STATE_START;		menu_ptr=0; menu_mode[menu_ptr]=EMULATION; fast_mode_ptr=0;fast_menu_index=0; continue; break;
									case 1: tape_cmd = TAPE_STATE_STOP;			menu_ptr=0; menu_mode[menu_ptr]=EMULATION; fast_mode_ptr=0;fast_menu_index=0; continue; break;
									case 2: tape_cmd = TAPE_STATE_REWIND;		menu_ptr=0; menu_mode[menu_ptr]=EMULATION; fast_mode_ptr=0;fast_menu_index=0; continue; break;
									case 3: tape_cmd = TAPE_STATE_PREV_BLOCK;	menu_ptr=0; menu_mode[menu_ptr]=EMULATION; fast_mode_ptr=0;fast_menu_index=0; continue; break;
									case 4: tape_cmd = TAPE_STATE_NEXT_BLOCK;	menu_ptr=0; menu_mode[menu_ptr]=EMULATION; fast_mode_ptr=0;fast_menu_index=0; continue; break;
									case 5: tape_cmd = TAPE_STATE_EJECT;		menu_ptr=0; menu_mode[menu_ptr]=EMULATION; fast_mode_ptr=0;fast_menu_index=0; continue; break;
									default: break;									
								}
							}							

						}
						if(need_redraw){
							draw_fast_menu((SCREEN_W/2)-(9*FONT_W),(SCREEN_H/2)-(5*FONT_H),false,fast_mode[fast_mode_ptr],fast_menu_index);
							need_redraw=false;
							clear_input();
						}
						continue;
					break;
					case MENU_HELP:
						if (is_new_screen){
							////printf("is_new_screen\n");
							draw_main_window();	
							is_new_screen=false;
							busy_wait_ms(150);
							clear_input();
						}
						/*--Return from Menu--*/
						if((KBD_F12)||(KBD_ESC)||(KBD_F1)||((data_joy&D_JOY_START)&&(hat_switch==0))||(KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
							menu_ptr--;
							need_redraw=true;
							is_new_screen=true;
							clear_input();
							continue;
						}
						if((KBD_F3)||(data_joy==D_JOY_B)){
							help_lng=1;
							need_redraw=true;
							is_new_screen=true;
							clear_input();
							continue;
						}
						if((KBD_F2)||(data_joy==D_JOY_A)){
							help_lng=0;
							need_redraw=true;
							is_new_screen=true;
							clear_input();
							continue;
						}													

						/*--Return from Menu--*/
						if(((KBD_UP)||(data_joy==D_JOY_UP))&&(lineStart>0)){lineStart--;need_redraw=true;};
						if(((KBD_DOWN)||(data_joy==D_JOY_DOWN))&&(lineStart<HELP_LINES)){lineStart++;need_redraw=true;};
						if(((KBD_PAGE_UP)||(KBD_LEFT)||(data_joy==D_JOY_LEFT))&&(lineStart>0)){lineStart-=5;need_redraw=true;}
						if(((KBD_PAGE_DOWN)||(KBD_RIGHT)||(data_joy==D_JOY_RIGHT))&&(lineStart<HELP_LINES)){lineStart+=5;need_redraw=true;}
						if(lineStart<0){lineStart=0;};
						if(lineStart>(HELP_LINES-SCREEN_HELP_LINES)){lineStart=(HELP_LINES-SCREEN_HELP_LINES);};
						////printf("need_redraw:%d lineStart:%d\n",need_redraw,lineStart);
						if (data_joy>0){ //***********************
							old_data_joy=0;
							if(Joystics.Present_WII_joy){
								Wii_clear_old();
							}
						};
						if(need_redraw){
							draw_help_text(lineStart,help_lng);
							short int linePos=lineStart;
							if (linePos<0) linePos=0;
							if (linePos>(HELP_LINES-SCREEN_HELP_LINES)) linePos=(HELP_LINES-SCREEN_HELP_LINES);
							short int shft=(207*linePos)/(HELP_LINES-SCREEN_HELP_LINES); 
							draw_rect(FONT_W+1+FONT_W*37,PREVIEW_POS_Y-1,FONT_W,SCREEN_H-(FONT_H*3)+2,COLOR_MAIN_RAMK,false);//Рамка полосы прокрутки
							draw_rect(FONT_W+2+FONT_W*37,PREVIEW_POS_Y,FONT_W-2,SCREEN_H-(FONT_H*3),COLOR_BACKGOUND,true);  //Заливка фона полосы прокрутки
							draw_rect(FONT_W+3+FONT_W*37,shft+17,4,5,COLOR_TEXT,true);  //указатель полосы прокрутки
							if (conv_utf_cp866(help_head[help_lng], temp_msg, strlen(help_head[help_lng]))>0){
								draw_text5x7_len((23*FONT_5x7_W),SCREEN_H-FONT_5x7_H,temp_msg,CL_BLACK,CL_WHITE,18);
							}
							need_redraw=false;
							clear_input();
							continue;
						}
					break;
					case MENU_KEYBOARD:
						if (is_new_screen){
							////printf("is_new_screen\n");
							memset(hud_line,0x11,SCREEN_W);
							resume_pause();
							current_hud_mode|=HM_KBD_HUD;
							handler_ptr=&hud_prepare_kbd;
							graphics_set_hud_handler(handler_ptr);
							zx_machine_enable_vbuf(true);
							is_new_screen=false;
							clear_input();
							//convert_kb_u_to_kb_zx(&kb_st_ps2,zx_write_buffer->kb_data, true);
						}
						/*--Return from Menu--*/
						if((KBD_F12)||(KBD_ESC)||((data_joy&D_JOY_START)&&(hat_switch==0))||(KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
							kbd_col=0;
							kbd_row=0;
							kbd_cshift=false;
							kbd_sshift=false;
							fast_menu_index=old_menu_index;
							//show_hud=old_show_hud;
							current_hud_mode&=~HM_KBD_HUD;
							fast_mode_ptr=0;
							menu_ptr--;
							/*if(menu_mode[menu_ptr]==MENU_JOY_MAIN){
								graphics_set_hud_handler(NULL);
 								zx_machine_enable_vbuf(true);
								busy_wait_ms(150);
								zx_machine_enable_vbuf(false);
							}*/
							hat_switch=0x80;
							old_menu_index=0;
							is_new_screen=true;
							busy_wait_ms(250);
							//clear_input();
							continue;
						} 						
						/*--Return from Menu--*/
						if((KBD_DOWN)	||(data_joy==D_JOY_DOWN))	{kbd_row++;need_redraw=true;if(KBD_PRESS){busy_wait_ms(175);};}
						if((KBD_UP)		||(data_joy==D_JOY_UP))		{kbd_row--;need_redraw=true;if(KBD_PRESS){busy_wait_ms(175);};}
						if((KBD_RIGHT)	||(data_joy==D_JOY_RIGHT))	{kbd_col++;need_redraw=true;if(KBD_PRESS){busy_wait_ms(175);};}
						if((KBD_LEFT)	||(data_joy==D_JOY_LEFT))	{kbd_col--;need_redraw=true;if(KBD_PRESS){busy_wait_ms(175);};}

						if (kbd_col<0){kbd_col=KBD_MAX_COL;}
						if (kbd_col>KBD_MAX_COL){kbd_col=0;}
						if (kbd_row>KBD_MAX_ROW){kbd_row=0;}
						if (kbd_row<0){kbd_row=KBD_MAX_ROW;}
						
						//if((data_joy>0)||(KBD_PRESS)) printf("kbd_row:%d  kbd_col:%d  kbd_cshift:%d  kbd_sshift:%d\n",kbd_row,kbd_col,kbd_cshift,kbd_sshift);
						if((KBD_ENTER)||(data_joy==D_JOY_A)){
							if(kbd_row==0){
								if(kbd_col<KBD_MAX_COL){
									////printf("Send Stroke:%d\n",kbd_col);
									send_keystroke((uint8_t)kbd_col);
									if(kbd_col>5){
										menu_mode[menu_ptr]=EMULATION;
										fast_mode_ptr=0;
										fast_menu_index=0;
										//show_hud=old_show_hud;
										current_hud_mode&=~HM_KBD_HUD;
										graphics_set_hud_handler(NULL);
										kbd_col=0;
										kbd_row=0;
										//kbd_vis=false;
										kbd_cshift=false;
										kbd_sshift=false;
									}
								}
								if(kbd_col==KBD_MAX_COL){
									menu_mode[menu_ptr]=EMULATION;
									fast_mode_ptr=0;
									fast_menu_index=0;
									//show_hud=old_show_hud;
									current_hud_mode&=~HM_KBD_HUD;
									graphics_set_hud_handler(NULL);
									kbd_col=0;
									kbd_row=0;
									kbd_cshift=false;
									kbd_sshift=false;
								}
							} else {
								main_loop = time_us_32();
								if(kbd_chr[kbd_row][kbd_col]==1){
									kbd_cshift^=true;
								} else if(kbd_chr[kbd_row][kbd_col]==2){
									kbd_sshift^=true;
								}								
								if((kbd_cshift)){zx_write_buffer->kb_data[0]|=(1<<0);}
								if((kbd_sshift)){zx_write_buffer->kb_data[7]|=(1<<1);}
								zx_write_buffer->kb_data[kbd_idx[kbd_row][kbd_col]]|=kbd_codes[kbd_col];
								if((kbd_cshift)&&(kbd_sshift)){
									kbd_cshift=false;
									kbd_sshift=false;
								}								
							}
							busy_wait_ms(150);
							//for(uint8_t j=0;j<8;j++){//printf("\t%02X",zx_write_buffer->kb_data[j]);};//printf("\n"); //DEBUG
							////printf("kbd_idx:%02X  kbd_codes:%02X\n",kbd_idx[kbd_row][kbd_col],kbd_codes[kbd_col]);
							need_redraw=true;
							//kbd_scr_update=my_millis();
						}
						if(need_redraw){
							//printf("img_up:%d   img_down:%d   img_left:%d   img_right:%d   img_first_line:%d   img_last_line:%d   kbd_first_line:%d \n",img_up,img_down,img_left,img_right,img_first_line,img_last_line,kbd_first_line);
							//busy_wait_ms(200);
							//printf("[%010ld]>1>[%08lX][%08lX][%08lX][%08lX]:[%02X]  [%02X][%02X][%02X][%02X][%02X][%02X][%02X][%02X]  ["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]\n", time_us_32()-main_loop,kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3],kb_st_ps2.state,zx_write_buffer->kb_data[0],zx_write_buffer->kb_data[1],zx_write_buffer->kb_data[2],zx_write_buffer->kb_data[3],zx_write_buffer->kb_data[4],zx_write_buffer->kb_data[5],zx_write_buffer->kb_data[6],zx_write_buffer->kb_data[7],BYTE_TO_BINARY(data_joy),BYTE_TO_BINARY(old_data_joy),BYTE_TO_BINARY(rel_data_joy));
							zx_machine_input_set();
							while (!ack_input){busy_wait_ms(5);}
							need_redraw=false;
						} else{
							if((!KBD_PRESS_STATE)&&(!joy_pressed)){
								memset(&zx_write_buffer->kb_data,0,8);
								//printf("[%010ld]>3>[%08lX][%08lX][%08lX][%08lX]:[%02X]  [%02X][%02X][%02X][%02X][%02X][%02X][%02X][%02X]  ["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]\n", time_us_32()-main_loop,kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3],kb_st_ps2.state,zx_write_buffer->kb_data[0],zx_write_buffer->kb_data[1],zx_write_buffer->kb_data[2],zx_write_buffer->kb_data[3],zx_write_buffer->kb_data[4],zx_write_buffer->kb_data[5],zx_write_buffer->kb_data[6],zx_write_buffer->kb_data[7],BYTE_TO_BINARY(data_joy),BYTE_TO_BINARY(old_data_joy),BYTE_TO_BINARY(rel_data_joy));
								zx_machine_input_set();
								while (!ack_input){busy_wait_ms(5);}
								main_loop = time_us_32();
							}
						}
					break;
					case MENU_SELDRIVE:
						if (is_new_screen){
							////printf("is_new_screen\n");
							draw_fast_menu((SCREEN_W/2)-(8*FONT_W),(SCREEN_H/2)-(5*FONT_H),true,FAST_MENU_MOUNT,settings_index);
							is_new_screen=false;
							scroll_action = 0;
							last_action = 0;
							busy_wait_ms(150);
							clear_input();
						}
						if((KBD_F12)||(KBD_ESC)||(data_joy&D_JOY_START)||(KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
							menu_mode[menu_ptr]=MENU_MAIN;
							is_new_screen=true;
							clear_input();
							continue;
						}
						if(init_fs){
							if((KBD_UP)||(data_joy==D_JOY_UP)){settings_index--;need_redraw=true;}
							if((KBD_DOWN)||(data_joy==D_JOY_DOWN)){settings_index++;need_redraw=true;}
							if((KBD_LEFT)||(data_joy==D_JOY_LEFT)){settings_index--;need_redraw=true;};
							if((KBD_RIGHT)||(data_joy==D_JOY_RIGHT)){settings_index++;}need_redraw=true;;
							if (settings_index<0){settings_index=TRD_SUBMENU_LINES;}
							if (settings_index>TRD_SUBMENU_LINES){settings_index=0;}
							if(((KBD_ENTER)||(data_joy==D_JOY_A))){
								disk_action = my_millis();
								if(settings_index<(TRD_SUBMENU_LINES-4)){
									clear_input();
									if(cfg_res_before_mode>0){
										zx_machine_reset(false);
										AY_reset(cfg_sound_mode);// сбросить AY
									}
									////printf("Insert TRD:%s to Drive: %s \n",afilename,fast_mode[5][settings_index]);
									if(load_image_TRDOS(activefilename,settings_index)){
										memset(temp_msg,0,sizeof(temp_msg));
										sprintf(temp_msg," Insert TRD:%s to Drive: %c:",afilename, 0x41+settings_index);
										MessageBox("TRD",temp_msg,CL_WHITE,CL_BLUE,2);
										menu_mode[menu_ptr]=MENU_MAIN;
										is_new_screen=true;
										continue;
									} else {
										MessageBox("Error setting TRD!!!",afilename,CL_YELLOW,CL_LT_RED,1);
										//printf("Setting TRD - ERROR\n");									
										break;
									}
									menu_mode[menu_ptr]=MENU_MAIN;
									is_new_screen=true;
									//TRDOS_disabled=false;
									continue;
								}
								if(settings_index==TRD_SUBMENU_LINES){
									clear_input();
									for(uint8_t i=0;i<4;i++){
										free_image_TRDOS(i);
										drives_status[i]=ICON_DISK_OFF;
									}
									////printf("Eject All \n");
									MessageBox("TRD","Eject All TRD!!!",CL_BLUE,CL_YELLOW,2);
									menu_mode[menu_ptr]=MENU_MAIN;
									is_new_screen=true;
									//TRDOS_disabled=true;
									continue;
								}
								if((settings_index>(TRD_SUBMENU_LINES-5))&&(settings_index<TRD_SUBMENU_LINES)){
									clear_input();
									free_image_TRDOS(settings_index-4);
									drives_status[settings_index-4]=ICON_DISK_OFF;
									all_FD_empty = true;
									for(uint8_t i=0;i<4;i++){
										if(!is_image_TRDOS_empty(i)){
											all_FD_empty=false;
											break;
										}
									}
									//TRDOS_disabled=all_FD_empty;
									////printf("Eject from  Drive: %s \n",fast_mode[5][settings_index]);
									memset(temp_msg,0,sizeof(temp_msg));
									sprintf(temp_msg," Eject TRD from Drive: %c:", 0x41+(settings_index-4));
									MessageBox("TRD",temp_msg,CL_BLUE,CL_YELLOW,2);
									menu_mode[menu_ptr]=MENU_MAIN;
									is_new_screen=true;
									continue;
								}
							}
							if(need_redraw){
								draw_fast_menu((SCREEN_W/2)-(8*FONT_W),(SCREEN_H/2)-(5*FONT_H),false,FAST_MENU_MOUNT,settings_index);
								need_redraw=false;
								clear_input();
							}
						}
						break;
					case MENU_SETTINGS:
						if (is_new_screen){
							draw_config_menu((SCREEN_W/2)-(14*FONT_W),(SCREEN_H/2)-((CONFIG_MENU_ITEMS/2)*FONT_H),true,settings_index);
							is_new_screen=false;
							need_redraw=true;
							busy_wait_ms(150);
							clear_input();
						}
						if((KBD_F12)||(KBD_ESC)||(data_joy&D_JOY_START)||(KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
							fast_menu_index=old_menu_index;
							menu_ptr--;
							old_menu_index=0;
							is_new_screen=true;
							clear_input();
							continue;
						}
						if((KBD_UP)||(data_joy==D_JOY_UP)){settings_index--;need_redraw=true;}
						if((KBD_DOWN)||(data_joy==D_JOY_DOWN)){settings_index++;need_redraw=true;}
						if((KBD_LEFT)||(data_joy==D_JOY_LEFT)){menu_inc_dec=-1;need_redraw=true;}
						if((KBD_RIGHT)||(data_joy==D_JOY_RIGHT)){menu_inc_dec=1;need_redraw=true;}
						/*if(KBD_PRESS){
							//printf("settings_index:%d   menu_inc_dec:%d   settings_lines:%d\n",settings_index,menu_inc_dec,settings_lines);
						}*/
						if (settings_index<0){settings_index=settings_lines;}
						if (settings_index>settings_lines){settings_index=0;}
						if ((menu_inc_dec!=0)&&(need_redraw==true)){
								if (settings_index==0){ //Boot go to :
									cfg_boot_scr+=menu_inc_dec;
									if((cfg_boot_scr>MAX_CFG_BOOT_SCR_MODE)&&(menu_inc_dec>0)){
										cfg_boot_scr=0;
									}
									if((cfg_boot_scr>MAX_CFG_BOOT_SCR_MODE)&&(menu_inc_dec<0)){
										cfg_boot_scr=MAX_CFG_BOOT_SCR_MODE;
									}
								}							
								if (settings_index==1){ //Head Up Display:
									cfg_hud_enable+=menu_inc_dec;
									if((cfg_hud_enable>MAX_CFG_HUD_MODE)&&(menu_inc_dec>0)){
										cfg_hud_enable=0;
									}
									if((cfg_hud_enable>MAX_CFG_HUD_MODE)&&(menu_inc_dec<0)){
										cfg_hud_enable=MAX_CFG_HUD_MODE;
									}
									if(cfg_hud_enable!=current_hud_mode){
										current_hud_mode&=~0x03;
										current_hud_mode|=cfg_hud_enable;
									}
								}							
								if (settings_index==2){ //Tap load:
									cfg_tap_load_mode+=menu_inc_dec;
									if((cfg_tap_load_mode>MAX_CFG_TAP_MODE)&&(menu_inc_dec>0)){
										cfg_tap_load_mode=0;
									}
									if((cfg_tap_load_mode>MAX_CFG_TAP_MODE)&&(menu_inc_dec<0)){
										cfg_tap_load_mode=MAX_CFG_TAP_MODE;
									}
									////printf("cfg_tap_load_mode>%d\n",cfg_tap_load_mode);
								}
								if (settings_index==3){ //Ext tape load pin:
									cfg_tape_load_pin+=menu_inc_dec;
									if((cfg_tape_load_pin>MAX_CFG_TAPELOAD_PIN)&&(menu_inc_dec>0)){
										cfg_tape_load_pin=0;
									}
									if((cfg_tape_load_pin>MAX_CFG_TAPELOAD_PIN)&&(menu_inc_dec<0)){
										cfg_tape_load_pin=MAX_CFG_TAPELOAD_PIN;
									}
									////printf("cfg_tap_load_mode>%d\n",cfg_tap_load_mode);
								}
								if (settings_index==4){ //Def joy1 mode:
									cfg_def_joy1_mode+=menu_inc_dec;
									if((cfg_def_joy1_mode>MAX_CFG_JOY_MODE)&&(menu_inc_dec>0)){
										cfg_def_joy1_mode=0;
									}
									if((cfg_def_joy1_mode>MAX_CFG_JOY_MODE)&&(menu_inc_dec<0)){
										cfg_def_joy1_mode=MAX_CFG_JOY_MODE;
									}
									now_joy1_mode=cfg_def_joy1_mode;
									////printf("cfg_def_joy_mode>%d\n",cfg_def_joy_mode);
									//cfg_def_joy_mode = cfg_def_joy_mode;
								}
								if (settings_index==5){ //Def joy2 mode:
									cfg_def_joy2_mode+=menu_inc_dec;
									if((cfg_def_joy2_mode>MAX_CFG_JOY_MODE)&&(menu_inc_dec>0)){
										cfg_def_joy2_mode=0;
									}
									if((cfg_def_joy2_mode>MAX_CFG_JOY_MODE)&&(menu_inc_dec<0)){
										cfg_def_joy2_mode=MAX_CFG_JOY_MODE;
									}
									now_joy2_mode=cfg_def_joy2_mode;
									////printf("cfg_def_joy_mode>%d\n",cfg_def_joy_mode);
									//cfg_def_joy_mode = cfg_def_joy_mode;
								}
								if (settings_index==6){ //Def kbd mode:
									cfg_def_kbd_mode+=menu_inc_dec;
									if((cfg_def_kbd_mode>MAX_CFG_KBD_MODE)&&(menu_inc_dec>0)){
										cfg_def_kbd_mode=0;
									}
									if((cfg_def_kbd_mode>MAX_CFG_KBD_MODE)&&(menu_inc_dec<0)){
										cfg_def_kbd_mode=MAX_CFG_KBD_MODE;
									}
									now_kbd_mode=cfg_def_kbd_mode;
									////printf("cfg_def_joy_mode>%d\n",cfg_def_joy_mode);
									//cfg_def_joy_mode = cfg_def_joy_mode;
								}
								if (settings_index==7){	//Reboot ZX before load:
									cfg_res_before_mode+=menu_inc_dec;
									if((cfg_res_before_mode>MAX_CFG_RES_MODE)&&(menu_inc_dec>0)){
										cfg_res_before_mode=0;
									}
									if((cfg_res_before_mode>MAX_CFG_RES_MODE)&&(menu_inc_dec<0)){
										cfg_res_before_mode=MAX_CFG_RES_MODE;
									}
			
								}
								if (settings_index==8){ //Snd mode:
									cfg_sound_mode+=menu_inc_dec;
									if((cfg_sound_mode>MAX_CFG_SND_MODE)&&(menu_inc_dec>0)){
										cfg_sound_mode=0;
									}
									if((cfg_sound_mode>MAX_CFG_SND_MODE)&&(menu_inc_dec<0)){
										cfg_sound_mode=MAX_CFG_SND_MODE;
									}
			
									if(cfg_sound_mode==HARDWARE_TS){
										Deinit_PWM_175();
										Deinit_Soft_AY();
										Init_PWM_175(cfg_tspin_mode);
									} else {
										Deinit_PWM_175();
										Init_Soft_AY();
									}
									AY_reset(cfg_sound_mode);										
								}
								if (settings_index==9){ //Sound out mode:
									cfg_sound_out_mode+=menu_inc_dec;
									if((cfg_sound_out_mode>MAX_CFG_OUT_MODE)&&(menu_inc_dec>0)){
										cfg_sound_out_mode=0;
									}
									if((cfg_sound_out_mode>MAX_CFG_OUT_MODE)&&(menu_inc_dec<0)){
										cfg_sound_out_mode=MAX_CFG_OUT_MODE;
									}
								}
								if (settings_index==10){ //Soft Sound Vol:
									if((cfg_sound_mode>NO_SOUND)&&(cfg_sound_mode<HARDWARE_TS)){
										cfg_volume+=(menu_inc_dec*CFG_VOLUME_STEP);
									}
									if((cfg_volume>MAX_CFG_VOLUME_MODE)&&(menu_inc_dec>0)){
										cfg_volume=MAX_CFG_VOLUME_MODE;
									}
									if((cfg_volume<0)&&(menu_inc_dec<0)){
										cfg_volume=0;
									}
								}
								if (settings_index==11){ //Ext TS Clock Pin:
									if(cfg_sound_mode==HARDWARE_TS){
										cfg_tspin_mode+=menu_inc_dec;
									}
									if((cfg_tspin_mode>MAX_CFG_TSPIN_MODE)&&(menu_inc_dec>0)){
										cfg_tspin_mode=DEF_CFG_TSPIN_MODE;
									}
									if((cfg_tspin_mode>MAX_CFG_TSPIN_MODE)&&(menu_inc_dec<0)){
										cfg_tspin_mode=MAX_CFG_TSPIN_MODE;
									}
									if(cfg_sound_mode==HARDWARE_TS){
										Deinit_PWM_175();
										Deinit_Soft_AY();
										Init_PWM_175(cfg_tspin_mode);
									} else {
										Deinit_PWM_175();
										Init_Soft_AY();
									}
									AY_reset(cfg_sound_mode);									
								}
								if (settings_index==12){ //Ext TS Clk Speed:
									if(cfg_sound_mode==HARDWARE_TS){
										cfg_tsspeed_mode+=menu_inc_dec;
									}
									if((cfg_tsspeed_mode>MAX_CFG_TSSPEED_MODE)&&(menu_inc_dec>0)){
										cfg_tsspeed_mode=0;
									}
									if((cfg_tsspeed_mode>MAX_CFG_TSSPEED_MODE)&&(menu_inc_dec<0)){
										cfg_tsspeed_mode=MAX_CFG_TSSPEED_MODE;
									}
								}								
								if (settings_index==13){ //HW TS Main Chip:
									cfg_tschip_order+=menu_inc_dec;
									if((cfg_tschip_order>MAX_CFG_TSORDER_MODE)&&(menu_inc_dec>0)){
										cfg_tschip_order=0;
									}
									if((cfg_tschip_order>MAX_CFG_TSORDER_MODE)&&(menu_inc_dec<0)){
										cfg_tschip_order=MAX_CFG_TSORDER_MODE;
									}
								}
								if (settings_index==14){ //Video output:
									cfg_video_out+=(uint8_t)menu_inc_dec;
									if((cfg_video_out>MAX_CFG_VIDEO_MODE)&&(menu_inc_dec>0)){
										cfg_video_out=0;
									}
									if((cfg_video_out>MAX_CFG_VIDEO_MODE)&&(menu_inc_dec<0)){
										cfg_video_out=MAX_CFG_VIDEO_MODE;
									}
									//printf("cfg_video_out:%d\n",cfg_video_out);
									//if((g_out)cfg_video_out<g_out_TFT_ST7789){
										//printf("Test framerate begin \n");
										if(current_video_out!=cfg_video_out){
											cfg_frame_rate=0;
											uint8_t test=10;
											while(!graphics_try_framerate((g_out) cfg_video_out,(fr_rate) cfg_frame_rate, false)){
												cfg_frame_rate+=1;
												if((cfg_frame_rate>MAX_CFG_VIDEO_FREQ_MODE)){
													cfg_frame_rate=0;
												}
												test--;
												if(test==0) break;
											}
										}
										//printf("Test framerate end \n");
									//}
								}	
								if (settings_index==15){ //Video framerate:
									cfg_frame_rate+=menu_inc_dec;
									if((cfg_frame_rate>MAX_CFG_VIDEO_FREQ_MODE)&&(menu_inc_dec>0)){
										cfg_frame_rate=0;
									}
									if((cfg_frame_rate>MAX_CFG_VIDEO_FREQ_MODE)&&(menu_inc_dec<0)){
										cfg_frame_rate=MAX_CFG_VIDEO_FREQ_MODE;
									}
									uint8_t test=10;
									while(!graphics_try_framerate((g_out) cfg_video_out,(fr_rate) cfg_frame_rate, false)){
										cfg_frame_rate+=1;
										if((cfg_frame_rate>MAX_CFG_VIDEO_FREQ_MODE)){
											cfg_frame_rate=0;
										}
										test--;
										if(test==0) break;
									}
								}
								if (settings_index==16){ //Mobile Murmulator:
									cfg_mobile_mode+=menu_inc_dec;
									if((cfg_mobile_mode>MAX_CFG_MOBILE_MODE)&&(menu_inc_dec>0)){
										cfg_mobile_mode=MAX_CFG_MOBILE_MODE;
									}
									if((cfg_mobile_mode>MAX_CFG_MOBILE_MODE)&&(menu_inc_dec<0)){
										cfg_mobile_mode=DEF_CFG_MOBILE_MODE;
									}
								}								
								#ifdef VGA_HDMI
								if((g_out)cfg_video_out>g_out_HDMI){
									if (settings_index==17){ //LCD BrightLev:
										cfg_brightness+=menu_inc_dec;
										if((cfg_brightness>MAX_CFG_BRIGHT_MODE)&&(menu_inc_dec>0)){
											cfg_brightness=MAX_CFG_BRIGHT_MODE;
										}
										if((cfg_brightness>MAX_CFG_BRIGHT_MODE)&&(menu_inc_dec<0)){
											cfg_brightness=0;
										}
										pwm_set_gpio_level(TFT_LED_PIN,(TFT_MIN_BRIGHTNESS+(cfg_brightness*10)));			//уровень подсветки TFT
									}
									if (settings_index==18){ //LCD Rotate:
										cfg_rotate+=menu_inc_dec;
										if((cfg_rotate>MAX_CFG_ROTATE)&&(menu_inc_dec>0)){
											cfg_rotate=MAX_CFG_ROTATE;
										}
										if((cfg_rotate>MAX_CFG_ROTATE)&&(menu_inc_dec<0)){
											cfg_rotate=0;
										}
									}	
									if (settings_index==19){ //LCD Inversion:
										cfg_inversion+=menu_inc_dec;
										if((cfg_inversion>MAX_CFG_INVERSION)&&(menu_inc_dec>0)){
											cfg_inversion=MAX_CFG_INVERSION;
										}
										if((cfg_inversion>MAX_CFG_INVERSION)&&(menu_inc_dec<0)){
											cfg_inversion=0;
										}
									}
									if (settings_index==20){ //LCD Pixel format:
										cfg_pixels+=menu_inc_dec;
										if((cfg_pixels>MAX_CFG_PIXELS)&&(menu_inc_dec>0)){
											cfg_pixels=MAX_CFG_PIXELS;
										}
										if((cfg_pixels>MAX_CFG_PIXELS)&&(menu_inc_dec<0)){
											cfg_pixels=0;
										}
									}
								}
								#endif
								menu_inc_dec=0;
							}
						if((KBD_ENTER)||(data_joy==D_JOY_A)){
							if(init_fs){
								if ((settings_index==12)&&(cfg_tsspeed_mode==TSSPEED_MODE_CUST)){
									memset(temp_msg,0,sizeof(temp_msg));
									sprintf(temp_msg,"%07d",0);
									if(EditDialogBox("[SETUP]","Enter clock speed:",temp_msg,CL_GREEN,COLOR_BACKGOUND,DIALOG_OK_CAN,true)==DLG_RES_OK){
										printf("%s \n",temp_msg);
									}
									busy_wait_ms(150);
									need_redraw = true;
								}
								if(settings_index==settings_lines-4){
									config_read();
									settings_index = 0;
									MessageBox("SETTINGS"," LOADED ",CL_LT_YELLOW,CL_BLUE,3);
									is_new_screen = true;
								}
								if(settings_index==settings_lines-3){
									config_write_defaults();
									config_read();
									settings_index = 0;
									MessageBox("SETTINGS"," RESET TO DEFAUTS ",CL_LT_YELLOW,CL_BLUE,2);
									is_new_screen = true;
								}
								if(settings_index==settings_lines-2){
								#ifdef VGA_HDMI
									if((g_out)cfg_video_out>g_out_HDMI){
										cfg_lcd_video_out=cfg_video_out;
										//printf("Set vout: %d \n",cfg_video_out);
									}
								#endif
									config_save();
									settings_index = 0;
									MessageBox("SETTINGS"," ALL SAVED ",CL_LT_YELLOW,CL_BLUE,4);
									if ((current_frame_rate!=cfg_frame_rate)||
										(current_video_out!=cfg_video_out)||
										(current_rotate!=cfg_rotate)||
										(current_inversion!=cfg_inversion)||
										(current_pixels!=cfg_pixels)||
										(current_pin!=cfg_tape_load_pin)||
										(current_sound_out!=cfg_sound_out_mode)||
										(current_mobile_mode!=cfg_mobile_mode)
									){
										//MessageBox("Critical settings changed!!!   ","        Please REBOOT!!!       ",CL_LT_YELLOW,CL_BLUE,1);
										MessageBox("SETTINGS","REBOOT DEVICE",CL_LT_YELLOW,CL_GRAY,1);
										busy_wait_ms(500);
										software_reset();
									}									
									is_new_screen = true;
								}
							}
							if(settings_index==settings_lines-1){
								MessageBox("SETTINGS","REBOOT DEVICE TO EXIT FIRMWARE UPDATE",CL_LT_GREEN,CL_BLUE,1);
								busy_wait_ms(500);
								reset_usb_boot(0,0);
							}
							if(settings_index==settings_lines){
								//MessageBox("SETTINGS","REBOOT DEVICE",CL_LT_BLUE,CL_YELLOW,1);
								MessageBox("SETTINGS","REBOOT DEVICE",CL_LT_YELLOW,CL_GRAY,1);
								busy_wait_ms(500);
								software_reset();
							}
						}
						/*if(((KBD_ESC)||(data_joy&D_JOY_START))){
								paused=10;
								is_fast_menu_mode=false;
								is_pause_mode=false;
								go_menu_mode=false;
								is_emulation_mode=true;
								zx_machine_enable_vbuf(true);
								//printf("exit settings");
								continue;
							}
						if((is_menu_mode)&&(kb_st_ps2.u[1]&KB_U1_ESC)){
								is_settings_mode=false;
								is_new_screen=true;
								go_menu_mode=true;		
								//printf("exit settings-esc");
								continue;					
							}
						*/
						if(need_redraw){
							draw_config_menu((SCREEN_W/2)-(14*FONT_W),(SCREEN_H/2)-((CONFIG_MENU_ITEMS/2)*FONT_H),false,settings_index);
							need_redraw=false;
							clear_input();
						}
						break;
					case MENU_POKE:
						memset(temp_msg,0,sizeof(temp_msg));
						sprintf(temp_msg,"%05d",0);
						if(EditDialogBox("[POKE]","Please enter address:",temp_msg,CL_BLACK,CL_WHITE,DIALOG_OK_CAN,true)==DLG_RES_OK){
							clear_input();
							uint32_t r_addr = atoi(temp_msg);
							if((r_addr>=0x4000)&&(r_addr<0xFFFF)){
								sprintf(temp_msg,"%03d",read_zx_mem((uint16_t) r_addr));
								if(EditDialogBox("[POKE]","Please enter value:",temp_msg,CL_BLACK,CL_WHITE,DIALOG_OK_CAN,true)==DLG_RES_OK){
									uint16_t r_val = atoi(temp_msg);
									if(r_val<0x0100){
										write_zx_mem((uint16_t)r_addr, (uint8_t) r_val);
										MessageBox("SUCCESS","Memory writed.",CL_LT_GREEN,CL_WHITE,1);
									} else {
										MessageBox("Error!!!","Wrong VALUE!!!",CL_LT_YELLOW,CL_LT_RED,1);
									}
								}
							} else {
								MessageBox("Error!!!","Wrong ADDRESS!!!",CL_LT_YELLOW,CL_LT_RED,1);
							}
						}
						menu_ptr=0;
						menu_mode[menu_ptr]=EMULATION;
						clear_input();
						fast_mode_ptr=0;
						fast_menu_index=0;
						//show_hud=old_show_hud;
						//current_hud_mode&=~HM_KBD_HUD;
						old_menu_index=0;
						is_new_screen=true;
						continue;
						break;
					case MENU_POKE_FILE:
						#ifndef DEBUG_DISABLE_LOADERS
						if (is_new_screen){
							draw_pokes_list(pokes_buff,poke_count,lineStart,settings_index,true);
							//draw_rect(PREVIEW_POS_X,SCREEN_H-(FONT_H*4),PREVIEW_WIDTH,(FONT_H*3),COLOR_FILE_BG,true);
							is_new_screen=false;
							need_redraw=true;
							busy_wait_ms(150);
							clear_input();
						}					
						/*--Return from Menu--*/
						if((KBD_F12)||(KBD_ESC)||(KBD_F1)||((data_joy&D_JOY_START)&&(hat_switch==0))||(KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
							menu_ptr--;
							need_redraw=true;
							is_new_screen=true;
							clear_input();
							continue;
						}							
						/*--Return from Menu--*/
						if(((KBD_DOWN)||(data_joy==D_JOY_DOWN))&&(settings_index<(poke_count+BTN_POS_MAX))){
							settings_index++;
							need_redraw=true;
						};
						if(((KBD_UP)||(data_joy==D_JOY_UP))&&(settings_index>0)){
							settings_index--;
							need_redraw=true;
						};
						if(((KBD_PAGE_UP)||(KBD_LEFT)||(data_joy==D_JOY_LEFT))&&(settings_index>0)){
							if((settings_index)<(poke_count-1)){
								settings_index-=5;
							} else {
								settings_index--;
							}
							need_redraw=true;
						};
						if((KBD_PAGE_DOWN)||(KBD_RIGHT)||(data_joy==D_JOY_RIGHT)){
							if((settings_index)<(poke_count-1)){
								settings_index+=5;
							} else {
								settings_index++;
							}
							need_redraw=true;
						}
						if (settings_index<0) settings_index=0;
						if (settings_index>=((poke_count-1)+BTN_POS_MAX))settings_index=((poke_count-1)+BTN_POS_MAX);
						for (short int i=((poke_count-1)+BTN_POS_MAX); i--;){
							if ((settings_index-lineStart)>=(PREVIEW_PAGE_5x7-1)) lineStart++;
							if ((settings_index-lineStart)<0) lineStart--;
						}
						btn_pos = settings_index-(poke_count-1);
						if(btn_pos>BTN_POS_MAX){
							btn_pos=1;
							settings_index=poke_count;
						};

						if((KBD_ENTER)||(data_joy==D_JOY_A)){
							if(btn_pos<1){
								pokes_buff[lineStart+settings_index].checked^=1;
							}
							if(btn_pos==1){
								POKE_LINE* set_pokes = pokes_buff;
								for(uint8_t i=0;i<poke_count;i++){set_pokes->checked=true;*set_pokes++;}
							}
							if(btn_pos==2){
								POKE_LINE* set_pokes = pokes_buff;
								for(uint8_t i=0;i<poke_count;i++){set_pokes->checked=false;*set_pokes++;}
							}
							if(btn_pos==3){
								set_pok_values(pokes_buff,poke_count,activefilename);
								MessageBox("SUCCESS","Memory writed.",CL_LT_GREEN,CL_WHITE,1);
								menu_ptr=0;
								menu_mode[menu_ptr]=EMULATION;
								clear_input();
								fast_mode_ptr=0;
								fast_menu_index=0;
								old_menu_index=0;
								is_new_screen=true;
								continue;
							}
							if(btn_pos==4){
								//memset(temp_buffer_x, 0, TEMP_BUFF_SIZE_X);
								memset(temp_buffer_y, 0, TEMP_BUFF_SIZE_Y);
								menu_ptr--;
								need_redraw=true;
								is_new_screen=true;
								clear_input();
								continue;
							}
							need_redraw=true;
						}


						////printf("need_redraw:%d lineStart:%d\n",need_redraw,lineStart);
						if (data_joy>0){ //***********************
							old_data_joy=0;
							if(Joystics.Present_WII_joy){
								Wii_clear_old();
							}
						};
						if(need_redraw){
							//printf("lineStart:%d   settings_index:%d   btn_pos:%d\n",lineStart,settings_index,btn_pos);
							draw_pokes_list(pokes_buff,poke_count,lineStart,settings_index,true);
							short int linePos=lineStart+settings_index;
							if (linePos<0) linePos=0;
							if (linePos>(poke_count-1)) linePos=(poke_count-1);
							short int shft=(207*linePos)/(poke_count-1); 
							draw_rect(PREVIEW_POS_X+PREVIEW_WIDTH-(FONT_W-1),PREVIEW_POS_Y-1,FONT_W,SCREEN_H-(FONT_H*3)+2,COLOR_MAIN_RAMK,false);//Рамка полосы прокрутки //FONT_W+1+FONT_W*37
							draw_rect(PREVIEW_POS_X+PREVIEW_WIDTH-(FONT_W-2),PREVIEW_POS_Y,FONT_W-2,SCREEN_H-(FONT_H*3),COLOR_BACKGOUND,true);  //Заливка фона полосы прокрутки //FONT_W+2+FONT_W*37
							draw_rect(PREVIEW_POS_X+PREVIEW_WIDTH-(FONT_W-3),shft+17,4,5,COLOR_TEXT,true);  //указатель полосы прокрутки //FONT_W+3+FONT_W*37
							draw_rect(PREVIEW_POS_X,(SCREEN_H-(FONT_H*3)-1),PREVIEW_WIDTH-(FONT_W-1),(FONT_H*2)+1,COLOR_FILE_BG,true);
							draw_pokes_bottom_btn(PREVIEW_POS_X,(SCREEN_H-(FONT_H*3)),btn_pos);
							need_redraw=false;
							clear_input();
							continue;
						}
						#endif
						break;
					default:
						break;
				}//switch (menu_mode[menu_ptr])
				/*SHOW */
				if(menu_mode[menu_ptr]==MENU_MAIN){
					if((init_fs)&&(last_action>0)&&(my_millis()-last_action)>SHOW_SCREEN_DELAY){
						////printf("Timers1: LA:%d GB:%d\n",last_action,my_millis());
						if(sizeof(sd_buffer)<ZX_RAM_PAGE_SIZE){
							need_reset_after_menu=true;
						}
						last_action=0;
						//if (!files[cur_file_index][(FILE_NAME_LEN-1)]){
							scroll_lfn = true;
							scroll_action = my_millis();
							scroll_pos=0;
						/*} else {
							scroll_lfn = false;
							scroll_action = 0;
							scroll_pos=0;
						}*/
						draw_rect(PREVIEW_POS_X,SCREEN_H-(FONT_H*4),PREVIEW_WIDTH,(FONT_H*3),COLOR_BACKGOUND,true);//Фон отображения информации о файле //COLOR_BACKGOUND
						//const char* ext = get_file_extension(files[cur_file_index]);
						file = (FileRec*)&files[cur_file_index];
						memset(temp_msg,0x00,sizeof(temp_msg));
						strncpy(temp_msg,file->filename,FILE_NAME_LEN-1);
						const char* ext = get_file_extension(temp_msg);
						////printf("ext:%s\n",ext);
						strcpy(activefilename,dir_path);
						strcat(activefilename,"/");
						strcat(activefilename,current_lfn);
						display_file_index=cur_file_index;
						#ifndef DEBUG_DISABLE_LOADERS
						if(strcasecmp(ext, "z80")==0) {
							//printf("LoadScreenshot: %s\n",activefilename);
							//if(LoadScreenFromZ80Snapshot(activefilename)) //printf("show - OK \n"); else //printf("screen not found");
							if(!LoadScreenFromZ80Snapshot(activefilename)){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
								draw_mur_logo();
							}
							continue;
						} else
						if(strcasecmp(ext, "sna") == 0) {
							//printf("LoadScreenshot: %s\n",activefilename);
							draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
							if(!LoadScreenFromSNASnapshot(activefilename)){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
								draw_mur_logo();
							}
							continue;
						} else
						#endif
						if(strcasecmp(ext, "scr") == 0) {
							//printf("LoadScreenshot: %s\n",activefilename);
							draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
							if(!LoadScreenshot(activefilename,false)){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
								draw_mur_logo();
							}
							continue;
						} else
						if(strcasecmp(ext, "tap") == 0) {
							//printf("LoadScreenshot: %s\n",activefilename);
							draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_BACKGOUND,true);
							if(!LoadScreenFromTap(activefilename,show_screen)){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
								draw_mur_logo();
							}
							continue;
						} else

						#ifdef TRDOS_COMPILE
						if(strcasecmp(ext, "trd") == 0) {
							//printf("LoadScreenshot: %s\n",activefilename);
							draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_BACKGOUND,true);
							if(!LoadScreenFromTRD(activefilename)){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
								draw_mur_logo();
							}
							continue;
						} else
						#endif
						if(strcasecmp(ext, "txt") == 0) {
							//printf("LoadScreenshot: %s\n",activefilename);
							draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
							if(!LoadTxt(activefilename)){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
								draw_mur_logo();
							}
							continue;
						} else
						#ifndef DEBUG_DISABLE_LOADERS
						if(strcasecmp(ext, "pok") == 0) {
							//printf("LoadScreenshot: %s\n",activefilename);
							draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
							if(poke_count=load_pok_captions(activefilename)){
								draw_pokes_list(pokes_buff,poke_count,0,0,false);
							} else {
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
								draw_mur_logo();
							}
							continue;
						} else
						#endif
						if(strcasecmp(ext, "cfg") == 0) {
							//printf("LoadScreenshot: %s\n",activefilename);
							draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
							if(!LoadTxt(activefilename)){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
								draw_mur_logo();
							}
							continue;
						} else {
							if (display_file_index==-1){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_BACKGOUND,true);//Фон отображения скринов						
								draw_mur_logo_big(155,60,1);
							} else {
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);	
								draw_mur_logo();
								if((!(file->attr&TOP_DIR_ATTR))&&(!(file->attr&AM_DIR))){
									//printf("z>%s    %02X\n",temp_msg,file->attr);
									file_descr = sd_open_file(&sd_file,activefilename,FA_READ);
									if (file_descr!=FR_OK){sd_close_file(&sd_file);return false;}
									memset(temp_msg, 0, sizeof(temp_msg));
									sprintf(temp_msg,"FSize: %ldKb",(long)sd_file_size(&sd_file)/1024);
									draw_text_len(PREVIEW_POS_X,SCREEN_H-(FONT_H*3), temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);
									sd_close_file(&sd_file);
									memset(temp_msg, 0, sizeof(temp_msg));
								}
							};
							display_file_index=cur_file_index;
							////printf("Draw Mur Logo \n");
						}
					}

					if((init_fs)&&(scroll_lfn)&&(my_millis()-scroll_action)>(SHOW_SCREEN_DELAY/2)){
						scroll_action = my_millis();
						//uint8_t pos = cur_file_index-shift_file_index;
						if (strlen(current_lfn)>24){
							if(scroll_pos<(strlen(current_lfn)-23)){
								strncpy(temp_msg,current_lfn+scroll_pos,24);
								scroll_pos++;
							} else {
								scroll_pos=0;
							}
						} else {
							strncpy(temp_msg,current_lfn,23);
						}
						//draw_text_len(2*FONT_W,2*FONT_H+pos*FONT_H,temp_msg,COLOR_SELECT_TEXT,COLOR_CURRENT_BG,(FILE_NAME_LEN-1));
						draw_text_len(PREVIEW_POS_X,SCREEN_H-PREVIEW_POS_Y, temp_msg,COLOR_TEXT,COLOR_FILE_BG,24);//COLOR_BACKGOUND
					}
				}
				////printf("mel>%d\n",my_millis()-main_loop);
				//main_loop = my_millis();				
				if(ticker>131070){ticker=0;}
			}while(menu_mode[menu_ptr]<EMULATION); //while(1) main menu loop
		}
		//END MENU LOOP
		
		#ifdef DEBUG_BLINK
		if((my_millis()-timer_update)>TIMER_PERIOD){
			timer_update=my_millis();
			gpio_put(WORK_LED_PIN,1);
			////printf("Time is:%d \n",my_millis());
			busy_wait_ms(50);
			gpio_put(WORK_LED_PIN,0);
			busy_wait_ms(50);
		};
		#endif

		//printf("2 menu_ptr:%d menu_mode:%d\n",menu_ptr,menu_mode[menu_ptr]);
		//memcpy(zx_cpu_ram[0],&RAM[0],16384);
		//zx_cpu_ram[0]=&RAM[0];

		if(menu_mode[menu_ptr]==EMULATION){
			memset(hud_line,0x11,SCREEN_W);
			//graphics_set_hud_handler(hud_ptr[current_hud]);
			//printf("HM1>[%04X]\n",current_hud_mode);
			if(current_hud_mode&HM_ON){
				current_hud_mode|=HM_MAIN_HUD;
				old_hud_mode=current_hud_mode;
				hud_timer=0;
			};
			if(current_hud_mode&HM_TIME){
				current_hud_mode|=HM_MAIN_HUD;
				old_hud_mode=current_hud_mode;
				hud_timer = my_millis();
			};
			if(current_hud_mode&HM_SHOW_BATTERY)	{hud_ptr=&hud_battery;};
			if(current_hud_mode&HM_MAIN_HUD)		{hud_ptr=&hud_prepare;};
			if(current_hud_mode&HM_TAPE_HUD)		{hud_ptr=&hud_prepare_tape;};
			if(current_hud_mode&HM_KBD_HUD)			{hud_ptr=&hud_prepare_kbd;};
			if(current_hud_mode&HM_SHOW_VOLUME)		{hud_ptr=&hud_prepare_scale;};
			if(current_hud_mode&HM_SHOW_BRIGHT)		{hud_ptr=&hud_prepare_scale;};
			if(current_hud_mode&HM_SHOW_KEYLOCK)	{hud_ptr=&hud_kb_lock;};
			graphics_set_hud_handler(hud_ptr);
			current_hud_mode=old_hud_mode;
			//printf("HM2>[%04X]\n",current_hud_mode);
			
			menu_ptr=0;
			fast_mode_ptr=0;
			/*if(old_show_hud!=show_hud){
				show_hud=old_show_hud;
			}*/

			printf("Enter Emulation Mode\n");
			zx_machine_enable_vbuf(true);
			if (im_z80_stop){
				resume_pause();				
			}
			if(need_reset_after_menu){
				zx_machine_reset(false);
			}
			if(hat_switch==0) clear_input();
			//BEGIN EMULATION LOOP
			main_loop = time_us_32();
			//printf(">hat_switch:[%02X]\n",hat_switch);
			ticker=0;
			disk_action=time_us_32();
			tape_action=time_us_32();
			int tapePrevBlock=0;
			#ifndef DEBUG_DISABLE_LOADERS
			if (tape_disp>0){
				if(cfg_tap_load_mode==AUTO_LOAD_TAP){
					tap_loader_active|=TAPE_INTERNAL_AUTO;
					//printf("TAP autoplay ON\n");
					TAP_Play();
				} else if(cfg_tap_load_mode==NORM_LOAD_TAP){
					tap_loader_active|=TAPE_INTERNAL_MANU;
					//printf("TAP play manual\n");
				}
			}
			#endif
			do{
				//printf("tap_loader_active>[%04X]\n",tap_loader_active);
				ticker++;
				if ((ticker%4096)==0){
					gpio_put(WORK_LED_PIN,0);
					//6printf("%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",mixL,mixR,outL,outR,beeper_signed,tape_signed,beep_data,beep_data_old);
					//printf("HM1>[%04X] %d\n",current_hud_mode,hud_timer);
				}
				if ((ticker%300000)==0){
					if(current_hud_mode&HM_SHOW_BATTERY){
						get_battery_stats();
					}
				}
			
				/*--TR-DOS indicator--*/
				#ifdef TRDOS_COMPILE
				if((disk_action>0)&&(time_us_32()-disk_action)>(SHOW_SCREEN_DELAY*75)){
					disk_action = time_us_32();
					for (uint8_t dr=0;dr<4;dr++){
						if ((GetWD1793_Drive_Load(dr)>0)&&(drives_status[dr]!=ICON_DISK_LOAD)){
							drives_status[dr]=ICON_DISK_LOAD;
						} 
						if((drives_status[dr]==ICON_DISK_OFF)){ 
							drives_status[dr]=ICON_DISK_OFF;
						}
						if (!TRDOS_disabled){
							if(GetWD1793_Drive()==dr){
								if ((GetWD1793_Status()==1)&&(drives_status[dr]!=ICON_DISK_READ)){
									drives_status[dr]=ICON_DISK_READ;
									if(current_hud_mode==HM_OFF){
										hud_timer = 0;
									} else
									if(current_hud_mode&HUD_TIME){
										hud_timer = my_millis();
										current_hud_mode|=HM_MAIN_HUD;
									};
									gpio_put(WORK_LED_PIN,1);
									/*
									i2s_out((int)(((int)(0)*4)*(cfg_volume/CFG_VOLUME_STEP)),(int)(((int)(0)*4)*(cfg_volume/CFG_VOLUME_STEP)));
									busy_wait_us(125);
									i2s_out((int)(((int)(128)*4)*(cfg_volume/CFG_VOLUME_STEP)),(int)(((int)(128)*4)*(cfg_volume/CFG_VOLUME_STEP)));
									busy_wait_us(125);
									i2s_out((int)(((int)(0)*4)*(cfg_volume/CFG_VOLUME_STEP)),(int)(((int)(0)*4)*(cfg_volume/CFG_VOLUME_STEP)));
									*/
								}
								if ((GetWD1793_Status()==2)&&(drives_status[dr]!=ICON_DISK_WRITE)){
									drives_status[dr]=ICON_DISK_WRITE;
									if(current_hud_mode==HM_OFF){
										hud_timer = 0;
									} else
									if(current_hud_mode&HUD_TIME){
										hud_timer = my_millis();
										current_hud_mode|=HM_MAIN_HUD;
									};
									gpio_put(WORK_LED_PIN,1);
								}					
							}
						}
					}
				}
				//zx_machine_enable_vbuf(true);
				#endif
				/*--TR-DOS indicator--*/
				//if(current_hud_mode&HM_TAPE_HUD){ //&&(!allow_repaint)					
				/*--Tape load indicators--*/
					if (tape_disp>0){

						#ifndef DEBUG_DISABLE_LOADERS
						if((tap_loader_active&TAPE_ROM_READY)&&(tapeFileSize>0)){
							tap_loader_active&=~TAPE_ROM_READY;
							if((cfg_tap_load_mode==FAST_LOAD_TAP)&&(!(tap_loader_active&TAPE_INTERNAL_ROM))){
								tap_loader_active|=TAPE_INTERNAL_ROM;
								tap_loader_active&=~TAPE_INTERNAL_AUTO;
								printf("TAP Flash Load\n");
								//zx_machine_set_pc(0x0556);
							}							
							if((cfg_tap_load_mode==AUTO_LOAD_TAP)&&(!(tap_loader_active&TAPE_INTERNAL_AUTO))){
								tap_loader_active|=TAPE_INTERNAL_AUTO;
								tap_loader_active&=~TAPE_INTERNAL_ROM;
								printf("TAP autoplay ON\n");
								TAP_Play();
							} 
						}
						if((cfg_tap_load_mode==FAST_LOAD_TAP)&&(tapeFileSize==0)&&(tap_loader_active == TAPE_OFF)){
							tape_disp=0;
						}
						if((old_tape_disp!=tape_disp)&&(tape_disp==2)){
							old_tape_disp = tape_disp;
							current_hud_mode|=HM_TAPE_HUD;
							printf("On Tape HUD\n");
						}
						if((old_tape_disp!=tape_disp)&&(tape_disp==1)){
							old_tape_disp = tape_disp;
							current_hud_mode&=~HM_TAPE_HUD;
							printf("Off Tape HUD\n");
						}

						if((tape_action>0)&&(time_us_32()-tape_action)>(SHOW_SCREEN_DELAY*250)){
							//printf("mm:[%08d]   la:[%08d]  TS:[%02X]   td:[%04d]\n",my_millis(),tape_action,TapeStatus,tape_disp);
							if(tapePrevBlock!=tapeCurrentBlock){
								current_hud_mode|=HM_TAPE_HUD;
								tapePrevBlock=tapeCurrentBlock;
								hud_timer=my_millis();
							}
							tape_action = time_us_32();
							tap_block_percent = round((tapeTotByteCount*100)/tapeFileSize);//относительно 100%
							////printf("Loaded [%%%d]\n",tap_block_percent);
							memset(hud_text,0,sizeof(hud_text));
							TapeBlock* item = (TapeBlock*)&temp_buffer_y[sizeof(TapeBlock)*tapeCurrentBlock];
							char icon[3] = {0x20,0x20,0x20};
							switch (TapeStatus) {
								case TAPE_STOPPED:
									icon[0] = 0xF4;
									icon[1] = 0xF2;
									icon[2] = 0xF4;
									tape_disp=1;
									//current_hud_mode&=~HM_TAPE_HUD;
								   	break;
								case TAPE_LOADING:
									icon[0] = 0xF2;
									icon[1] = 0xF3;
									icon[2] = 0xF4;
									tape_disp=2;
									//current_hud_mode|=HM_TAPE_HUD;
								   	break;
								case TAPE_PAUSED:
									icon[0] = 0xF3;
									icon[1] = 0xF2;
									icon[2] = 0xF4;
									tape_disp=1;
									//current_hud_mode&=~HM_TAPE_HUD;
									break;
								case TAPE_LOADED:
									icon[0] = 0xFD;
									icon[1] = 0xF2;
									icon[2] = 0xF8;
									tape_disp=1;
									tape_action=0;
									//current_hud_mode&=~HM_TAPE_HUD;
									break;
							}
							if((tapeCurrentBlock>0)&&(TapeStatus==TAPE_STOPPED)){
								icon[2] = 0xF8;
							}
							if(cfg_tap_load_mode<FAST_LOAD_TAP){
								sprintf(&hud_text[0],"%c [%03d%%]:%03d-%03d/%03dKb     F5:%c F6:%c F7:\xF5 F8:\xF6 F9:\xF7",icon[0],tap_block_percent,tapeCurrentBlock+1,tapBlocksCount+1,(item->Size/1024),icon[1],icon[2]);
								memset(temp_msg,0x00,sizeof(temp_msg));
								strncpy(temp_msg,current_lfn,38);
								sprintf(&hud_text[HUD_TEXT_LINE_LEN],"%-38s {%10s}",temp_msg,item->Flag==0?item->NAME:"DATA BYTES");
							} else if(cfg_tap_load_mode==FAST_LOAD_TAP){
								memset(temp_msg,0x00,sizeof(temp_msg));
								strncpy(temp_msg,current_lfn,51);
								sprintf(&hud_text[0],"%-51s",temp_msg);
							}
							//draw_logo_header(165,224);
						}
						#endif
					}
				/*--Tape load indicators--*/
				//}
				/*--Show Volume Ind--*/
				if((current_hud_mode&HM_SHOW_BRIGHT)||(current_hud_mode&HM_SHOW_VOLUME)){
					if(current_hud_mode&HM_SHOW_VOLUME){
						if(cfg_volume<=MAX_CFG_VOLUME_MODE){
							vol_strength=3;
						}
						if(cfg_volume<216){
							vol_strength=2;
						}
						if(cfg_volume<56){
							vol_strength=1;
						}
						if(cfg_volume<1){
							vol_strength=0;
						}						
						vol_bank = AY_get_ampl();
					}					
					if((hud_timer>0)&&(my_millis()-hud_timer)>(SHOW_SCREEN_DELAY)){
						hud_timer=0;
						printf("Vol/Bright timer off\n");
						current_hud_mode=old_hud_mode;
						current_hud_mode&=~HM_SHOW_VOLUME;
						current_hud_mode&=~HM_SHOW_BRIGHT;
						//printf("HM3>[%04X]\n",current_hud_mode);
					}

				};
				/*--Show Volume Ind--*/

				/*--KEYBOARD LOCK--*/
				if(current_hud_mode&HM_SHOW_KEYLOCK){
					if(kbd_lock){
						if((hud_timer>0)&&(my_millis()-hud_timer)>(SHOW_SCREEN_DELAY*2)){
							#ifdef VGA_HDMI
							if(cfg_mobile_mode==MOBILE_MURM_ON){
								pwm_set_gpio_level(TFT_LED_PIN,0);			//уровень подсветки TFT
							}
							#endif
							printf("Screen OFF\n");
							hud_timer=0;
							continue;
						}
					} else {
						#ifdef VGA_HDMI
						if(cfg_mobile_mode==MOBILE_MURM_ON){
							pwm_set_gpio_level(TFT_LED_PIN,(TFT_MIN_BRIGHTNESS+(cfg_brightness*10)));			//уровень подсветки TFT
						}
						#endif
						if((hud_timer>0)&&(my_millis()-hud_timer)>(SHOW_SCREEN_DELAY*2)){
							printf("Screen ON\n");
							current_hud_mode&=~HM_SHOW_KEYLOCK;
							hud_timer=0;
							continue;
						}
					}

				}
				/*--KEYBOARD LOCK--*/



				/*--HUD Switch--*/
				if((current_hud_mode&HM_MAIN_HUD)||(current_hud_mode&HM_TAPE_HUD)){
					if((hud_timer>0)&&(my_millis()-hud_timer)>(SHOW_SCREEN_DELAY*(tape_disp<2?2:4))){ 
						
						//printf("Main/Tape timer off\n");
						current_hud_mode=old_hud_mode;
						if(current_hud_mode&HM_ON){
							if(current_hud_mode&HM_MAIN_HUD)		{hud_ptr=&hud_prepare;};
							if(current_hud_mode&HM_TAPE_HUD)		{hud_ptr=&hud_prepare_tape;};
							if(current_hud_mode&HM_KBD_HUD)			{hud_ptr=&hud_prepare_kbd;};
							if(current_hud_mode&HM_SHOW_VOLUME)		{hud_ptr=&hud_prepare_scale;};
							if(current_hud_mode&HM_SHOW_BRIGHT)		{hud_ptr=&hud_prepare_scale;};
							if(current_hud_mode&HM_SHOW_KEYLOCK)	{hud_ptr=&hud_kb_lock;};
							//printf("HM4>[%04X]\n",current_hud_mode);
							//if((current_hud_mode&HM_SHOW_KEYLOCK)&&(kbd_lock)){
							//	hud_timer=my_millis();
							//}
							/*else {
								hud_timer=0;
							}*/
							//printf("HM>[%04X] %d\n",(current_hud_mode&HM_SHOW_KEYLOCK),hud_timer);
						} else if(current_hud_mode&HM_TIME){
							//printf("HM5>[%04X]\n",current_hud_mode);
							current_hud_mode&=~HM_MAIN_HUD;
							current_hud_mode&=~HM_TAPE_HUD;
							hud_ptr=NULL;
						}
					}
				}
				if(current_hud_mode!=old_hud_mode){
					if(current_hud_mode==HM_OFF)			{hud_ptr=NULL;};
					if(current_hud_mode&HM_SHOW_BATTERY)	{hud_ptr=&hud_battery;};
					if(current_hud_mode&HM_MAIN_HUD)		{hud_ptr=&hud_prepare;};
					if(current_hud_mode&HM_TAPE_HUD)		{hud_ptr=&hud_prepare_tape;};
					if(current_hud_mode&HM_KBD_HUD)			{hud_ptr=&hud_prepare_kbd;};
					if(current_hud_mode&HM_SHOW_VOLUME)		{hud_ptr=&hud_prepare_scale;};
					if(current_hud_mode&HM_SHOW_BRIGHT)		{hud_ptr=&hud_prepare_scale;};
					if(current_hud_mode&HM_SHOW_KEYLOCK)	{hud_ptr=&hud_kb_lock;};	

					graphics_set_hud_handler(hud_ptr);
					old_hud_mode=current_hud_mode;
					//printf("HM6>[%04X]\n",current_hud_mode);
					//printf("tape_disp>[%04d]\n",tape_disp);
					
				}
				/*--HUD Switch--*/

				//if ((ticker%4096)==0){ //4608
				if (ack_input){
					//ticker=0;
					//memset(zx_write_buffer->kb_data,0,8);
					process_input();
					if(!kbd_lock){
						memset(zx_write_buffer->kb_data,0,8);
					}
					/*if((keyPressed)&&(menu_mode[menu_ptr]==EMULATION)){
						
					}*/
					
					/*
					if(KBD_PRESS){
						memset(temp_msg,0,sizeof(temp_msg));
						keys_to_str(temp_msg,' ',kb_st_ps2);
						//printf("0> kbd[0][%08lX]   kbd[1][%08lX]   kbd[2][%08lX]   kbd[2][%08lX] [%s]\n",kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3],temp_msg);
						printf("key>[%s]\n",temp_msg);
					}
					*/

					/*
					//DUMP KBD
					printf("i2cKbdMode:%d  ",i2cKbdMode);
					for (uint8_t i=0;i<8;i++){
						printf("kb[%d]:[%02X]   ",i,zx_write_buffer->kb_data[i]);
					}
					for (uint8_t i=0;i<4;i++){
						printf("ps[%d]:[%08X]   ",i,kb_st_ps2.u[i]);
					}
					printf("\n");
					*/
					/*
						#define HAT_UP		1>>1;
						#define HAT_DOWN	1>>2;
						#define HAT_LEFT	1>>3;
						#define HAT_RIGHT	1>>4;
						#define HAT_A		1>>5;
						#define HAT_B		1>>6;
					*/
					//hat_switch=0;

					/*switch input mode*/

					if((joy_pressed)&&(hat_locked==0)){
						hat_switch|=hat_switch_process((uint16_t)data_joy);
						hat_switch|=hat_switch_process((uint16_t)(data_joy>>16));
					}
					if(!kbd_lock){
					/*switch input mode*/
						//printf("data_joy:[%08X]\that_locked:[%02X]\n",data_joy,hat_locked);
						if(hat_switch==0){
							/*MAP KBD TO KBD*/
							if(now_kbd_mode==4){ //9 - Keyboard keys maps to QAOPM keys
								if(kb_st_ps2.state==0x08){
									memset(zx_write_buffer->kb_data,0,8);
								}						
								if (KBD_UP)		{zx_write_buffer->kb_data[2]|=(1<<0);}; //Q
								if (KBD_DOWN)	{zx_write_buffer->kb_data[1]|=(1<<0);}; //A
								if (KBD_LEFT)	{zx_write_buffer->kb_data[5]|=(1<<1);}; //O
								if (KBD_RIGHT)	{zx_write_buffer->kb_data[5]|=(1<<0);}; //P
								if((KBD_R_ALT)||(KBD_R_SHIFT)||(KBD_NUM_PERIOD)||(KBD_DELETE)){zx_write_buffer->kb_data[7]|=(1<<2);}; //M
							}
							if(now_kbd_mode==3){ //8 - Keyboard keys maps to Sinclair 2 joystick
								if(kb_st_ps2.state==0x08){
									memset(zx_write_buffer->kb_data,0,8);
								}						
								if (KBD_LEFT)	{zx_write_buffer->kb_data[3]|=(1<<0);}; //1
								if (KBD_RIGHT)	{zx_write_buffer->kb_data[3]|=(1<<1);}; //2
								if (KBD_DOWN)	{zx_write_buffer->kb_data[3]|=(1<<2);}; //3
								if (KBD_UP)		{zx_write_buffer->kb_data[3]|=(1<<3);}; //4
								if((KBD_R_ALT)||(KBD_R_SHIFT)||(KBD_NUM_PERIOD)||(KBD_DELETE)){zx_write_buffer->kb_data[3]|=(1<<4);}; //5
							}
							if(now_kbd_mode==2){ //7 - Keyboard keys maps to Sinclair 1 joystick
								if(kb_st_ps2.state==0x08){
									memset(zx_write_buffer->kb_data,0,8);
								}
								if (KBD_LEFT)	{zx_write_buffer->kb_data[4]|=(1<<4);}; //6
								if (KBD_RIGHT)	{zx_write_buffer->kb_data[4]|=(1<<3);}; //7
								if (KBD_DOWN)	{zx_write_buffer->kb_data[4]|=(1<<2);}; //8
								if (KBD_UP)		{zx_write_buffer->kb_data[4]|=(1<<1);}; //9
								if((KBD_R_ALT)||(KBD_R_SHIFT)||(KBD_NUM_PERIOD)||(KBD_DELETE)){zx_write_buffer->kb_data[4]|=(1<<0);}; //0
							}
							if(now_kbd_mode==1){ //5 - Keyboard keys maps to Kempston joystick
								if(KBD_RIGHT){
									data_joy|=0b00000001;
									////printf("KBD Right\n");
								};
								if(KBD_LEFT){
									data_joy|=0b00000010;
									////printf("KBD Left\n");
								};
								if(KBD_DOWN){
									data_joy|=0b00000100;
									////printf("KBD Down\n");
								};
								if(KBD_UP){
									data_joy|=0b00001000;
									////printf("KBD Up\n");
								};
								if((KBD_R_ALT)||(KBD_R_SHIFT)||(KBD_NUM_PERIOD)||(KBD_DELETE)){
									data_joy|=0b00010000;
									////printf("KBD Alt\n");
								};
								zx_write_buffer->kempston=(uint8_t)(data_joy);
							}
							if(now_kbd_mode==0){ //6 - Keyboard keys maps to Cursor joystick
								if(kb_st_ps2.state==0x08){
									memset(zx_write_buffer->kb_data,0,8);
								}
								if (KBD_UP)		{zx_write_buffer->kb_data[0]|=(1<<0);zx_write_buffer->kb_data[4]|=(1<<3);}; //Caps + 7
								if (KBD_DOWN)	{zx_write_buffer->kb_data[0]|=(1<<0);zx_write_buffer->kb_data[4]|=(1<<4);}; //Caps + 6
								if (KBD_LEFT)	{zx_write_buffer->kb_data[0]|=(1<<0);zx_write_buffer->kb_data[3]|=(1<<4);}; //Caps + 5
								if (KBD_RIGHT)	{zx_write_buffer->kb_data[0]|=(1<<0);zx_write_buffer->kb_data[4]|=(1<<2);}; //Caps + 8
								if((KBD_R_ALT)||
									(KBD_R_SHIFT)||
									(KBD_NUM_PERIOD)||
									(KBD_DELETE)){zx_write_buffer->kb_data[0]|=(1<<0);zx_write_buffer->kb_data[4]|=(1<<0);}; //Caps + 0
								
							}			
							/*MAP KBD TO KBD*/
							
							/*MAP JOY TO KBD*/
							if(!Joystics.Present_WII_joy) reset_kmouse();
							if(Joystics.Present_WII_joy){ //Auto fire XY for WII
								if (Joy_data.Data_WII_joy&0x00000008){								// wii buttonX 0x00000008
									if(autofireX==0){
										autofireX=fire_millis();
									}
									if((fire_millis()-autofireX)>50){
										autofireX=fire_millis();
										autofire_flipX^=1;
										//printf("autofire_flip:%d\n",autofire_flip);
									}								
									if(autofire_flipX){
										data_joy|=0x20;
										joy_pressed=true;
									} else {
										data_joy&=~0x20;
										joy_pressed=true;
									}
								}							
								if (Joy_data.Data_WII_joy&0x00000002){								// wii buttonY 0x00000002
									if(autofireY==0){
										autofireY=fire_millis();
									}
									if((autofireY>0)&&(fire_millis()-autofireY)>50){
										autofireY=fire_millis();
										autofire_flipY^=1;
										//printf("autofire_flip:%d\n",autofire_flip);
									}
									if(autofire_flipY){
										data_joy|=0x10;
										joy_pressed=true;
									} else {
										data_joy&=~0x10;
										joy_pressed=true;
									}
								}
								if(!joy_pressed){
									if((autofireX>0)||(autofireY>0)){
										//printf("disable "BYTE_TO_BINARY_PATTERN"    autofireX:%ld    autofireY:%ld\n", BYTE_TO_BINARY(data_joy),autofireX,autofireY);
										if((!(Joy_data.Data_WII_joy&0x00001000))&&(data_joy&0x10)){				//wii buttonB 0x00001000
											autofire_flipY=false;
											data_joy&=~0x10;
										}
										if((!(Joy_data.Data_WII_joy&0x00002000))&&(data_joy&0x20)){				//wii buttonB 0x00002000
											autofire_flipX=false;
											data_joy&=~0x20;
										}
										autofireX=0;
										autofireY=0;
									}
								}
							}
	
							if(data_joy!=rel_data_joy){
								memset(zx_write_buffer->kb_data,0,8);
								zx_write_buffer->kempston=0;
								//rel_data_joy=data_joy;
							}

							if(now_joy1_mode==4){ //4 - External NES joystick maps to QAOPM keys
								if ((data_joy&D_JOY_UP))		{zx_write_buffer->kb_data[2]|=(1<<0);}; //Q
								if ((data_joy&D_JOY_DOWN))		{zx_write_buffer->kb_data[1]|=(1<<0);}; //A
								if ((data_joy&D_JOY_LEFT))		{zx_write_buffer->kb_data[5]|=(1<<1);}; //O
								if ((data_joy&D_JOY_RIGHT))		{zx_write_buffer->kb_data[5]|=(1<<0);}; //P
								if ((data_joy&D_JOY_B))			{zx_write_buffer->kb_data[7]|=(1<<2);}; //M
								if ((data_joy&D_JOY_A))			{zx_write_buffer->kb_data[6]|=(1<<0);}; //Enter
								if ((data_joy&D_JOY_START))		{zx_write_buffer->kb_data[7]|=(1<<3);}; //N
								if ((data_joy&D_JOY_SELECT))	{zx_write_buffer->kb_data[7]|=(1<<4);}; //B
							}
							if(now_joy1_mode==3){ //3 - External NES joystick maps to Sinclair 2 joystick
								if ((data_joy&D_JOY_LEFT))		{zx_write_buffer->kb_data[3]|=(1<<0);}; //1
								if ((data_joy&D_JOY_RIGHT))		{zx_write_buffer->kb_data[3]|=(1<<1);}; //2
								if ((data_joy&D_JOY_DOWN))		{zx_write_buffer->kb_data[3]|=(1<<2);}; //3
								if ((data_joy&D_JOY_UP))		{zx_write_buffer->kb_data[3]|=(1<<3);}; //4
								if ((data_joy&D_JOY_B))			{zx_write_buffer->kb_data[3]|=(1<<4);}; //5
								if ((data_joy&D_JOY_A))			{zx_write_buffer->kb_data[6]|=(1<<0);}; //Enter
							}
							if(now_joy1_mode==2){ //2 - External NES joystick maps to Sinclair 1 joystick
								if ((data_joy&D_JOY_LEFT))		{zx_write_buffer->kb_data[4]|=(1<<4);}; //6
								if ((data_joy&D_JOY_RIGHT))		{zx_write_buffer->kb_data[4]|=(1<<3);}; //7
								if ((data_joy&D_JOY_DOWN))		{zx_write_buffer->kb_data[4]|=(1<<2);}; //8
								if ((data_joy&D_JOY_UP))		{zx_write_buffer->kb_data[4]|=(1<<1);}; //9
								if ((data_joy&D_JOY_B))			{zx_write_buffer->kb_data[4]|=(1<<0);}; //0
								if ((data_joy&D_JOY_A))			{zx_write_buffer->kb_data[6]|=(1<<0);}; //Enter
							}
							if(now_joy1_mode==1){ //1 - External NES joystick maps to Cursor joystick
								if ((data_joy&D_JOY_UP))		{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[4]|=(1<<3);busy_wait_us(2);}; //Caps + 7
								if ((data_joy&D_JOY_DOWN))		{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[4]|=(1<<4);busy_wait_us(2);}; //Caps + 6
								if ((data_joy&D_JOY_LEFT))		{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[3]|=(1<<4);busy_wait_us(2);}; //Caps + 5
								if ((data_joy&D_JOY_RIGHT))		{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[4]|=(1<<2);busy_wait_us(2);}; //Caps + 8
								if ((data_joy&D_JOY_A))			{zx_write_buffer->kb_data[6]|=(1<<0);busy_wait_us(2);}; //Enter
								if ((data_joy&D_JOY_B))			{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[4]|=(1<<0);busy_wait_us(2);}; //Caps + 0
							}
							if(now_joy1_mode==0){ //0 - External NES joystick maps to Kempston joystick //||(now_joy1_mode>4)
								zx_write_buffer->kempston|=(uint8_t)(data_joy&0xFF);
							};			
							
							if(now_joy2_mode==4){ //4 - External NES joystick maps to QAOPM keys
								if (((data_joy>>16)&D_JOY_UP))			{zx_write_buffer->kb_data[2]|=(1<<0);}; //Q
								if (((data_joy>>16)&D_JOY_DOWN))		{zx_write_buffer->kb_data[1]|=(1<<0);}; //A
								if (((data_joy>>16)&D_JOY_LEFT))		{zx_write_buffer->kb_data[5]|=(1<<1);}; //O
								if (((data_joy>>16)&D_JOY_RIGHT))		{zx_write_buffer->kb_data[5]|=(1<<0);}; //P
								if (((data_joy>>16)&D_JOY_B))			{zx_write_buffer->kb_data[7]|=(1<<2);}; //M
								if (((data_joy>>16)&D_JOY_A))			{zx_write_buffer->kb_data[6]|=(1<<0);}; //Enter
								if (((data_joy>>16)&D_JOY_START))		{zx_write_buffer->kb_data[7]|=(1<<3);}; //N
								if (((data_joy>>16)&D_JOY_SELECT))		{zx_write_buffer->kb_data[7]|=(1<<4);}; //B
							}
							if(now_joy2_mode==3){ //3 - External NES joystick maps to Sinclair 2 joystick
								if (((data_joy>>16)&D_JOY_LEFT))		{zx_write_buffer->kb_data[3]|=(1<<0);}; //1
								if (((data_joy>>16)&D_JOY_RIGHT))		{zx_write_buffer->kb_data[3]|=(1<<1);}; //2
								if (((data_joy>>16)&D_JOY_DOWN))		{zx_write_buffer->kb_data[3]|=(1<<2);}; //3
								if (((data_joy>>16)&D_JOY_UP))			{zx_write_buffer->kb_data[3]|=(1<<3);}; //4
								if (((data_joy>>16)&D_JOY_B))			{zx_write_buffer->kb_data[3]|=(1<<4);}; //5
								if (((data_joy>>16)&D_JOY_A))			{zx_write_buffer->kb_data[6]|=(1<<0);}; //Enter
							}
							if(now_joy2_mode==2){ //2 - External NES joystick maps to Sinclair 1 joystick
								if (((data_joy>>16)&D_JOY_LEFT))		{zx_write_buffer->kb_data[4]|=(1<<4);}; //6
								if (((data_joy>>16)&D_JOY_RIGHT))		{zx_write_buffer->kb_data[4]|=(1<<3);}; //7
								if (((data_joy>>16)&D_JOY_DOWN))		{zx_write_buffer->kb_data[4]|=(1<<2);}; //8
								if (((data_joy>>16)&D_JOY_UP))			{zx_write_buffer->kb_data[4]|=(1<<1);}; //9
								if (((data_joy>>16)&D_JOY_B))			{zx_write_buffer->kb_data[4]|=(1<<0);}; //0
								if (((data_joy>>16)&D_JOY_A))			{zx_write_buffer->kb_data[6]|=(1<<0);}; //Enter
							}
							if(now_joy2_mode==1){ //1 - External NES joystick maps to Cursor joystick
								if (((data_joy>>16)&D_JOY_UP))			{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[4]|=(1<<3);busy_wait_us(2);}; //Caps + 7
								if (((data_joy>>16)&D_JOY_DOWN))		{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[4]|=(1<<4);busy_wait_us(2);}; //Caps + 6
								if (((data_joy>>16)&D_JOY_LEFT))		{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[3]|=(1<<4);busy_wait_us(2);}; //Caps + 5
								if (((data_joy>>16)&D_JOY_RIGHT))		{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[4]|=(1<<2);busy_wait_us(2);}; //Caps + 8
								if (((data_joy>>16)&D_JOY_A))			{zx_write_buffer->kb_data[6]|=(1<<0);busy_wait_us(2);}; //Enter
								if (((data_joy>>16)&D_JOY_B))			{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[4]|=(1<<0);busy_wait_us(2);}; //Caps + 0
							}
							if(now_joy2_mode==0){ //0 - External NES joystick maps to Kempston joystick //||(now_joy1_mode>4)
								zx_write_buffer->kempston|=(uint8_t)((data_joy>>16)&0xFF);
							};			
														

							/*if(now_joy1_mode<5){
								//Map PS/2 cursor keys to Spectrum cursor keys
								if((data_joy==0)&&(kb_st_ps2.state==0x08)){
									memset(zx_write_buffer->kb_data,0,8);
								}							
								if ((KBD_UP))	{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[4]|=(1<<3);busy_wait_us(2);};
								if ((KBD_DOWN))	{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[4]|=(1<<4);busy_wait_us(2);};
								if ((KBD_LEFT))	{zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[3]|=(1<<4);busy_wait_us(2);};
								if ((KBD_RIGHT)){zx_write_buffer->kb_data[0]|=(1<<0);busy_wait_us(2);zx_write_buffer->kb_data[4]|=(1<<2);busy_wait_us(2);};
							}*/					
							/*MAP JOY TO KBD*/
	
							// Kempston Mouse USB RP2040 I2C    
					        if((i2cKbdMode)&&((ibuff[0]==1)||(ibuff[0]==3)||(ibuff[0]==4))){
		          				zx_write_buffer->kempston_mouse_btn=ibuff[1];
		          				zx_write_buffer->kempston_mouse_x=ibuff[2];
		          				zx_write_buffer->kempston_mouse_y=ibuff[3];
		        			}
							//printf("kempston[%02X]\n",zx_write_buffer->kempston);
							//printf("zx[0][%02X]   zx[1][%02X]   zx[2][%02X]   zx[3][%02X]   zx[4][%02X]   zx[5][%02X]   zx[6][%02X]   zx[7][%02X]  data_joy:[%02X]   joy_pressed:[%d]\n",zx_write_buffer->kb_data[0],zx_write_buffer->kb_data[1],zx_write_buffer->kb_data[2],zx_write_buffer->kb_data[3],zx_write_buffer->kb_data[4],zx_write_buffer->kb_data[5],zx_write_buffer->kb_data[6],zx_write_buffer->kb_data[7],data_joy,joy_pressed);
							//printf("data_joy "BYTE_TO_BINARY_PATTERN"\n", BYTE_TO_BINARY(data_joy));
							//printf("data_joy:%d     kempston:%d\n",data_joy,zx_write_buffer->kempston);
						}
						//printf("u[0]:[%08lX]\tu[1]:[%08lX]\tu[2]:[%08lX]\tu[3]:[%08lX]\n",kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3]);
						
						convert_kb_u_to_kb_zx(&kb_st_ps2,zx_write_buffer->kb_data);

						
						//if(((rel_data_joy&D_JOY_MENU)==D_JOY_MENU)||(((rel_data_joy>>16)&D_JOY_MENU)==D_JOY_MENU)){
						
						//}
						if(hat_locked>0){
							if(((data_joy&D_JOY_HAT_UNLOCK)==D_JOY_HAT_UNLOCK)||(((data_joy>>16)&D_JOY_HAT_UNLOCK)==D_JOY_HAT_UNLOCK)){
								hat_locked=0;
								menu_ptr++;
								menu_mode[menu_ptr]=MENU_JOY_MAIN;
								rel_data_joy=data_joy;
								break;
							}
						}
						if((!joy_pressed)&&((rel_data_joy&D_JOY_START)||((rel_data_joy>>16)&D_JOY_START))&&(hat_switch==0)&&(hat_locked==0)){
							menu_ptr++;
							menu_mode[menu_ptr]=MENU_JOY_MAIN;
							rel_data_joy=data_joy;
							break;
						}
					}
					/*
					printf("[%010ld]>>> ZX>KB[%02X][%02X][%02X][%02X][%02X][%02X][%02X][%02X] KEMP["BYTE_TO_BINARY_PATTERN"] *** EXT>KB[%08lX][%08lX][%08lX][%08lX]:[%02X]  JOY["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]\n", 
							time_us_32()-main_loop,
							zx_write_buffer->kb_data[0],zx_write_buffer->kb_data[1],zx_write_buffer->kb_data[2],zx_write_buffer->kb_data[3],zx_write_buffer->kb_data[4],zx_write_buffer->kb_data[5],zx_write_buffer->kb_data[6],zx_write_buffer->kb_data[7],
							BYTE_TO_BINARY(zx_write_buffer->kempston),
							kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3],kb_st_ps2.state,
							BYTE_TO_BINARY(data_joy),BYTE_TO_BINARY(old_data_joy),BYTE_TO_BINARY(rel_data_joy));
					*/
					main_loop = time_us_32();
					rel_data_joy=data_joy;
					ack_input=false;
				} else {
					if(!kbd_lock){					
						if(memcmp(zx_read_buffer,zx_write_buffer,sizeof(ZX_Input_t))!=0){
							zx_machine_input_set();
						}
					}
				}
				if(hat_switch>0){
					if(current_hud_mode&HUD_TIME){hud_timer = my_millis();};
					//printf(">hat_switch:[%02X]\n",hat_switch);
					if((hat_switch&HAT_START)){
						if(hat_switch&HAT_UP){
							hat_switch&=~HAT_UP;
							if((data_joy<<16)>0)now_joy1_mode++;
							if((data_joy>>16)>0)now_joy2_mode++;
							if(now_joy1_mode>4) now_joy1_mode=0;
							if(now_joy2_mode>4) now_joy2_mode=0;
							data_joy=0;
							old_data_joy=0;
							rel_data_joy=0;
							//printf("now_joy_mode:%d\n",now_joy_mode);
							if((current_hud_mode&HM_ON)||(current_hud_mode&HM_TIME)){
								current_hud_mode|=HM_MAIN_HUD;
								hud_timer = my_millis();
							}
							busy_wait_ms(150);
							//memset(zx_write_buffer->kb_data,0,8);
						}
						if(hat_switch&HAT_DOWN){
							hat_switch&=~HAT_DOWN;
							if((data_joy<<16)>0)now_joy1_mode--;
							if((data_joy>>16)>0)now_joy2_mode--;
							if(now_joy1_mode>=5) now_joy1_mode=4;
							if(now_joy2_mode>=5) now_joy2_mode=4;
							data_joy=0;
							old_data_joy=0;
							rel_data_joy=0;
							//printf("now_joy_mode:%d\n",now_joy_mode);
							if((current_hud_mode&HM_ON)||(current_hud_mode&HM_TIME)){
								current_hud_mode|=HM_MAIN_HUD;
								hud_timer = my_millis();
							}
							busy_wait_ms(150);
							//memset(zx_write_buffer->kb_data,0,8);
						}
						if(hat_switch&HAT_A){
							hat_switch&=~HAT_A;
							menu_ptr++;
							menu_mode[menu_ptr]=MENU_KEYBOARD;
							is_new_screen=true;
							need_redraw=true;
							//memset(zx_write_buffer->kb_data,0,8);
							continue;
						}
						if(hat_switch&HAT_B){
							hat_switch&=~HAT_B;
							menu_ptr++;
							menu_mode[menu_ptr]=MENU_MAIN;
							is_new_screen=true;
							data_joy=0;
							old_data_joy=0;
							rel_data_joy=0;
							//memset(zx_write_buffer->kb_data,0,8);
							continue;
						}
						if(init_fs){
							#ifndef DEBUG_DISABLE_LOADERS
							if(hat_switch&HAT_RIGHT){
								hat_switch&=~HAT_RIGHT;
								im_ready_loading = false;
								im_z80_stop = true;
								while (im_z80_stop){
									busy_wait_ms(10);
									if (im_ready_loading){
										zx_machine_reset(false);
										AY_reset(cfg_sound_mode);// сбросить AY
										sprintf(save_file_name_image,"0:/save/QSAVE.Z80 ");
										if (load_image_z80(save_file_name_image)){
											MessageBox("QUICKLOAD","",CL_WHITE,CL_BLUE,2);
										} else {
											MessageBox("Error QUICKLOAD!!!","",CL_YELLOW,CL_LT_RED,1);
										}
										clear_input();
										im_z80_stop = false;					   
										im_ready_loading = false;
										break;
									}
								}
							}
							if(hat_switch&HAT_LEFT){
								hat_switch&=~HAT_LEFT;
								im_ready_loading = false;
								im_z80_stop = true;
								while (im_z80_stop){
									busy_wait_ms(10);
									if (im_ready_loading){
										busy_wait_ms(10);
										short int file_descr = sd_mkdir("0:/save");
										if ((file_descr!=FR_OK)&&(file_descr!=FR_EXIST) ){
											memset(temp_msg,0,sizeof(temp_msg));
											sprintf(temp_msg," Error saving QUICKSAVE");
											MessageBox("QUICKSAVE",temp_msg,CL_LT_YELLOW,CL_RED,1);
											break;	
										}
										//char slot_name[20];
										memset(temp_msg,0,sizeof(temp_msg));
										sprintf(save_file_name_image,"0:/save/QSAVE.Z80 ");
										if(save_image_z80(save_file_name_image)){
											MessageBox("QUICKSAVE","",CL_WHITE,CL_BLUE,2);
										} else {
											memset(temp_msg,0,sizeof(temp_msg));
											sprintf(temp_msg," Error saving QUICKSAVE");
											MessageBox("QUICKSAVE",temp_msg,CL_LT_YELLOW,CL_RED,1);
											break;	
										}
										busy_wait_ms(100);
										clear_input();
										im_z80_stop = false;					   
										im_ready_loading = false;
										break;
									}
								}
							}
							#endif
						}
					}
					if(hat_switch&HAT_SELECT){
						if(hat_switch&HAT_UP){
							hat_switch&=~HAT_UP;
							kb_st_ps2.u[2]|=KB_U2_NUM_PLUS;
							busy_wait_ms(50);
							//memset(zx_write_buffer->kb_data,0,8);
							printf("Vol UP\n");
						}
						if(hat_switch&HAT_DOWN){
							hat_switch&=~HAT_DOWN;
							kb_st_ps2.u[2]|=KB_U2_NUM_MINUS;
							busy_wait_ms(50);
							//memset(zx_write_buffer->kb_data,0,8);
							printf("Vol DOWN\n");
						}
						#ifdef VGA_HDMI
						if(cfg_mobile_mode==MOBILE_MURM_ON){
							if(hat_switch&HAT_RIGHT){
								hat_switch&=~HAT_RIGHT;
								cfg_brightness+=1;
								if(cfg_brightness>MAX_CFG_BRIGHT_MODE){
									cfg_brightness=MAX_CFG_BRIGHT_MODE;
								}
								pwm_set_gpio_level(TFT_LED_PIN,(TFT_MIN_BRIGHTNESS+(cfg_brightness*10)));			//уровень подсветки TFT
								hud_timer = my_millis();
								current_hud_mode|=HM_SHOW_BRIGHT;
								current_hud_mode&=~HM_SHOW_VOLUME;
								printf("Bri UP\n");
								busy_wait_ms(150);
							}
							if(hat_switch&HAT_LEFT){
								hat_switch&=~HAT_LEFT;
								cfg_brightness-=1;
								if(cfg_brightness>MAX_CFG_BRIGHT_MODE){
									cfg_brightness=0;
								}
								pwm_set_gpio_level(TFT_LED_PIN,(TFT_MIN_BRIGHTNESS+(cfg_brightness*10)));			//уровень подсветки TFT
								hud_timer = my_millis();
								current_hud_mode|=HM_SHOW_BRIGHT;
								current_hud_mode&=~HM_SHOW_VOLUME;
								printf("Bri DOWN\n");
								busy_wait_ms(150);
							}
							if(hat_switch&HAT_A){
								//hat_switch&=~HAT_A;
								hat_switch=0;
								kbd_lock=true;
								hud_timer = my_millis();
								current_hud_mode|=HM_SHOW_KEYLOCK;
								printf("KB LOCK\n");
								busy_wait_ms(50);
								continue;
							}
							if(hat_switch&HAT_B){
								//hat_switch&=~HAT_B;
								hat_switch=0;
								kbd_lock=false;
								hud_timer = my_millis();
								current_hud_mode|=HM_SHOW_KEYLOCK;
								printf("KB UNLOCK\n");
								busy_wait_ms(50);
								continue;
							}
						}
						#endif					
					}
				}
				//printf("["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]\n",BYTE_TO_BINARY(hat_switch),BYTE_TO_BINARY(rel_data_joy),BYTE_TO_BINARY(joy_pressed));
				
				/*if ((ticker%32768)==0){
					printf("E> joy_pressed:%d   data_joy:[%02X]   old_data_joy:[%02X]   rel_data_joy:[%02X]   hat_switch:[%02X] \n",joy_pressed,data_joy,old_data_joy,rel_data_joy,hat_switch);
				}*/

				if((!joy_pressed)&&(hat_switch>0)){
					hat_switch=0;
					clear_input();
				}

				/*
				if ((ticker%256)==0){
				//if((data_joy!=0)||(KBD_PRESS)){
					printf("kbd[0][%08lX]   kbd[1][%08lX]   kbd[2][%08lX]   kbd[3][%08lX] data_joy:[%02X]\n",kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3],data_joy);
					//printf("menu_mode[menu_ptr]>%d   menu_ptr>%d   fast_mode[fast_mode_ptr]>%d   fast_mode_ptr>%d   fast_menu_index>%d   old_menu_index>%d\n",menu_mode[menu_ptr],menu_ptr,fast_mode[fast_mode_ptr],fast_mode_ptr,fast_menu_index,old_menu_index);
				}*/
				/* hotkey process*/
				if(!kbd_lock)
				if(KBD_PRESS){
					/*if (KBD_SCROLL_LOCK) {
						is_skipframe^=1;
						//printf("Skip frame:%d\n",is_skipframe);
						zx_machine_slow_FR(is_skipframe);
					}*/
					if((KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
						menu_ptr++;
						menu_mode[menu_ptr]=MENU_MAIN;
						break;
					}
					/*--PAUSE EMULATION--*/
					if((KBD_PAUSE_BREAK)){
						enter_pause();
						printf("Paused\n");
						zx_machine_enable_vbuf(false);
						busy_wait_ms(10);
						MessageBox("PAUSE","Screen refresh stopped",CL_LT_GREEN,CL_BLUE,0);
						busy_wait_ms(100);
						clear_input();
						while(!wait_kbdjoy_emu()){
							busy_wait_ms(10);
						}
						printf("Resume Pause\n");
						clear_input();
						resume_pause();
						zx_machine_enable_vbuf(true);
						im_z80_stop = false;
					}
					/*--PAUSE EMULATION--*/
					/*--Flat Key Operations--*/
					if (!((KBD_L_SHIFT||KBD_R_SHIFT)||(KBD_L_CTRL||KBD_R_CTRL))){
						/*--HELP--*/
						if (KBD_F1) {
							menu_ptr++;
							menu_mode[menu_ptr]=MENU_HELP;
							need_redraw=true;
							last_action=0;
							break;
						}
						/*--HELP--*/
						/*--SAVE MENU--*/
						if (KBD_F2){
							menu_ptr++;
							menu_mode[menu_ptr]=MENU_JOY_MAIN;
							fast_mode_ptr++;
							fast_mode[fast_mode_ptr]=FAST_MENU_SAVE;
							fast_menu_index = 0;
							break;
						}
						/*--SAVE MENU--*/
						/*--LOAD MENU--*/
						if (KBD_F3){
							menu_ptr++;
							menu_mode[menu_ptr]=MENU_JOY_MAIN;
							fast_mode_ptr++;
							fast_mode[fast_mode_ptr]=FAST_MENU_LOAD;
							fast_menu_index = 0;
							break;
						}
						/*--LOAD MENU--*/
						/*--Switch joystick type--*/
						/*if (KBD_F11) {
							now_joy_mode++;
							if(now_joy_mode>=MAX_JOY_MODE){
								now_joy_mode=0;
							}
							zx_machine_enable_vbuf(false);
							MessageBox("JOYSTICK",joy_text[now_joy_mode],CL_YELLOW,CL_WHITE,4);
							zx_machine_enable_vbuf(true);
							clear_input();
						};*/
						/*--Switch joystick type--*/				
					}
					/*--Flat Key Operations--*/
					/*--Emulator reset--*/
					if (((KBD_L_CTRL)||(KBD_R_CTRL))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_DELETE)){
						clear_input();
						//printf("Restart ZX\n");
						AY_reset(cfg_sound_mode);
						WD1793_Reset(0);
						zx_machine_reset(false);
						reset_kmouse();
						now_joy1_mode=cfg_def_joy1_mode;
						#ifndef DEBUG_DISABLE_LOADERS
						tap_loader_active=TAPE_OFF;
						#endif
						busy_wait_ms(150);
						clear_input();
					}
					if (((KBD_L_CTRL)||(KBD_R_CTRL))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_TAB)){
						clear_input();
						//printf("Restart ZX\n");
						AY_reset(cfg_sound_mode);
						WD1793_Reset(0);
						zx_machine_reset(true);
						reset_kmouse();
						now_joy1_mode=cfg_def_joy1_mode;
						#ifndef DEBUG_DISABLE_LOADERS
						tap_loader_active=TAPE_OFF;
						#endif
						busy_wait_ms(150);
						clear_input();
					}					
					/*--Emulator reset--*/
					/*--Emulator NMI--*/
					if (((KBD_L_CTRL)||(KBD_R_CTRL))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_INSERT)){
						clear_input();
						//printf("NMI\n");
						zx_machine_NMI();
						busy_wait_ms(150);
						clear_input();
					}
					/*--Emulator NMI--*/
					/*--HARD reset--*/
					if (((KBD_L_SHIFT)||(KBD_R_SHIFT))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_DELETE)){ //((KBD_L_CTRL)||(KBD_R_CTRL))&&
						software_reset();
					}
					/*--HARD reset--*/
					/*--DEF reset--*/
					if (((KBD_L_CTRL)||(KBD_R_CTRL))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_BACK_SPACE)){ 
						config_write_defaults();
						busy_wait_ms(100);
						software_reset();
					}
					/*--DEF reset--*/
					/*--Volume--*/
					if((cfg_sound_mode>0)&&(cfg_sound_mode<4)){
						if ((KBD_NUM_PLUS)||(((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_PAGE_UP))){ 
							cfg_volume+=CFG_VOLUME_STEP;
							if(cfg_volume>MAX_CFG_VOLUME_MODE){
								cfg_volume=MAX_CFG_VOLUME_MODE;
							}
							current_hud_mode|=HM_SHOW_VOLUME;
							current_hud_mode&=~HM_SHOW_BRIGHT;
							hud_timer = my_millis();
							busy_wait_ms(100);							
							memset(&zx_write_buffer->kb_data,0,8);
							clear_input();
						}
						if ((KBD_NUM_MINUS)||(((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_PAGE_DOWN))){ 
							cfg_volume-=CFG_VOLUME_STEP;
							if(cfg_volume<0){
								cfg_volume=0;
							}
							current_hud_mode|=HM_SHOW_VOLUME;
							current_hud_mode&=~HM_SHOW_BRIGHT;
							hud_timer = my_millis();
							busy_wait_ms(100);
							memset(&zx_write_buffer->kb_data,0,8);
							clear_input();
						}
						if ((KBD_NUM_MULT)||(((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_END))){
							if(cfg_volume>0){
								old_volume=cfg_volume;
								cfg_volume=0;
							} else if (cfg_volume==0){
								if(old_volume>0){
									cfg_volume=old_volume;
								} else {
									cfg_volume=DEF_CFG_VOLUME_MODE;
								}
								old_volume=0;
							}
							current_hud_mode|=HM_SHOW_VOLUME;
							current_hud_mode&=~HM_SHOW_BRIGHT;
							hud_timer = my_millis();
							busy_wait_ms(100);							
							memset(&zx_write_buffer->kb_data,0,8);
							clear_input();
						}
						if ((KBD_NUM_SLASH)||(((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_DELETE))){
							AY_next_ampls();
							vol_bank = AY_get_ampl();
							busy_wait_ms(100);
							current_hud_mode|=HM_SHOW_VOLUME;
							current_hud_mode&=~HM_SHOW_BRIGHT;
							hud_timer = my_millis();
							memset(&zx_write_buffer->kb_data,0,8);
							clear_input();
						}						
						//printf("cfg_volume:%d\n",cfg_volume);
					}
					/*--Volume--*/
					/*switch input mode*/
					if(((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_F11)){ //Show save slots
						now_joy1_mode++;
						if(now_joy1_mode>=MAX_JOY_MODE) now_joy1_mode=5;
						if(now_joy1_mode<5) now_joy1_mode=5;
						clear_input();
					}
					/*switch input mode*/
					/*switch HUD*/
					if (KBD_F11){ //Show HUD
						uint8_t chm = current_hud_mode&0x03;
						chm++;
						//printf("CHM>%d\n",chm);
						if(chm>HM_TIME)chm=HM_OFF;
						if(chm==HM_OFF){
							MessageBox("HUD is ON","\0",CL_GREEN,CL_WHITE,2);
							hud_timer=0;
							current_hud_mode=HM_ON;
							current_hud_mode|=HM_MAIN_HUD;
							continue;
						} 
						if(chm==HM_ON){
							hud_timer=my_millis();
							current_hud_mode=HM_TIME;
							current_hud_mode|=HM_MAIN_HUD;
							MessageBox("HUD is OFF by TIME","\0",CL_BLUE,CL_WHITE,2);
							continue;
						}
						if(chm==HM_TIME){
							hud_timer=0;
							current_hud_mode=HM_OFF;
							MessageBox("HUD is OFF","\0",CL_BLUE,CL_WHITE,2);
							continue;
						}
						clear_input();
					}
					/*switch HUD*/
					/*--QUICK FILE OPERATIONS--*/
					if(init_fs){
						//AY_print_state_debug();
						if((KBD_MENU)||(((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_F12))){ //Show save slots
							if(!show_slots){get_saveslots();}
							show_slots^=1;
							clear_input();
						}
						/*--Fast Load/Save--*/
						#ifndef DEBUG_DISABLE_LOADERS
						if ((KBD_L_SHIFT || KBD_R_SHIFT)||(KBD_L_CTRL || KBD_R_CTRL)){
							//if(KBD_PRESS)show_slots=true;
							uint inx_f1=0;
							//char file_list;
							if(KBD_F1){inx_f1=1;};
							if(KBD_F2){inx_f1=2;};
							if(KBD_F3){inx_f1=3;};
							if(KBD_F4){inx_f1=4;};
							if(KBD_F5){inx_f1=5;};
							if(KBD_F6){inx_f1=6;};
							if(KBD_F7){inx_f1=7;};
							if(KBD_F8){inx_f1=8;};
							if(KBD_F9){inx_f1=9;};
							if(KBD_F10){inx_f1=10;};
							//printf("save/load cleear\n");
							if (inx_f1>0&&inx_f1<11){
								sprintf(save_file_name_image,"0:/save/__F%u.Z80 ",inx_f1);
								//save_file_name_image[11]=48+inx_f1;
								//G_PRINTF("save filename = %s \n",save_file_name_image);
								show_slots=false;
								if (KBD_L_SHIFT || KBD_R_SHIFT){
									im_z80_stop = true;
									while (im_z80_stop){
										busy_wait_ms(10);
										if (im_ready_loading){
											//busy_wait_ms(10);
											//char slot_name[20];
											//draw_text(100,100,slot_name,0xf,0x1);
											zx_machine_reset(false);
											AY_reset(cfg_sound_mode);// сбросить AY
											memset(temp_msg,0,sizeof(temp_msg));
											sprintf(temp_msg," Loading slot#%u ",inx_f1);
											if (load_image_z80(save_file_name_image)){
												MessageBox("QUICKLOAD",temp_msg,CL_WHITE,CL_BLUE,2);
											} else {
												MessageBox("Error QUICKLOAD!!!",temp_msg,CL_YELLOW,CL_LT_RED,1);
											}
											//AY_reset();// сбросить AY
											clear_input();
											im_z80_stop = false;					   
											im_ready_loading = false;
											break;
										}
									}
									continue;
								}
								if (KBD_L_CTRL || KBD_R_CTRL){
									im_ready_loading = false;
									im_z80_stop = true;
									while (im_z80_stop){
										busy_wait_ms(10);
										if (im_ready_loading){
											busy_wait_ms(10);
											short int file_descr = sd_mkdir("0:/save");
											if ((file_descr!=FR_OK)&&(file_descr!=FR_EXIST) ){
												memset(temp_msg,0,sizeof(temp_msg));
												sprintf(temp_msg," Error saving slot#%u ",inx_f1);
												MessageBox("QUICKSAVE",temp_msg,CL_LT_YELLOW,CL_RED,1);
												//draw_text(90,100,slot_name,0x0E,0x02);
												////printf("Error - mkdir %d\n",file_descr);
												break;	
											}
											//char slot_name[20];
											memset(temp_msg,0,sizeof(temp_msg));
											sprintf(temp_msg," Saving slot#%u ",inx_f1);
											//draw_text(100,100,slot_name,0xf,0x1);
											MessageBox("QUICKSAVE",temp_msg,CL_WHITE,CL_BLUE,2);
											save_image_z80(save_file_name_image);
											busy_wait_ms(100);
											clear_input();
											im_z80_stop = false;					   
											im_ready_loading = false;
											break;
										}
									}			   
									continue;
								}
							}
						}
						#endif
						//if(KBD_RELEASE) show_slots=false;
						/*--Fast Load/Save--*/
						/*--Tape load controls--*/
						#ifndef DEBUG_DISABLE_LOADERS
						if (KBD_F5){
							// Start .tap reproduction
							tape_cmd = TAPE_STATE_START;
							continue;
						}
						if ((KBD_F6)&&(TapeStatus==TAPE_LOADING)) {
							// Stop .tap reproduction
							tape_cmd = TAPE_STATE_STOP;
							continue;
						}
						if ((KBD_F6)&&((tapeCurrentBlock>0)&&(TapeStatus!=TAPE_LOADING))) {
							tape_cmd = TAPE_STATE_REWIND;
							continue;
						}
						if (KBD_F7) {
							tape_cmd = TAPE_STATE_PREV_BLOCK;
							continue;
						}
						if (KBD_F8) {
							tape_cmd = TAPE_STATE_NEXT_BLOCK;
							continue;
						}
						if (KBD_F9) {
							tape_cmd = TAPE_STATE_EJECT;
							continue;
						}						
						#endif
						/*--Tape load controls--*/
					}
					/*--QUICK FILE OPERATIONS--*/
					/*--Joy Menu--*/
					if(((KBD_F12))||(KBD_PRT_SCR)){
						menu_ptr++;
						menu_mode[menu_ptr]=MENU_JOY_MAIN;
						break;
					}
					/*--Joy Menu--*/
					/*--KBD Joy Switch--*/
					if (KBD_SCROLL_LOCK){
						if(now_kbd_mode>1){now_kbd_mode=0;}
						if(now_kbd_mode==0){
							now_kbd_mode=1;
							current_hud_mode|=HM_MAIN_HUD;
							while(KBD_PRESS){
								process_input();
								busy_wait_ms(10);
							};
							hud_timer = my_millis();
							continue;
						}
						if(now_kbd_mode==1){
							now_kbd_mode=0;
							current_hud_mode|=HM_MAIN_HUD;
							while(KBD_PRESS){
								process_input();
								busy_wait_ms(10);
							};
							hud_timer = my_millis();
							continue;
						}
					}
					/*--KBD Joy Switch--*/
					//for(uint8_t j=0;j<8;j++){//printf("\t%02X",zx_write_buffer->kb_data[j]);};//printf("\n");//DEBUG
				}
				if(cfg_tap_load_mode<2){
					if(tape_cmd>TAPE_STATE_NULL){
						printf("begin tapestate:%d\n",tape_cmd);
						switch (tape_cmd){
						case TAPE_STATE_START:
							printf("TRY TAP Start\n");
							if(tape_start()){
								current_hud_mode|=HM_TAPE_HUD;
								tape_cmd = TAPE_STATE_NULL;
								continue;
							} else {
								current_hud_mode&=~HM_TAPE_HUD;
								tape_cmd = TAPE_STATE_NULL;
								continue;
							}
							busy_wait_ms(10);
							break;
						case TAPE_STATE_STOP:
							if(tape_stop()){current_hud_mode&=~HM_TAPE_HUD;}
							tape_cmd = TAPE_STATE_NULL;
							continue;
							break;
						case TAPE_STATE_REWIND:
							if(tape_rewind()){current_hud_mode&=~HM_TAPE_HUD;}
							tape_cmd = TAPE_STATE_NULL;
							continue;
							break;
						case TAPE_STATE_EJECT:
							if(tape_eject()){
								memset(afilename,0,FILE_NAME_LEN);
								memset(activefilename,0,400);
								current_hud_mode&=~HM_TAPE_HUD;
							}
							tape_cmd = TAPE_STATE_NULL;
							continue;
							break;
						case TAPE_STATE_PREV_BLOCK:
							if(tape_prev_block()){
								current_hud_mode&=~HM_TAPE_HUD;
							}
							tape_cmd = TAPE_STATE_NULL;
							continue;
							break;
						case TAPE_STATE_NEXT_BLOCK:
							if(tape_next_block()){
								current_hud_mode&=~HM_TAPE_HUD;
							}
							tape_cmd = TAPE_STATE_NULL;
							continue;
							break;
						default:
							break;
						}

					}	
				}


				//if((show_hud)&&(!zx_screen_refresh)&&(!allow_repaint)){
				////printf("ml>%ld\n",time_us_32()-main_loop);
				//main_loop = time_us_32();
				/*if ((ticker%256)==0){
					Wii_debug(&Wii_joy);
					printf("kempston_mouse X:%d    Y:%d   B:%02X\n",zx_write_buffer->kempston_mouse_x,zx_write_buffer->kempston_mouse_y,zx_write_buffer->kempston_mouse_btn);
				}*/

				//printf("data_joy:%d     kempston:%d\n",data_joy,zx_write_buffer->kempston);
				if(ticker>1048576){ticker=0;}
			}while(menu_mode[menu_ptr]==EMULATION); //while(1) emulation loop
			//END EMULATION LOOP
			//clear_input();
			
			disk_action=0;
			tape_action=0;
			zx_machine_enable_vbuf(false);
			if(menu_mode[menu_ptr]!=MENU_HELP){
				enter_pause();
			}
			//TRDOS_disabled = true;
		}
		//printf("3 menu_ptr:%d menu_mode:%d\n",menu_ptr,menu_mode[menu_ptr]);
	//END GLOBAL LOOP
	} while(true);//while(1) main loop
	software_reset();
}
