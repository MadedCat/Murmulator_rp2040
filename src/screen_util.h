#pragma once
#include "inttypes.h"
#include "stdbool.h"
#include "globals.h"

#define FONT_W (8)
#define FONT_H (8)

#define FONT_5x7_W (5)
#define FONT_5x7_H (7)

#define PREVIEW_POS_X (FONT_W+(FONT_W*FILE_NAME_LEN)+FONT_W)
#define PREVIEW_POS_Y (FONT_H*2)
#define PREVIEW_WIDTH (SCREEN_W-PREVIEW_POS_X-FONT_W)
#define PREVIEW_HEIGHT (SCREEN_H-(FONT_H*6))
#define PREVIEW_PAGE_5x7 (PREVIEW_HEIGHT/FONT_5x7_H)

/*
color:
	0x00 - black
	0x01 - blue
	0x02 - red
	0x03 - pink
	0x04 - green
	0x05 - cyan
	0x06 - yellow
	0x07 - gray
	0x08 - black
	0x09 - blue+
	0x0a - red+
	0x0b - pink+
	0x0c - green+
	0x0d - cyan+
	0x0e - yellow+
	0x0f - white
*/


#define CL_BLACK		0x00
#define CL_BLUE			0x01
#define CL_RED			0x02
#define CL_PINK			0x03
#define CL_GREEN		0x04
#define CL_CYAN			0x05
#define CL_YELLOW		0x06
#define CL_GRAY			0x07
#define CL_LT_BLACK		0x08
#define CL_LT_BLUE		0x09
#define CL_LT_RED		0x0A
#define CL_LT_PINK		0x0B
#define CL_LT_GREEN		0x0C
#define CL_LT_CYAN		0x0D
#define CL_LT_YELLOW	0x0E
#define CL_WHITE		0x0F
#define CL_EMPTY		0xFF

#define COLOR_TEXT			CL_BLACK	//normal text
#define COLOR_ITEXT			CL_WHITE	//inverse text
#define COLOR_DTEXT			CL_GRAY	 //disable text
#define COLOR_CURRENT_BG	CL_LT_CYAN
#define COLOR_CURRENT_TEXT	CL_BLACK
#define COLOR_SELECT_TEXT	CL_YELLOW
#define COLOR_BACKGOUND		CL_WHITE
//#define COLOR_BORDER		CL_BLACK
#define COLOR_PIC_BG		CL_GRAY
#define COLOR_FILE_BG		CL_GRAY
#define COLOR_MAIN_RAMK		CL_BLACK
#define COLOR_FULLSCREEN	CL_GRAY
#define COLOR_HUD			CL_BLUE

typedef uint8_t color_t;

bool draw_pixel(int x,int y,color_t color);
void draw_text(int x,int y,const char* text,color_t colorText,color_t colorBg);
void draw_text_len(int x,int y,const char* text,color_t colorText,color_t colorBg,int len);
void draw_line(int x0,int y0, int x1, int y1,color_t color);
//void draw_circle(int x0,int y0,  int r,color_t color);
void init_screen(uint8_t* scr_buf,int scr_width,int scr_height);
void draw_rect(int x,int y,int w,int h,color_t color,bool filled);
void ShowScreenshot(uint8_t* buffer,int xPos,int yPos);
void draw_text5x7(int x,int y,char* text,color_t colorText,color_t colorBg);
void draw_text5x7_len(int x,int y,char* text,color_t colorText,color_t colorBg,int len);
void draw_bufline_text_len(uint8_t* ptr, int line, char* text,color_t colorText,color_t colorBg, int len);
void draw_bufline_text5x7_len(uint8_t* ptr, int line,char* text,color_t colorText,color_t colorBg,int len);
void draw_logo_header(short int xPos,short int yPos);