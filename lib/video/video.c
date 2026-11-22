#include <stdio.h>
#include "video.h"
#include "hardware/clocks.h"
#include <stdalign.h>

#include "hardware/structs/pll.h"
#include "hardware/structs/systick.h"

#include "hardware/dma.h"
#include "hardware/irq.h"
#include <string.h>
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "stdlib.h"

#include "../../src/globals.h"

#include "../../src/hud_func.h"

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 320
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 240
#endif

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

//bool graphics_begin_screen = false;

//программы PIO
//программа конвертации адреса для HDMI

uint16_t pio_program_instructions_conv_HDMI[] = {
	//		 //	 .wrap_target
	0x80a0, //  0: pull   block
	0x40e8, //  1: in	 osr, 8
	0x4034, //  2: in	 x, 20
	0x8020, //  3: push   block
			//	 .wrap
};


const struct pio_program pio_program_conv_addr_HDMI = {
	.instructions = pio_program_instructions_conv_HDMI,
	.length = 4,
	.origin = -1,
};

//программа конвертации адреса для VGA

uint16_t pio_program_instructions_conv_VGA[] = {
	//		 //	 .wrap_target
	0x80a0, //  0: pull   block
	0x40e8, //  1: in	 osr, 8
	0x4037, //  2: in	 x, 23
	0x8020, //  3: push   block
			//	 .wrap
};


const struct pio_program pio_program_conv_addr_VGA= {
	.instructions = pio_program_instructions_conv_VGA,
	.length = 4,
	.origin = -1,
};

//программа видеовывода HDMI
static const uint16_t instructions_PIO_HDMI[] = {
	0x7006, //  0: out	pins, 6		 side 2
	0x7006, //  1: out	pins, 6		 side 2
	0x7006, //  2: out	pins, 6		 side 2
	0x7006, //  3: out	pins, 6		 side 2
	0x7006, //  4: out	pins, 6		 side 2
	0x6806, //  5: out	pins, 6		 side 1
	0x6806, //  6: out	pins, 6		 side 1
	0x6806, //  7: out	pins, 6		 side 1
	0x6806, //  8: out	pins, 6		 side 1
	0x6806, //  9: out	pins, 6		 side 1
};

static const struct pio_program program_PIO_HDMI = {
	.instructions = instructions_PIO_HDMI,
	.length = 10,
	.origin = -1,
};


//программа видеовывода VGA
static uint16_t pio_program_VGA_instructions[] = {
	//	 .wrap_target

	//	 .wrap_target
	0x6008, //  0: out	pins, 8
	//	 .wrap
	//	 .wrap
};

static const struct pio_program program_pio_VGA = {
	.instructions = pio_program_VGA_instructions,
	.length = 1,
	.origin = -1,
};

/******TFT******/

//#define TFT_CLK_DIV 3.3f //64.0
//#define TFT_CLK_DIV 3.15f //100 FPS
//#define TFT_CLK_DIV 2.1f //150 FPS
#define TFT_CLK_DIV 2.625f //120 FPS
//#define TFT_CLK_DIV 6.3f //50 FPS
#define HEADER_32Bit_ALIGN (16)
#define HEADER_32Bit_PACKETS (11)

static const uint16_t pio_program_TFT_instructions[] = {
    0xe004, //  0: set    pins, 4         side 0     
            //     .wrap_target
    0x80a0, //  1: pull   block           side 0     
    0x6021, //  2: out    x, 1            side 0     
    0x0026, //  3: jmp    !x, 6           side 0     
    0xe404, //  4: set    pins, 4         side 0 [4] 
    0x0007, //  5: jmp    7               side 0     
    0xe414, //  6: set    pins, 20        side 0 [4] 
    0x602f, //  7: out    x, 15           side 0     
    0x0020, //  8: jmp    !x, 0           side 0     
    0x6050, //  9: out    y, 16           side 0     
    0xa0c1, // 10: mov    isr, x          side 0     
    0x80a0, // 11: pull   block           side 0     
    0x6001, // 12: out    pins, 1         side 0     
    0x104c, // 13: jmp    x--, 12         side 1     
    0xa026, // 14: mov    x, isr          side 0     
    0x048b, // 15: jmp    y--, 11         side 0 [4] 
    0xe015, // 16: set    pins, 21        side 0     
            //     .wrap
};

static const struct pio_program program_pio_TFT = {
    .instructions = pio_program_TFT_instructions,
    .length = 17,
    .origin = -1,
};

/******TFT******/

typedef struct V_MODE{
	int VS_begin;
	int VS_end;
	int V_visible_lines;
	int V_total_lines;

	int H_visible_len;
	int H_visible_begin;
	int HS_len;
	int H_len;

	uint8_t VS_TMPL;
	uint8_t VHS_TMPL;
	uint8_t HS_TMPL;
	uint8_t NO_SYNC_TMPL;

	fr_rate FR_RATE;
	double CLK_SPD;

} V_MODE;

typedef struct G_BUFFER{
	uint width;
	uint height;
	int shift_x;
	int shift_y;
	uint8_t* data;
	uint8_t* overlay;
	bool (*handler)(short line);
	uint8_t rotate;
	bool inversion;
	uint8_t pix_format;
}G_BUFFER;
//данные модуля

typedef struct ALLOWED_MODE{
	g_out video_out;
	const V_MODE* video_mode;
}ALLOWED_MODE;

//640x480x60 - vga
static const V_MODE v_mode_640x480x60v	={
	.VS_begin			=	491,
	.VS_end				=	492,
	.V_total_lines		=	525,
	.V_visible_lines	=	480,

	.HS_len				=	96/2,
	.H_len				=	800/2,
	.H_visible_begin	=	144/2,
	.H_visible_len		=	640/2,

	.VS_TMPL			=	242,   //шаблоны синхронизации заложены в таблицу палитры после 240 индекса
	.VHS_TMPL			=	243,
	.HS_TMPL			=	241,
	.NO_SYNC_TMPL		=	240,
	.FR_RATE 			=	rate_60Hz,
	.CLK_SPD			=	25175000.0
};

//640x480x72 - vga
static const V_MODE v_mode_640x480x72v 	={
    .VS_begin			=	490,
    .VS_end				=	491,
    .V_total_lines		=	520,
    .V_visible_lines	=	480,
    
    .HS_len				=	40/2,
    .H_len				=	832/2,
    .H_visible_begin	=	168/2,
    .H_visible_len		=	640/2,

    .VS_TMPL			=	242,   //шаблоны синхронизации заложены в таблицу палитры после 240 индекса
    .VHS_TMPL			=	243,
    .HS_TMPL			=	241,
    .NO_SYNC_TMPL		=	240,
	.FR_RATE 			=	rate_72Hz,
	.CLK_SPD			=	31500000.0
};

//640x480x75 - vga
static const V_MODE v_mode_640x480x75v 	={
	.VS_begin			=	482,
	.VS_end				=	484,
	.V_total_lines		=	500,
	.V_visible_lines	=	480,

	.HS_len				=	64/2,
	.H_len				=	840/2,
	.H_visible_begin	=	184/2,
	.H_visible_len		=	640/2,

	.VS_TMPL			=	242,   //шаблоны синхронизации заложены в таблицу палитры после 240 индекса
	.VHS_TMPL			=	243,
	.HS_TMPL			=	241,
	.NO_SYNC_TMPL		=	240,
	.FR_RATE 			=	rate_75Hz,
	.CLK_SPD			=	31500000.0
};

//640x480x85 - vga
static const V_MODE v_mode_640x480x85v	={
	.VS_begin			=	482,
	.VS_end				=	484,
	.V_total_lines		=	509,
	.V_visible_lines	=	480,

	.HS_len				=	56/2,
	.H_len				=	832/2,
	.H_visible_begin	=	136/2,
	.H_visible_len		=	640/2,


	.VS_TMPL			=	242,   //шаблоны синхронизации заложены в таблицу палитры после 240 индекса
	.VHS_TMPL			=	243,
	.HS_TMPL			=	241,
	.NO_SYNC_TMPL		=	240,
	.FR_RATE 			=	rate_85Hz,
	.CLK_SPD			=	36000000.0
};

//640x480x72 - hdmi
static const V_MODE v_mode_640x480x72h 	={
    .VS_begin			=	490,
    .VS_end				=	491,
    .V_total_lines		=	520,
    .V_visible_lines	=	480,
    
    .HS_len				=	40/2,
    .H_len				=	832/2,
    .H_visible_begin	=	168/2,
    .H_visible_len		=	640/2,

    .VS_TMPL			=	242,   //шаблоны синхронизации заложены в таблицу палитры после 240 индекса
    .VHS_TMPL			=	243,
    .HS_TMPL			=	241,
    .NO_SYNC_TMPL		=	240,
	.FR_RATE 			=	rate_72Hz,
	.CLK_SPD			=	252000000.0
};

//640x480x75 - hdmi
static const V_MODE v_mode_640x480x75h 	={
	.VS_begin			=	482,
	.VS_end				=	484,
	.V_total_lines		=	500,
	.V_visible_lines	=	480,

	.HS_len				=	64/2,
	.H_len				=	840/2,
	.H_visible_begin	=	184/2,
	.H_visible_len		=	640/2,

	.VS_TMPL			=	242,   //шаблоны синхронизации заложены в таблицу палитры после 240 индекса
	.VHS_TMPL			=	243,
	.HS_TMPL			=	241,
	.NO_SYNC_TMPL		=	240,
	.FR_RATE 			=	rate_75Hz,
	.CLK_SPD			=	252000000.0
};

/******TFT******/
//320x240x60 - tft
static const V_MODE v_mode_320x240x60t 	={
	.VS_begin			=	0,
	.VS_end				=	320,
	.V_total_lines		=	240,
	.V_visible_lines	=	240,

	.HS_len				=	0,
	.H_len				=	(SCREEN_WIDTH/2)+HEADER_32Bit_PACKETS,
	.H_visible_begin	=	0,
	.H_visible_len		=	320,

	.VS_TMPL			=	0,
	.VHS_TMPL			=	0,
	.HS_TMPL			=	0,
	.NO_SYNC_TMPL		=	0,
	.FR_RATE 			=	rate_60Hz,
	.CLK_SPD			=	31500000.0
};

//320x240x72 - tft
static const V_MODE v_mode_320x240x72t 	={
	.VS_begin			=	0,
	.VS_end				=	320,
	.V_total_lines		=	240,
	.V_visible_lines	=	240,

	.HS_len				=	0,
	.H_len				=	(SCREEN_WIDTH/2)+HEADER_32Bit_PACKETS,
	.H_visible_begin	=	0,
	.H_visible_len		=	320,

	.VS_TMPL			=	0,
	.VHS_TMPL			=	0,
	.HS_TMPL			=	0,
	.NO_SYNC_TMPL		=	0,
	.FR_RATE 			=	rate_72Hz,
	.CLK_SPD			=	31500000.0
};

//320x240x75 - tft
static const V_MODE v_mode_320x240x75t 	={
	.VS_begin			=	0,
	.VS_end				=	320,
	.V_total_lines		=	240,
	.V_visible_lines	=	240,

	.HS_len				=	0,
	.H_len				=	(SCREEN_WIDTH/2)+HEADER_32Bit_PACKETS,
	.H_visible_begin	=	0,
	.H_visible_len		=	320,

	.VS_TMPL			=	0,
	.VHS_TMPL			=	0,
	.HS_TMPL			=	0,
	.NO_SYNC_TMPL		=	0,
	.FR_RATE 			=	rate_75Hz,
	.CLK_SPD			=	31500000.0
};

//320x240x85 - tft
static const V_MODE v_mode_320x240x85t 	={
	.VS_begin			=	0,
	.VS_end				=	320,
	.V_total_lines		=	240,
	.V_visible_lines	=	240,

	.HS_len				=	0,
	.H_len				=	(SCREEN_WIDTH/2)+HEADER_32Bit_PACKETS,
	.H_visible_begin	=	0,
	.H_visible_len		=	320,

	.VS_TMPL			=	0,
	.VHS_TMPL			=	0,
	.HS_TMPL			=	0,
	.NO_SYNC_TMPL		=	0,
	.FR_RATE 			=	rate_85Hz,
	.CLK_SPD			=	31500000.0
};
/******TFT******/

#define MAX_ALLOWED_MODE (26)

static const ALLOWED_MODE mode_table[MAX_ALLOWED_MODE] = {
	{g_out_VGA			,&v_mode_640x480x60v},
	{g_out_VGA			,&v_mode_640x480x72v},
	{g_out_VGA			,&v_mode_640x480x75v},
	{g_out_VGA			,&v_mode_640x480x85v},
	{g_out_HDMI			,&v_mode_640x480x72h},
	{g_out_HDMI			,&v_mode_640x480x75h},
	//tft section
	{g_out_TFT_ST7789	,&v_mode_320x240x60t},
	{g_out_TFT_ST7789	,&v_mode_320x240x72t},
	{g_out_TFT_ST7789	,&v_mode_320x240x75t},
	{g_out_TFT_ST7789	,&v_mode_320x240x85t},

	{g_out_TFT_ST7789V	,&v_mode_320x240x60t},
	{g_out_TFT_ST7789V	,&v_mode_320x240x72t},
	{g_out_TFT_ST7789V	,&v_mode_320x240x75t},
	{g_out_TFT_ST7789V	,&v_mode_320x240x85t},

	{g_out_TFT_ILI9341	,&v_mode_320x240x60t},
	{g_out_TFT_ILI9341	,&v_mode_320x240x72t},
	{g_out_TFT_ILI9341	,&v_mode_320x240x75t},
	{g_out_TFT_ILI9341	,&v_mode_320x240x85t},

	{g_out_TFT_ILI9341V	,&v_mode_320x240x60t},
	{g_out_TFT_ILI9341V	,&v_mode_320x240x72t},
	{g_out_TFT_ILI9341V	,&v_mode_320x240x75t},
	{g_out_TFT_ILI9341V	,&v_mode_320x240x85t},

	{g_out_TFT_GC9A01	,&v_mode_320x240x60t},
	{g_out_TFT_GC9A01	,&v_mode_320x240x72t},
	{g_out_TFT_GC9A01	,&v_mode_320x240x75t},
	{g_out_TFT_GC9A01	,&v_mode_320x240x85t},
 
};

static G_BUFFER g_buf={
	.data=NULL,
	.overlay=NULL,
	.handler=NULL,
	.shift_x=0,
	.shift_y=0,
	.height=240,
	.width=320,
	.rotate=0,
	.inversion=false,
	.pix_format=0
};


/******TFT******/

uint8_t tempcmd[7];

// 126MHz SPIs @ 60 kHz
//#define MADCTL_BGR_PIXEL_ORDER (1<<3)
//#define MADCTL_ROW_COLUMN_EXCHANGE (1<<5)
//#define MADCTL_COLUMN_ADDRESS_ORDER_SWAP (1<<6)

#define MADCTL_MY 0x80  ///< Bottom to top
#define MADCTL_MX 0x40  ///< Right to left
#define MADCTL_MV 0x20  ///< Reverse Mode
#define MADCTL_ML 0x10  ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order
#define MADCTL_MH 0x04  ///< LCD refresh right to left

// Format: cmd length (including cmd byte), post delay in units of 5 ms, then cmd payload
// Note the delays have been shortened a little
static const uint8_t init_seq_ST7789[] = {
    1, 20,  0x01, // Software reset
    1, 10,  0x11, // Exit sleep mode
    2, 2,   0x3A, 0x55, // Set colour mode to 16 bit
	2, 0,   0xC6, 0x00,
    2, 0,   0x36, MADCTL_MV | MADCTL_ML, // Set MADCTL //MADCTL_COLUMN_ADDRESS_ORDER_SWAP | MADCTL_ROW_COLUMN_EXCHANGE,
    5, 0,   0x2A, 0x00, 0x00, SCREEN_WIDTH >> 8, SCREEN_WIDTH & 0xff, // CASET: column addresses
    5, 0,   0x2B, 0x00, 0x00, SCREEN_HEIGHT >> 8, SCREEN_HEIGHT & 0xff, // RASET: row addresses
    1, 2,   0x20, // Inversion OFF
    1, 2,   0x13, // Normal display on, then 10 ms delay
    1, 2,   0x29, // Main screen turn on, then wait 500 ms
    0 // Terminate list
};

static const uint8_t init_seq_ST7789V[] = {
    1, 20,  0x01, // Software reset
    1, 10,  0x11, // Exit sleep mode
    2, 2,   0x3a, 0x55, // Set colour mode to 16 bit
	2, 0,   0xC6, 0x00,
    2, 0,   0x36, MADCTL_MV | MADCTL_ML, // Set MADCTL //MADCTL_COLUMN_ADDRESS_ORDER_SWAP | MADCTL_ROW_COLUMN_EXCHANGE,
    5, 0,   0x2a, 0x00, 0x00, SCREEN_WIDTH >> 8, SCREEN_WIDTH & 0xff, // CASET: column addresses
    5, 0,   0x2b, 0x00, 0x00, SCREEN_HEIGHT >> 8, SCREEN_HEIGHT & 0xff, // RASET: row addresses
    1, 2,   0x21, // Inversion ON
    1, 2,   0x13, // Normal display on, then 10 ms delay
    1, 2,   0x29, // Main screen turn on, then wait 500 ms
    0 // Terminate list
};

static const uint8_t init_seq_ILI9341[] = {
    1, 20,  0x01, // Software reset //3
    1, 10,  0x11, // Exit sleep mode //6
    2, 2,   0x3a, 0x55, // Set colour mode to 16 bit //10
    2, 0,   0x36, MADCTL_ML | MADCTL_BGR, //14
    5, 0,   0x2a, 0x00, 0x00, SCREEN_WIDTH >> 8, SCREEN_WIDTH & 0xff, // CASET: column addresses //21
    5, 0,   0x2b, 0x00, 0x00, SCREEN_HEIGHT >> 8, SCREEN_HEIGHT & 0xff, // RASET: row addresses //28
    1, 2,   0x20, // Inversion OFF //31
    1, 2,   0x13, // Normal display on, then 10 ms delay //34
    1, 2,   0x29, // Main screen turn on, then wait 500 ms // 37
    0 // Terminate list
};

static const uint8_t init_seq_ILI9341V[] = {
    1,	20,	0x01, // Software reset //3
    1,	10,	0x11, // Exit sleep mode //6
  	//2,	0,	0xC0, 0x21,             	// Power control VRH[5:0] 
  	//2,	0,	0xC1, 0x10,             	// Power control SAP[2:0];BT[3:0]
	//3,	0,	0xC5, 0x31, 0x3C,       	// VCM control  
  	//2,	0,	0xC7, 0xC0,             	// VCM control2
	//3,	0,	0xB1, 0x00, 0x1F,			//Frame Rate Control (In Normal Mode/Full Colors) //61Hz
	//3,	0,	0xB2, 0x00, 0x1F,			//Frame Rate Control (In Idle Mode/8 colors) //61Hz
	//3,	0,	0xB3, 0x00, 0x1F,			//Frame Rate Control (n Partial Mode/Full Colors) //61Hz
  	//16,	0,	0xE0, 0x0F, 0x16, 0x14, 0x0A, 0x0D, 0x06, 0x43, 0x75, 0x33, 0x06, 0x0E, 0x00, 0x0C, 0x09, 0x08, //Positive Gamma Correction default
  	//16,	0,	0xE1, 0x08, 0x2B, 0x2D, 0x04, 0x10, 0x04, 0x3E, 0x24, 0x4E, 0x04, 0x0F, 0x0E, 0x35, 0x38, 0x0F, //Negative Gamma Correction default
    2,	2,	0x3A, 0x55, // Set colour mode to 16 bit //10
    2,	0,	0x36, MADCTL_ML | MADCTL_BGR, //14 // Memory Access Control
    5,	0,	0x2A, 0x00, 0x00, SCREEN_WIDTH >> 8, SCREEN_WIDTH & 0xff, // CASET: column addresses //21
    5,	0,	0x2B, 0x00, 0x00, SCREEN_HEIGHT >> 8, SCREEN_HEIGHT & 0xff, // RASET: row addresses //28
    1,	2,	0x21, // Inversion ON //31
    1,	2,	0x13, // Normal display on, then 10 ms delay //34
    1,	2,	0x29, // Main screen turn on, then wait 500 ms // 37
    0 // Terminate list
};

static const uint8_t init_seq_GC9A01[] = {
    1, 1, 0xEF,
    2, 1, 0xEB, 0x14,
    1, 1, 0xFE,
    1, 1, 0xEF,
    2, 0, 0x36, MADCTL_BGR, // Set MADCTL
	2, 2, 0x3a, 0x55, // Set colour mode to 16 bit
    2, 1, 0xEB, 0x14,
    2, 1, 0x84, 0x40, // Rotation and mirroring
	3, 1, 0xB6, 0x00, 0x20,
    13, 3, 0x62, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70,
    13, 3, 0x63, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70,
    8, 3, 0x64, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07,
    11, 3, 0x66, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00,
    11, 3, 0x67, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98,
    8, 2, 0x74, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00,
    3, 1, 0x98, 0x3e, 0x07,
    1, 1, 0x35,
    1, 1, 0x21,
    1, 20, 0x11,
    1, 5, 0x29,
    0 // Terminate list
};
/******TFT******/

//буферы строк
//количество буферов задавать кратно степени двойки
//
#define N_LINE_BUF_log2 (3)

#define N_LINE_BUF_DMA (1<<N_LINE_BUF_log2)
#define N_LINE_BUF (N_LINE_BUF_DMA/2)

//максимальный размер строки
//#define LINE_SIZE_MAX (420)

#define LINE_SIZE_MAX ((((SCREEN_WIDTH*16)/32)+HEADER_32Bit_ALIGN)*4)

//указатели на буферы строк
//выравнивание нужно для кольцевого буфера
static uint32_t rd_addr_DMA_CTRL[N_LINE_BUF*2]__attribute__ ((aligned (4*N_LINE_BUF_DMA))) ;
//непосредственно буферы строк

static alignas(32) uint32_t lines_buf[N_LINE_BUF][LINE_SIZE_MAX/4];

static int SM_video=-1;
static int SM_conv=-1;

//DMA каналы
//каналы работы с первичным графическим буфером
static int dma_chan_ctrl=-1;
static int dma_chan=-1;
//каналы работы с конвертацией палитры
static int dma_chan_pal_conv_ctrl=-1;
static int dma_chan_pal_conv=-1;

//ДМА палитра для конвертации
static alignas(4096)uint32_t conv_color[1024];

static V_MODE hw_mode;
static g_mode active_mode;
static g_out active_out;
repeating_timer_t video_timer;



//программа установки начального адреса массива-конвертора
static void pio_set_x(PIO pio, int sm, uint32_t v){
	uint instr_shift = pio_encode_in(pio_x, 4);
	uint instr_mov = pio_encode_mov(pio_x, pio_isr);
	for (int i=0;i<8;i++){
		const uint32_t nibble = (v >> (i * 4)) & 0xf;
		pio_sm_exec(pio, sm, pio_encode_set(pio_x, nibble));
		pio_sm_exec(pio, sm, instr_shift);
	}
	pio_sm_exec(pio, sm, instr_mov);
}

//подготовка данных HDMI для вывода на пины
static uint64_t get_ser_diff_data(uint16_t dataR,uint16_t dataG,uint16_t dataB){
	uint64_t out64=0;
	for(int i=0;i<10;i++){
		out64<<=6;
		if(i==5) out64<<=2;
		uint8_t bR=(dataR>>(9-i))&1;
		uint8_t bG=(dataG>>(9-i))&1;
		uint8_t bB=(dataB>>(9-i))&1;

		bR|=(bR^1)<<1;
		bG|=(bG^1)<<1;
		bB|=(bB^1)<<1;

		if (HDMI_PIN_invert_diffpairs){
			bR^=0b11;
			bG^=0b11;
			bB^=0b11;
		};
		uint8_t d6;
		if (HDMI_PIN_RGB_notBGR){
			d6=(bR<<4)|(bG<<2)|(bB<<0);
		} else {
			d6=(bB<<4)|(bG<<2)|(bR<<0);
		};
		out64|=d6;
	}
	return out64;
};

//конвертор TMDS
static uint tmds_encoder(uint8_t d8){
	int s1=0;
	for(int i=0;i<8;i++){ s1+=(d8&(1<<i))?1:0;};
	int is_xnor=0;
	if ((s1>4)||((s1==4)&&((d8&1)==0))) is_xnor=1;
	uint16_t d_out=d8&1;
	uint16_t qi=d_out;
	for(int i=1;i<8;i++){
		d_out|=((qi<<1)^(d8&(1<<i)))^(is_xnor<<i);
		qi=d_out&(1<<i);
	}

	if(is_xnor){
		d_out|=1<<9;
	} else {
		d_out|=1<<8;
	}

	return d_out;

}

/******TFT******/

void *memset32(void *m, uint32_t val, size_t count){
    uint32_t *buf = m;
    while(count--) *buf++ = val;
    return m;
}

static inline void TFT_wait_idle(PIO pio, uint sm) {
    uint32_t sm_stall_mask = 1u << (sm + PIO_FDEBUG_TXSTALL_LSB);
    pio->fdebug = sm_stall_mask;
    while (!(pio->fdebug & sm_stall_mask))
        ;
}

static inline void TFT_put(PIO pio, uint sm, uint32_t x) {
    while (pio_sm_is_tx_fifo_full(pio, sm))
        ;
    *(volatile uint32_t*)&pio->txf[sm] = x;
}

static inline void __not_in_flash_func(TFT_write_cmd)(const uint8_t* cmd, size_t count) {
    TFT_wait_idle(PIO_VIDEO, SM_video);
	TFT_put(PIO_VIDEO, SM_video, 0x80070000); // Send 8 bit length 
	TFT_put(PIO_VIDEO, SM_video, (*cmd++<<24));
    if (count >= 2) {
        for (size_t i = 0; i < count - 1; ++i){
		    TFT_wait_idle(PIO_VIDEO, SM_video);
			TFT_put(PIO_VIDEO, SM_video, 0x00070000); // Send count of 8 bit length 
            TFT_put(PIO_VIDEO, SM_video, (*cmd++<<24));
		}
    }
    TFT_wait_idle(PIO_VIDEO, SM_video);    
}

static inline void TFT_send_init(const uint8_t* init_seq) {
    const uint8_t* cmd = init_seq;
    while (*cmd) {
        TFT_write_cmd(cmd + 2, *cmd);
        busy_wait_ms(*(cmd + 1) * 5);
        cmd += *cmd + 2;
    }
}

static inline void __not_in_flash_func(TFT_set_window)(const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height) {
    uint32_t screen_w_cmd = 0;
    uint32_t screen_h_cmd = 0;

    screen_w_cmd |= (x<<16);
    screen_w_cmd |= x + (width);
    screen_h_cmd |= (y<<16);
    screen_h_cmd |= y + (height);

    //tft_lcd_wait_idle(PIO_VIDEO, SM_video);

	TFT_put(PIO_VIDEO, SM_video, 0x80070000); // Send 8 bit length 
	TFT_put(PIO_VIDEO, SM_video, 0x2A000000);

	TFT_put(PIO_VIDEO, SM_video, 0x001F0000); // Send count of 32 bit length 
    TFT_put(PIO_VIDEO, SM_video, screen_w_cmd); // Send count of 32 bit length 
    
	TFT_put(PIO_VIDEO, SM_video, 0x80070000); // Send 8 bit length 
	TFT_put(PIO_VIDEO, SM_video, 0x2B000000);

	TFT_put(PIO_VIDEO, SM_video, 0x001F0000); // Send count of 32 bit length 
    TFT_put(PIO_VIDEO, SM_video, screen_h_cmd); // Send count of 32 bit length 

}

static inline void __not_in_flash_func(TFT_start_pixels)() {
    //tft_lcd_wait_idle(PIO_VIDEO, SM_video);
	TFT_put(PIO_VIDEO, SM_video, 0x80070000); // Send 8 bit length 
	TFT_put(PIO_VIDEO, SM_video, 0x2C000000);
}

static inline void __not_in_flash_func(TFT_stop_pixels)() {
    //tft_lcd_wait_idle(PIO_VIDEO, SM_video);
	TFT_put(PIO_VIDEO, SM_video, 0x80070000); // Send 8 bit length 
	TFT_put(PIO_VIDEO, SM_video, 0x00000000);
}

void TFT_clr_scr(const uint16_t color) {
    memset(&g_buf.data[0], 0, g_buf.height * (g_buf.width/2));
    TFT_set_window(0, 0,SCREEN_WIDTH,SCREEN_HEIGHT);
    uint32_t i = (SCREEN_WIDTH/2) * SCREEN_HEIGHT;
	TFT_start_pixels();
    while (--i) {
		TFT_put(PIO_VIDEO, SM_video, 0x001F0000); // Send count of 32 bit length 
        TFT_put(PIO_VIDEO, SM_video, (uint32_t)(color<<16)|color); // Send count of 32 bit length         
    }
	//TFT_stop_pixels();
}

void TFT_prepare_buffer(){
	uint lines_buf_inx=0;
	for (uint8_t i=0;i<N_LINE_BUF;i++){
		memset32(&lines_buf[lines_buf_inx],0x00000000,LINE_SIZE_MAX/4);
		uint32_t* out_buf32=(uint32_t*)lines_buf[lines_buf_inx];
		*out_buf32++ = 0x80070000; //80 - command flag , 07 - 8 bit length, 0000 - 1 packet 
		*out_buf32++ = 0x2A000000; //width cmd
		*out_buf32++ = 0x001F0000; //00 - data flag, 1F - 32 bit length, 0000 - 1 packet 
		*out_buf32++ = 0x0000013F; //320 pixels
		*out_buf32++ = 0x80070000; //80 - command flag , 07 - 8 bit length, 0000 - 1 packet 
		*out_buf32++ = 0x2B000000; //height cmd
		*out_buf32++ = 0x001F0000; //00 - data flag, 1F - 32 bit length, 0000 - 1 packet 
		*out_buf32++ = 0x00780078; //one current line
		*out_buf32++ = 0x80070000; //80 - command flag , 07 - 8 bit length, 0000 - 1 packet 
		*out_buf32++ = 0x2C000000; //write ram cmd
		*out_buf32++ = 0x001F009F; //00 - data flag, 1F - 32 bit length, 009F - 160 packet
		lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF;		
	}
	lines_buf_inx=0;
	for (uint8_t i=0;i<N_LINE_BUF_DMA;i++){
		rd_addr_DMA_CTRL[i]=(uint32_t)&lines_buf[lines_buf_inx];
		lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF;
	}
}

/******TFT******/

volatile uint dma_inx_out=0;
volatile uint lines_buf_inx=0;


#pragma GCC push_options
#pragma GCC optimize("-Os") //fast
//основная функция заполнения буферов видеоданных
void __not_in_flash_func(main_video_loop)(){
	
	//static uint dma_inx_out=0;
	//static uint lines_buf_inx=0;

	if (dma_chan_ctrl==-1) return;//не определен дма канал

	//получаем индекс выводимой строки
	uint dma_inx=(N_LINE_BUF_DMA-2+((dma_channel_hw_addr(dma_chan_ctrl)->read_addr-(uint32_t)rd_addr_DMA_CTRL)/4))%(N_LINE_BUF_DMA);

	//uint n_loop=(N_LINE_BUF_DMA+dma_inx-dma_inx_out)%N_LINE_BUF_DMA;

	static uint32_t line_active=0;
	static uint8_t* vbuf=NULL;
	//static uint32_t frame_i=0;
	uint32_t* out_buf32=0;
	int line=line_active;
	//while(n_loop--)
	while(dma_inx_out!=dma_inx){
		//режим VGA
		line=line_active;
		line_active++;

		if(active_out<g_out_TFT_ST7789){
			if (line_active==hw_mode.V_total_lines) {line_active=0;vbuf=g_buf.data;} //frame_i++;
			if (line_active<hw_mode.V_visible_lines){
				//зона изображения
				
				switch (active_mode){
					case g_mode_320x240x4bpp:
					case g_mode_320x240x8bpp:
						//320x240 графика
						if (line_active&1){
							//повтор шаблона строки
						} else {
							//новая строка
							lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF;
							uint8_t* out_buf8=(uint8_t*)lines_buf[lines_buf_inx];
							//зона синхры, затираем все шаблоны
							if(line_active<(N_LINE_BUF_DMA*2)){
								memset(out_buf8,hw_mode.HS_TMPL,hw_mode.HS_len);
								memset(out_buf8+hw_mode.HS_len,hw_mode.NO_SYNC_TMPL,hw_mode.H_visible_begin-hw_mode.HS_len);
								memset(out_buf8+hw_mode.H_visible_begin+hw_mode.H_visible_len,hw_mode.NO_SYNC_TMPL,hw_mode.H_len-(hw_mode.H_visible_begin+hw_mode.H_visible_len));
							}
							//формирование картинки
							int line=line_active/2;
							out_buf8+=hw_mode.H_visible_begin;
							uint8_t* vbuf8;
							uint8_t* ovlbuf8;
							//для 4-битного буфера
							if (active_mode==g_mode_320x240x4bpp){
								if (vbuf!=NULL){
									vbuf8=vbuf+(line)*g_buf.width/2;
									//graphics_begin_screen = true;
									ovlbuf8 = NULL;
									if((*g_buf.handler)!=NULL){
										if((*g_buf.handler)(line)){
											ovlbuf8 = g_buf.overlay;
										}
									}
									for(int i=hw_mode.H_visible_len/2;i--;){
											if(ovlbuf8 != NULL){ //if((i>15)&&(i<(15+128))&&
												*out_buf8++=((*ovlbuf8)<0x10)?(*ovlbuf8&0x0f):(*vbuf8&0x0f);
												*ovlbuf8++;
												*out_buf8++=((*ovlbuf8)<0x10)?(*ovlbuf8&0x0f):(*vbuf8>>4);
												*ovlbuf8++;
												*vbuf8++;
											} else {
												*out_buf8++=*vbuf8&0x0f;
												*out_buf8++=*vbuf8++>>4;
											}
									}
								}
							} else {
							//для 8-битного буфера
								if(vbuf!=NULL){
									vbuf8=vbuf+(line)*g_buf.width;
									for(int i=hw_mode.H_visible_len/16;i--;){
										// *out_buf8++=100;
										//	*out_buf8++=(i<240)?i:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
										*out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;
									}
								}
							}
						}
						break;
					default:
						break;
				}
			} else {
				if((line_active>=hw_mode.VS_begin)&&(line_active<=hw_mode.VS_end)){//кадровый синхроимпульс
					if (line_active==hw_mode.VS_begin){
						//новый шаблон
						lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF;
						uint8_t* out_buf8=(uint8_t*)lines_buf[lines_buf_inx];
						memset(out_buf8,hw_mode.VHS_TMPL,hw_mode.HS_len);
						memset(out_buf8+hw_mode.HS_len,hw_mode.VS_TMPL,hw_mode.H_len-hw_mode.HS_len);
					} else {
						//повтор шаблона
					}
				} else {
					//строчный синхроимпульс
					if((line_active==(hw_mode.VS_end+1))||(line_active==(hw_mode.V_visible_lines))){
						//новый шаблон
						lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF;
						uint8_t* out_buf8=(uint8_t*)lines_buf[lines_buf_inx];
						memset(out_buf8,hw_mode.HS_TMPL,hw_mode.HS_len);
						memset(out_buf8+hw_mode.HS_len,hw_mode.NO_SYNC_TMPL,hw_mode.H_len-hw_mode.HS_len);
					} else {
						//повтор шаблона
					}
				}
			}
		}
		if(active_out>g_out_HDMI){
			if (line_active==hw_mode.V_total_lines) {line_active=0;vbuf=g_buf.data;} //frame_i++;
			if (line_active<hw_mode.V_total_lines){
				switch (active_mode){
					case g_mode_320x240x4bpp:
					case g_mode_320x240x8bpp:
						//320x240 графика
						//передаём команды отрисовки строки
						out_buf32=(uint32_t*)lines_buf[lines_buf_inx];
						*out_buf32++ = 0x80070000;	//80 - command flag , 07 - 8 bit length, 0000 - 1 packet 
						*out_buf32++ = 0x2A000000;	//width cmd
						*out_buf32++ = 0x001F0000;	//00 - data flag, 1F - 32 bit length, 0000 - 1 packet 
						*out_buf32++ = 0x0000013F;	//16 bit range 0-319 - 320 pixels
						*out_buf32++ = 0x80070000;	//80 - command flag , 07 - 8 bit length, 0000 - 1 packet
						*out_buf32++ = 0x2B000000;	//height cmd
						uint32_t screen_h_cmd = 0;
						screen_h_cmd |= (line<<16);
						screen_h_cmd |= (line);
						*out_buf32++ = 0x001F0000;	//00 - data flag, 1F - 32 bit length, 0000 - 1 packet 
						*out_buf32++ = screen_h_cmd;//one current line
						*out_buf32++ = 0x80070000;	//80 - command flag , 07 - 8 bit length, 0000 - 1 packet 
						*out_buf32++ = 0x2C000000; 	//write ram cmd
						*out_buf32++ = 0x001F009F;	//00 - data flag, 1F - 32 bit length, 009F - 160 packet
						uint16_t* out_buf16=(uint16_t*)out_buf32;
						uint8_t* vbuf8;
						uint8_t* ovlbuf8;
						//для 4-битного буфера
						if (active_mode==g_mode_320x240x4bpp){
							if (vbuf!=NULL){
								vbuf8=vbuf+((line)*(g_buf.width/2));
								//graphics_begin_screen = true;
								ovlbuf8 = NULL;
								if((*g_buf.handler)!=NULL){
									if((*g_buf.handler)(line)){
										ovlbuf8 = g_buf.overlay;
									}
								}
								for(int i=hw_mode.H_visible_len/2;i--;){
									if(ovlbuf8 != NULL){ //if((i>15)&&(i<(15+128))&&
										uint16_t pix = ((*ovlbuf8++)<<8)|(*ovlbuf8++);
										*out_buf16++=conv_color[((pix&0xFF)<0x10)?(pix&0x0F):(*vbuf8>>4)];
										*out_buf16++=conv_color[(((pix&0xFF00)>>8)<0x10)?((pix&0x0F00)>>8):(*vbuf8&0x0F)];
										*vbuf8++;
									} else {
										*out_buf16++=conv_color[*vbuf8>>4];
										*out_buf16++=conv_color[*vbuf8&0x0f];
										*vbuf8++;
									}
								}
							}
						} else {
						//для 8-битного буфера
							if(vbuf!=NULL){
								vbuf8=vbuf+(line)*g_buf.width;
								for(int i=hw_mode.H_visible_len/16;i--;){
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
									*out_buf16++=conv_color[(*vbuf8<240)?*vbuf8:0];vbuf8++;
								}
							}
						}
						break;
					default:
						break;
				}
			}
		}
		rd_addr_DMA_CTRL[dma_inx_out]=(uint32_t)&lines_buf[lines_buf_inx];//включаем заполненный буфер в данные для вывода
		dma_inx_out=(dma_inx_out+1)%(N_LINE_BUF_DMA);
		dma_inx=(N_LINE_BUF_DMA-2+((dma_channel_hw_addr(dma_chan_ctrl)->read_addr-(uint32_t)rd_addr_DMA_CTRL)/4))%(N_LINE_BUF_DMA);
		if(active_out>g_out_HDMI) lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF;
	}
}
#pragma GCC pop_options

bool __not_in_flash_func(video_timer_callback(repeating_timer_t *rt)){
	//if((!zx_screen_refresh)){
		main_video_loop();
	//}	
	return true;
}
/*
void __not_in_flash_func(graphics_update_screen)(){
	if(active_out>g_out_HDMI){
		main_video_loop();
	}
}
*/

void graphics_set_mode(g_mode mode){
	active_mode=mode;
};
//определение палитры
void graphics_set_palette(uint8_t i, uint32_t color888){
	if (i>=240) return;

	if (active_out==g_out_HDMI){
		uint64_t* conv_color64=(uint64_t*) conv_color;
		uint8_t R=(color888>>16)&0xff;
		uint8_t G=(color888>>8)&0xff;
		uint8_t B=(color888>>0)&0xff;

		//формирование цветов парами сбаллансированных пикселей
		conv_color64[i*2]=get_ser_diff_data(tmds_encoder(R),tmds_encoder(G),tmds_encoder(B));
		// conv_color64[i*2+1]=conv_color64[i*2]^0xff03ffffffffffc0l;
		conv_color64[i*2+1]=conv_color64[i*2]^0x0003ffffffffffffl;
	};
	if (active_out==g_out_VGA){
		uint8_t conv0[] = { 0b00, 0b00, 0b01, 0b10, 0b10, 0b10, 0b11, 0b11 };
		uint8_t conv1[] = { 0b00, 0b01, 0b01, 0b01, 0b10, 0b11, 0b11, 0b11 };
		uint8_t b = ((color888 & 0xff) / 42);
		uint8_t g = (((color888 >> 8) & 0xff) / 42);
		uint8_t r = (((color888 >> 16) & 0xff) / 42);
		uint8_t c_hi = (conv0[r] << 4) | (conv0[g] << 2) | conv0[b];
		uint8_t c_lo = (conv1[r] << 4) | (conv1[g] << 2) | conv1[b];
		uint16_t palette16_mask=(0xc0<<8)|(0xc0);

		uint16_t* conv_color16=(uint16_t*) conv_color;
		conv_color16[i] = (((c_hi << 8) | c_lo) & 0x3f3f) | palette16_mask;
	};
	if ((active_out==g_out_TFT_ST7789)||
		(active_out==g_out_TFT_ST7789V)||
		(active_out==g_out_TFT_ILI9341)||
		(active_out==g_out_TFT_ILI9341V)||
		(active_out==g_out_TFT_GC9A01)){
		conv_color[i] = (uint16_t)color888;
	}
}

void graphics_set_buffer(uint8_t *buffer){
	g_buf.data = buffer;
}
void graphics_set_hud_buffer(uint8_t *buffer){
	g_buf.overlay = buffer;
}
void graphics_set_hud_handler(bool (*handler)(short)){
	g_buf.handler = handler;
}

bool graphics_try_framerate(g_out g_out,fr_rate rate, bool apply){
	V_MODE temp_mode;
	for (uint8_t idx=0;idx<MAX_ALLOWED_MODE;idx++){
		if(mode_table[idx].video_out==g_out){
			memcpy(&temp_mode,mode_table[idx].video_mode,sizeof(V_MODE));
			if(temp_mode.FR_RATE==rate){
				if(apply) memcpy(&hw_mode,mode_table[idx].video_mode,sizeof(V_MODE));
				return true;
			}
		}
	}
	return false;
}

void graphics_set_def_framerate(g_out v_out){
	for (uint8_t idx=0;idx<MAX_ALLOWED_MODE;idx++){
		if(mode_table[idx].video_out==v_out){
			memcpy(&hw_mode,mode_table[idx].video_mode,sizeof(V_MODE));
			break;
		}
	}
}

void graphics_set_rotate(rotate degree){
	g_buf.rotate =(uint8_t) degree;
}
void graphics_set_inversion(bool inversion){
	g_buf.inversion = inversion;
}
void graphics_set_pixels(uint8_t pixels){
	g_buf.pix_format = pixels;
}

g_out graphics_test_output(){
	uint8_t readings;
	uint8_t pull_up=0;
	uint8_t pull_down=0;
	for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
		gpio_init(idx);
		gpio_set_dir(idx,GPIO_IN);
		gpio_pull_up(idx);
	}
	sleep_ms(10);
	
	for(uint8_t rep=0;rep<5;rep++){
		readings=0;
		for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
			readings|=((uint8_t)gpio_get(idx))<<(idx-beginVideo_PIN);
		}
		sleep_ms(10);
		pull_up+=readings;
	}
	printf("PU[%02X]",pull_up);	
	for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
		gpio_deinit(idx);
		sleep_ms(1);
		gpio_init(idx);
		gpio_set_dir(idx,GPIO_OUT);
	}

	for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
		gpio_init(idx);
		gpio_set_dir(idx,GPIO_IN);
		gpio_pull_down(idx);
	}
	sleep_ms(10);
	for(uint8_t rep=0;rep<5;rep++){
		readings=0;
		for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
			readings|=((uint8_t)gpio_get(idx))<<(idx-beginVideo_PIN);
		}
		sleep_ms(10);
		pull_down+=readings;
	}
	printf(" PD[%02X]",pull_down);	
	for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
		gpio_deinit(idx);
		sleep_ms(1);
		gpio_init(idx);
		gpio_set_dir(idx,GPIO_OUT);
	}
	if((pull_up==0x13)&&(pull_down==0x00)){ //(pull_up==0xFF)&&
		return g_out_TFT_ILI9341;
	} else 
	if((pull_up==0x00)&&(pull_down==0x00)){
		return g_out_VGA;
	} else
	if((pull_up>0x00)&&(pull_down>0x00)){
		return g_out_HDMI;
	}
	return g_out_VGA;
}

//выделение и настройка общих ресурсов - 4 DMA канала, PIO программ и 2 SM
void graphics_init(g_out g_out,fr_rate rate){
	active_out=g_out;
	
	if(!graphics_try_framerate(active_out,rate,true)){
		graphics_set_def_framerate(active_out);
	}
	
	//настройка PIO
	SM_video = pio_claim_unused_sm(PIO_VIDEO, true);
	if(active_out<g_out_TFT_ST7789){
		SM_conv = pio_claim_unused_sm(PIO_VIDEO_ADDR, true);
	}
	//выделение  DMA каналов
	dma_chan_ctrl=dma_claim_unused_channel(true);
	dma_chan=dma_claim_unused_channel(true);
	if(active_out<g_out_TFT_ST7789){
		dma_chan_pal_conv_ctrl=dma_claim_unused_channel(true);
		dma_chan_pal_conv=dma_claim_unused_channel(true);
	} 


	//заполнение палитры по умолчанию(ч.б.)
	for(int ci=0;ci<240;ci++) graphics_set_palette(ci,(ci<<16)|(ci<<8)|ci);//

	//---------------

	uint offs_prg0=0;
	uint offs_prg1=0;
	const int base_inx=240;
	switch (active_out){
		case g_out_HDMI:
				offs_prg1= pio_add_program(PIO_VIDEO_ADDR, &pio_program_conv_addr_HDMI);
				offs_prg0 = pio_add_program(PIO_VIDEO, &program_PIO_HDMI);
				pio_set_x(PIO_VIDEO_ADDR,SM_conv,((uint32_t)conv_color>>12));
				//индексы в палитре>240 - служебные
				//240-243 служебные данные(синхра) напрямую вносим в массив -конвертер
				uint64_t* conv_color64=(uint64_t*) conv_color;
				uint16_t b0=0b1101010100;
				uint16_t b1=0b0010101011;
				uint16_t b2=0b0101010100;
				uint16_t b3=0b1010101011;

				conv_color64[2*base_inx+0]=get_ser_diff_data(b0,b0,b3);
				conv_color64[2*base_inx+1]=get_ser_diff_data(b0,b0,b3);

				conv_color64[2*(base_inx+1)+0]=get_ser_diff_data(b0,b0,b2);
				conv_color64[2*(base_inx+1)+1]=get_ser_diff_data(b0,b0,b2);

				conv_color64[2*(base_inx+2)+0]=get_ser_diff_data(b0,b0,b1);
				conv_color64[2*(base_inx+2)+1]=get_ser_diff_data(b0,b0,b1);

				conv_color64[2*(base_inx+3)+0]=get_ser_diff_data(b0,b0,b0);
				conv_color64[2*(base_inx+3)+1]=get_ser_diff_data(b0,b0,b0);

			break;
		case g_out_VGA:
			offs_prg1= pio_add_program(PIO_VIDEO_ADDR, &pio_program_conv_addr_VGA);
			offs_prg0 = pio_add_program(PIO_VIDEO, &program_pio_VGA);
			pio_set_x(PIO_VIDEO_ADDR,SM_conv,((uint32_t)conv_color>>9));
			uint16_t* conv_color16=(uint16_t*) conv_color;

			conv_color16[base_inx]  =0b1100000011000000;//нет синхры
			conv_color16[base_inx+1]=0b1000000010000000;//строчная только
			conv_color16[base_inx+2]=0b0100000001000000;//кадровая только
			conv_color16[base_inx+3]=0b0000000000000000;//кадровая и строчная
			break;
		case g_out_TFT_ST7789:
		case g_out_TFT_ST7789V:
		case g_out_TFT_ILI9341:
		case g_out_TFT_ILI9341V:
		case g_out_TFT_GC9A01:
			offs_prg0 = pio_add_program(PIO_VIDEO, &program_pio_TFT);
			break;
		default:
			return;
			break;
	};

	pio_sm_config c_c = pio_get_default_sm_config();

	//настройка PIO SM для конвертации
	if(active_out<g_out_TFT_ST7789){
		if (active_out==g_out_HDMI) sm_config_set_wrap(&c_c, offs_prg1 , offs_prg1 + (pio_program_conv_addr_HDMI.length-1));
		if (active_out==g_out_VGA) sm_config_set_wrap(&c_c, offs_prg1 , offs_prg1 + (pio_program_conv_addr_VGA.length-1));
		sm_config_set_in_shift(&c_c, true, false, 32);
		pio_sm_init(PIO_VIDEO_ADDR, SM_conv, offs_prg1, &c_c);
		pio_sm_set_enabled(PIO_VIDEO_ADDR, SM_conv, true);
		c_c = pio_get_default_sm_config();
	}

	//настройка PIO SM для вывода данных	
	float fdiv=0;
	uint32_t div_32=0;

	switch (active_out){
		case g_out_HDMI:
			//настройка рабочей SM HDMI
			sm_config_set_wrap(&c_c, offs_prg0 , offs_prg0 + (program_PIO_HDMI.length-1));
			//настройка side set(пины CLK дифф пары HDMI)
			sm_config_set_sideset_pins(&c_c,beginHDMI_PIN_clk);
			sm_config_set_sideset(&c_c,2,false,false);
			for(int i=0;i<2;i++){
				pio_gpio_init(PIO_VIDEO, beginHDMI_PIN_clk+i);
				gpio_set_drive_strength(beginHDMI_PIN_clk+i,GPIO_DRIVE_STRENGTH_4MA);
				gpio_set_slew_rate(beginHDMI_PIN_clk+i,GPIO_SLEW_RATE_FAST);
			}

			pio_sm_set_pins_with_mask(PIO_VIDEO, SM_video, 3u<<beginHDMI_PIN_clk, 3u<<beginHDMI_PIN_clk);
			pio_sm_set_pindirs_with_mask(PIO_VIDEO, SM_video,  3u<<beginHDMI_PIN_clk,  3u<<beginHDMI_PIN_clk);

			//пины - диффпары  HDMI
			for(int i=0;i<6;i++){
				gpio_set_slew_rate(beginHDMI_PIN_data+i,GPIO_SLEW_RATE_FAST);
				pio_gpio_init(PIO_VIDEO, beginHDMI_PIN_data+i);
				gpio_set_drive_strength(beginHDMI_PIN_data+i,GPIO_DRIVE_STRENGTH_4MA);
				gpio_set_slew_rate(beginHDMI_PIN_data+i,GPIO_SLEW_RATE_FAST);
			}
			pio_sm_set_consecutive_pindirs(PIO_VIDEO, SM_video, beginHDMI_PIN_data, 6, true);//конфигурация пинов на выход
			sm_config_set_out_pins(&c_c, beginHDMI_PIN_data, 6);
			//
			sm_config_set_out_shift(&c_c, true, true, 30);
			sm_config_set_fifo_join(&c_c,PIO_FIFO_JOIN_TX);

			pio_sm_init(PIO_VIDEO, SM_video, offs_prg0, &c_c);
			pio_sm_set_enabled(PIO_VIDEO, SM_video, true);

			fdiv=clock_get_hz(clk_sys)/(hw_mode.CLK_SPD);//частота пиксельклока
			//доступны только целый и полуцелый делитель
			//для разумных частот 1 и 1.5
			fdiv=(fdiv<1)?1:fdiv;
			fdiv=(fdiv>1.4)?1.5:fdiv;
			//fdiv=1;
			div_32=(uint32_t) (fdiv * (1 << 16)+0.0);
			PIO_VIDEO->sm[SM_video].clkdiv=div_32&0xffff8000; //делитель для конкретной sm , накладываем маску , чтобы делитель был с шагом в 0.5

			break;
		case g_out_VGA:  //настройка рабочей SM VGA
			sm_config_set_wrap(&c_c, offs_prg0 , offs_prg0 + (program_pio_VGA.length-1));
			for(int i=0;i<8;i++){
				gpio_set_slew_rate(beginVideo_PIN+i,GPIO_SLEW_RATE_FAST);
				pio_gpio_init(PIO_VIDEO, beginVideo_PIN+i);
				gpio_set_drive_strength(beginVideo_PIN+i,GPIO_DRIVE_STRENGTH_4MA);
				gpio_set_slew_rate(beginVideo_PIN+i,GPIO_SLEW_RATE_FAST);
			}
			pio_sm_set_consecutive_pindirs(PIO_VIDEO, SM_video, beginVideo_PIN, 8, true);//конфигурация пинов на выход
			sm_config_set_out_pins(&c_c, beginVideo_PIN, 8);

			sm_config_set_out_shift(&c_c, true, true, 32);
			sm_config_set_fifo_join(&c_c,PIO_FIFO_JOIN_TX);

			pio_sm_init(PIO_VIDEO, SM_video, offs_prg0, &c_c);
			pio_sm_set_enabled(PIO_VIDEO, SM_video, true);

			fdiv=clock_get_hz(clk_sys)/(2*hw_mode.CLK_SPD);//частота пиксельклока
			div_32=(uint32_t) (fdiv * (1 << 16)+0.0);
			PIO_VIDEO->sm[SM_video].clkdiv=div_32&0xfffff000; //делитель для конкретной sm
			break;
		case g_out_TFT_ST7789:
		case g_out_TFT_ST7789V:
		case g_out_TFT_ILI9341:
		case g_out_TFT_ILI9341V:
		case g_out_TFT_GC9A01:

    		//gpio_init(beginVideo_PIN);
			//gpio_init(beginVideo_PIN+2);
			//gpio_set_dir(beginVideo_PIN, GPIO_OUT);
			//gpio_set_dir(beginVideo_PIN+2, GPIO_OUT);
			//gpio_put(beginVideo_PIN, 1);
			//gpio_put(beginVideo_PIN+2, 1);

			sm_config_set_wrap(&c_c, offs_prg0+1 , offs_prg0 + (program_pio_TFT.length-1));
			sm_config_set_sideset(&c_c, 1, false, false);

		
			for(int i=0;i<8;i++){
				gpio_set_slew_rate(beginVideo_PIN+i,GPIO_SLEW_RATE_FAST);
				pio_gpio_init(PIO_VIDEO, beginVideo_PIN+i);
				gpio_set_drive_strength(beginVideo_PIN+i,GPIO_DRIVE_STRENGTH_4MA);
				gpio_set_slew_rate(beginVideo_PIN+i,GPIO_SLEW_RATE_FAST);
			}
			/*
			pio_gpio_init(PIO_VIDEO, beginVideo_PIN);
			pio_gpio_init(PIO_VIDEO, beginVideo_PIN+4);
			pio_gpio_init(PIO_VIDEO, beginVideo_PIN+6);
			pio_gpio_init(PIO_VIDEO, beginVideo_PIN+7);
			*/

				
			sm_config_set_set_pins(&c_c, beginVideo_PIN, 5);	// Configure DC CS pin to drive by (set) pio asm command
			sm_config_set_out_pins(&c_c, beginVideo_PIN+6, 1);	// Configure Data pin to drive by (out) pio asm command
			sm_config_set_sideset_pins(&c_c,beginVideo_PIN+7);	// Configure Clock pin to drive by (sideset) pio asm command
			pio_sm_set_consecutive_pindirs(PIO_VIDEO, SM_video, beginVideo_PIN, 8, true); //конфигурация пинов на выход
			sm_config_set_fifo_join(&c_c,PIO_FIFO_JOIN_TX);		// merge 4*32 bit RX stack to 4*32 bit TX stack
			sm_config_set_out_shift(&c_c, false, false, 32);	// 32 bit out maximum per one PULL command
			
			//sm_config_set_clkdiv(&c_c, hw_mode.CLK_SPD);
			sm_config_set_clkdiv(&c_c, TFT_CLK_DIV);

			uint32_t div = PIO_VIDEO->sm[SM_video].clkdiv;
			printf("div:%ld\n",div);

			pio_sm_init(PIO_VIDEO, SM_video, offs_prg0, &c_c);
			pio_sm_set_enabled(PIO_VIDEO, SM_video, true);
			/*
			fdiv=clock_get_hz(clk_sys)/(3*hw_mode.CLK_SPD);//частота пиксельклока
			div_32=(uint32_t) (fdiv * (1 << 16)+0.0);
			printf("div_32:%ld\n",div_32);
			PIO_VIDEO->sm[SM_video].clkdiv=div_32&0xfffff000; //делитель для конкретной sm
			*/
			/*fdiv=clock_get_hz(clk_sys)/(3*hw_mode.CLK_SPD);//частота пиксельклока
			div_32=(uint32_t) (fdiv * (1 << 16)+0.0);
			PIO_VIDEO->sm[SM_video].clkdiv=div_32&0xfffff000; //делитель для конкретной sm
			*/

			switch (active_out){
				case g_out_TFT_ST7789:
					//memcpy(tempcmd,init_seq_ST7789,sizeof(init_seq_ST7789));
					TFT_send_init(init_seq_ST7789);
					break;
				case g_out_TFT_ST7789V:
					TFT_send_init(init_seq_ST7789V);
					break;
			    case g_out_TFT_ILI9341:
		            TFT_send_init(init_seq_ILI9341);//init_seq_ILI9341
					break;
			    case g_out_TFT_ILI9341V:
		            TFT_send_init(init_seq_ILI9341V);//init_seq_ILI9341
					break;
			    case g_out_TFT_GC9A01:
		            TFT_send_init(init_seq_GC9A01);//init_seq_ILI9341
					break;
				default:
					break;
			};

/*
Reg FPS
00h 119
01h 111
02h 105
03h 99
04h 94
05h 90
06h 86
07h 82
08h 78
09h 75
0Ah 72
0Bh 69
0Ch 67
0Dh 64
0Eh 62
0Fh 60
10h 58
11h 57
12h 55
13h 53
14h 52
15h 50
16h 49
17h 48
18h 46
19h 45
1Ah 44
1Bh 43
1Ch 42
1Dh 41
1Eh 40
1Fh 39
*/
/*			
			tempcmd[0]=0xB2;
			tempcmd[1]=0x10; //0x08
			tempcmd[2]=0x10; //0x08
			tempcmd[3]=0x01; //0x00
			tempcmd[4]=0xF0; //0x22
			tempcmd[5]=0xF0; //0x22
			TFT_write_cmd(&tempcmd[0],6);
*/

			tempcmd[0]=0x35;
			TFT_write_cmd(&tempcmd[0],1);

			tempcmd[0]=0xC6;
		    tempcmd[1]=0x02;
			TFT_write_cmd(&tempcmd[0],2);
			/*
			tempcmd[0]=0xC6;
			switch (rate){
    			case rate_60Hz:
				tempcmd[1]=0x0F;
        		break;
    		case rate_72Hz:
				tempcmd[1]=0x0A;			
        		break;
    		case rate_75Hz:
				tempcmd[1]=0x09;			
		        break;
    		case rate_85Hz:
				tempcmd[1]=0x06;			
	        	break;
    		default:
				tempcmd[1]=0x15;
		        break;
    		}
			TFT_write_cmd(&tempcmd[0],2);
			*/
			

			tempcmd[0]=0x36; // Memory Access Control (36h)
			if((active_out==g_out_TFT_ILI9341)||(active_out==g_out_TFT_ILI9341V)){
				if(g_buf.pix_format==0){
					tempcmd[1]|=MADCTL_BGR;
				}
				if(g_buf.pix_format==1){
					tempcmd[1]&=~MADCTL_BGR;
				}
				switch ((rotate)g_buf.rotate){
					case rot_0Deg:
						tempcmd[1]|=MADCTL_MV;
						break;
					case rot_90Deg:
						tempcmd[1]|=MADCTL_MX;//MADCTL_MV
						break;
					case rot_180Deg:
						tempcmd[1]|=MADCTL_MV|MADCTL_MY|MADCTL_MX;
						break;
					case rot_360Deg:
						tempcmd[1]|=MADCTL_MY|MADCTL_ML;
						break;
					default:
						tempcmd[1]=0x00;
						break;	
				}
			}
			if((active_out==g_out_TFT_ST7789)||(active_out==g_out_TFT_ST7789V)){
				if(g_buf.pix_format==0){
					tempcmd[1]|=MADCTL_BGR;
				}
				if(g_buf.pix_format==1){
					tempcmd[1]&=~MADCTL_BGR;
				}
				switch ((rotate)g_buf.rotate){
					case rot_0Deg:
						tempcmd[1]|=MADCTL_MX|MADCTL_MV;
						break;
					case rot_90Deg:
						tempcmd[1]|=MADCTL_MX|MADCTL_ML;//MADCTL_MV
						break;
					case rot_180Deg:
						tempcmd[1]|=MADCTL_MY|MADCTL_MV;
						break;
					case rot_360Deg:
						tempcmd[1]|=MADCTL_MY|MADCTL_ML;
						break;
					default:
						tempcmd[1]=0x00;
						break;	
				}	
			}
			TFT_write_cmd(&tempcmd[0],2);
			if(g_buf.inversion==false){
				tempcmd[0]=0x20;
				TFT_write_cmd(&tempcmd[0],1);
			}
			if(g_buf.inversion==true){
				tempcmd[0]=0x21;
				TFT_write_cmd(&tempcmd[0],1);
			}


		    TFT_clr_scr(0x7BEF);

			//return;

		    /*for (int i = 0; i < sizeof conv_color; i++) {
		        graphics_set_palette(i, 0x0000);
		    }*/

			TFT_prepare_buffer();

			pio_sm_set_enabled(PIO_VIDEO, SM_video, false);
			PIO_VIDEO->instr_mem[offs_prg0+1] = 0x8880;
			PIO_VIDEO->instr_mem[offs_prg0+11] = 0x8080;
			pio_sm_set_enabled(PIO_VIDEO, SM_video, true);
			
			break;

		default:
			return;
			break;
	}

	//настройки DMA

	//основной рабочий канал
	dma_channel_config cfg_dma = dma_channel_get_default_config(dma_chan);
	
	if(active_out<g_out_TFT_ST7789){
		channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_8);
	} 
	if(active_out>g_out_HDMI){
		channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
	}
	channel_config_set_chain_to(&cfg_dma, dma_chan_ctrl);// chain to other channel
	channel_config_set_read_increment(&cfg_dma, true);
	channel_config_set_write_increment(&cfg_dma, false);


	uint dreq=0;

	if(active_out<g_out_TFT_ST7789){
		dreq=DREQ_PIO1_TX0+SM_conv;
		if (PIO_VIDEO_ADDR==pio0) dreq=DREQ_PIO0_TX0+SM_conv;
		channel_config_set_dreq(&cfg_dma, dreq);
		dma_channel_configure(
			dma_chan,
			&cfg_dma,
			&PIO_VIDEO_ADDR->txf[SM_conv],	// Write address
			lines_buf[0],					// read address
			hw_mode.H_len/1,					//
			false			 				// Don't start yet
		);
	} 
	if(active_out>g_out_HDMI){
		channel_config_set_dreq(&cfg_dma, pio_get_dreq(PIO_VIDEO, SM_video, true));
		dma_channel_configure(
			dma_chan,
			&cfg_dma,
			&PIO_VIDEO->txf[SM_video],	// Write address //&pio->txf[sm]
			&lines_buf[0],   // read address
			(hw_mode.H_len+1)/1,
			false			// Don't start yet
		);	
	}

	//контрольный канал для основного
	cfg_dma = dma_channel_get_default_config(dma_chan_ctrl);
	channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
	channel_config_set_chain_to(&cfg_dma, dma_chan);// chain to other channel
	
	channel_config_set_read_increment(&cfg_dma, true);
	channel_config_set_write_increment(&cfg_dma, false);
	channel_config_set_ring(&cfg_dma,false,2+N_LINE_BUF_log2);



	dma_channel_configure(
		dma_chan_ctrl,
		&cfg_dma,
		&dma_hw->ch[dma_chan].read_addr,	// Write address
		rd_addr_DMA_CTRL,					// read address
		1,									//
		false								// Don't start yet
	);

	if(active_out<g_out_TFT_ST7789){
		//канал - конвертер палитры
		cfg_dma = dma_channel_get_default_config(dma_chan_pal_conv);
		int n_trans_data=1;
		switch (active_out){
			case g_out_HDMI:
				channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
				n_trans_data=4;
				break;
			case g_out_VGA:
				channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_16);
				n_trans_data=1;
				break;
			default:
				break;
		}
		channel_config_set_chain_to(&cfg_dma, dma_chan_pal_conv_ctrl);// chain to other channel
		channel_config_set_read_increment(&cfg_dma, true);
		channel_config_set_write_increment(&cfg_dma, false);
		dreq=DREQ_PIO1_TX0+SM_video;
		if (PIO_VIDEO==pio0) dreq=DREQ_PIO0_TX0+SM_video;
		channel_config_set_dreq(&cfg_dma, dreq);
		dma_channel_configure(
			dma_chan_pal_conv,
			&cfg_dma,
			&PIO_VIDEO->txf[SM_video],	// Write address
			&conv_color[0],				// read address
			n_trans_data,				//
			false						// Don't start yet
		);
		//канал управления конвертером палитры
		cfg_dma = dma_channel_get_default_config(dma_chan_pal_conv_ctrl);
		channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
		channel_config_set_chain_to(&cfg_dma, dma_chan_pal_conv);// chain to other channel

		channel_config_set_read_increment(&cfg_dma, false);
		channel_config_set_write_increment(&cfg_dma, false);

		dreq=DREQ_PIO1_RX0+SM_conv;
		if (PIO_VIDEO_ADDR==pio0) dreq=DREQ_PIO0_RX0+SM_conv;

		channel_config_set_dreq(&cfg_dma, dreq);

		dma_channel_configure(
			dma_chan_pal_conv_ctrl,
			&cfg_dma,
			&dma_hw->ch[dma_chan_pal_conv].read_addr,	// Write address
			&PIO_VIDEO_ADDR->rxf[SM_conv],				// read address
			1,											//
			true										// start yet
		);
	}

	dma_start_channel_mask((1u << dma_chan_ctrl)) ;

    int hz=0;

    switch (rate){
    case rate_60Hz:
        hz=60000;
        break;
    case rate_72Hz:
        hz=72000;
        break;
    case rate_75Hz:
        hz=75000;
        break;
    case rate_85Hz:
        hz=85000;
        break;
    default:
        hz=30000;
        break;
    }
	if(active_out<g_out_TFT_ST7789){
		if (!add_repeating_timer_us(1000000 / hz, video_timer_callback, NULL, &video_timer)) {
			return ;
		}
	} else {
		hz=150000;
		if (!add_repeating_timer_us(1000000 / hz, video_timer_callback, NULL, &video_timer)) {
			return ;
		}

	}
};
