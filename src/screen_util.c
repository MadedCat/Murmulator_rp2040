#include "globals.h"
#include <string.h>
#include "screen_util.h"
#include "font8x8.h"
#include "font5x7.h"
#include "small_logo.h"

//#include "math.h"
#define ABS(x) (((x)<0)?(-(x)):(x))
#define SIGN(x) (((x)<0)?(-(1)):(1))

uint8_t* screen_buf;
int scr_W=1;
int scr_H=1;


void init_screen(uint8_t* scr_buf,int scr_width,int scr_height){
	screen_buf=scr_buf;
	scr_W=scr_width;
	scr_H=scr_height;
};

bool draw_pixel (int x,int y,color_t color){
	if(color==CL_EMPTY) return true;
	if ((x<0)|(x>=scr_W)|(y<0)|(y>=scr_H))  return false;
	uint8_t* pix=&screen_buf[(y*scr_W+x)/2];
	uint8_t tmp=*pix;
	if (x&1) *pix=(tmp&0xf)|((color&0xf)<<4);
	else *pix=((tmp&0xf0))|((color&0xf));
	return true;
}

void draw_text(int x,int y,const char* text,color_t colorText,color_t colorBg){
	if(strlen(text)==0) return;
	for(int line=0;line<FONT_H;line++){
		uint8_t* symb=(uint8_t*)text;
		int yt=y+line;
		int xt=x;
		while(*symb){
		   uint8_t symb_data=FONT_8x8_DATA[*symb][line];
		   for(int i=0;i<FONT_W;i++){
				if (symb_data&(1<<(FONT_W-1)))draw_pixel(xt++,yt,colorText); else draw_pixel(xt++,yt,colorBg);
				symb_data<<=1;
		   }
		   symb++;
		}
	}
}

void draw_text_len(int x,int y,const char* text,color_t colorText,color_t colorBg,int len){
	if(strlen(text)==0) return;
	//printf("text[%s]\t",text);

	for(int line=0;line<FONT_H;line++){
		uint8_t* symb=(uint8_t*)text;
		//printf("\nch[%2X]\t",*symb);
		//printf("addr[%4X]\t",*symb*FONT_H);
		int yt=y+line;
		int xt=x;
		int inx_symb=0;
		while(*symb){
			uint8_t symb_data=FONT_8x8_DATA[*symb][line];
			//printf("[%2X]\t",symb_data);
			for(int i=0;i<FONT_W;i++){
				if (symb_data&(1<<(FONT_W-1))){
					draw_pixel(xt++,yt,colorText);
				} else {
					draw_pixel(xt++,yt,colorBg);
				}
				symb_data<<=1;
		   }
		   symb++;
		   inx_symb++;
		   if (inx_symb>=len) break;
		}
		while (inx_symb<len){
			uint8_t symb_data=0;
		   for(int i=0;i<FONT_W;i++){
				if (symb_data&(1<<(FONT_W-1)))draw_pixel(xt++,yt,colorText); else draw_pixel(xt++,yt,colorBg);
				symb_data<<=1;
			}
			inx_symb++;

		}
	}
	//printf("\n");
}

void draw_rect(int x,int y,int w,int h,color_t color,bool filled){
	if(color==CL_EMPTY) return;
	int xb=x;
	int yb=y;
	int xe=x+w;
	int ye=y+h;

	xb=xb<0?0:xb;
	yb=yb<0?0:yb;
	xe=xe<0?0:xe;
	ye=ye<0?0:ye;

	xb=xb>scr_W?scr_W:xb;
	yb=yb>scr_H?scr_H:yb;
	xe=xe>scr_W?scr_W:xe;
	ye=ye>scr_H?scr_H:ye;	 
	for(int y=yb;y<ye;y++)
	for(int x=xb;x<xe;x++)
		if (filled)
			draw_pixel(x,y,color);
		else 
			if ((x==xb)|(x==(xe-1))|(y==yb)|(y==(ye-1))) draw_pixel(x,y,color);
	

}

void draw_line(int x0,int y0, int x1, int y1,color_t color){
   int dx=ABS(x1-x0);
   int dy=ABS(y1-y0);
   if (dx==0) { if (dy==0) return; for(int y=y0;y!=y1;y+=SIGN(y1-y0)) draw_pixel(x0,y,color) ;return; }
   if (dy==0) { if (dx==0) return; for(int x=x0;x!=x1;x+=SIGN(x1-x0)) draw_pixel(x,y0,color) ;return;  }
   if (dx>dy)
	{
		float k=(float)(x1-x0)/ABS(y1-y0);
		//float kold=0;
		float xf=x0+k;
		int x=x0;
		for(int y=y0;y!=y1;y+=SIGN(y1-y0))
		{
			int i=0;
			for(;i<ABS(xf-x);i++)
			{ 
				draw_pixel(x+i*SIGN(x1-x0),y,color) ;
			}
			x+=i*SIGN(x1-x0);
			xf+=k;
		}
	} else {
		float k=(float)(y1-y0)/ABS(x1-x0);
	   

		//float kold=0;
		float yf=y0+k;
		int y=y0;
		for(int x=x0;x!=x1;x+=SIGN(x1-x0))
		{
			int i=0;
			for(;i<ABS(yf-y);i++)
			{ 
				draw_pixel(x,y+i*SIGN(y1-y0),color) ;
			}
			y+=i*SIGN(y1-y0);
			yf+=k;
		}
	}

}


// #define M_PI		3.14159265358979323846
// void draw_circle(int x0,int y0,  int r,color_t color)
// {
//	 if (r==0) return;
//	 int x=x0+r;
//	 int y=y0; 
//	 for(float a=0;a<=2*M_PI;a+=1.0/(10+3*r))
//	 {
//		 int x1=x0+r*cos(a)+0.5;
//		 int y1=y0+r*sin(a)+0.5;
//		 draw_line(x,y,x1,y1,color);
//		 x=x1;
//		 y=y1;
//	 }

//	 draw_line(x,y,x0+r,y0,color);

// }

void ShowScreenshot(uint8_t* buffer,int xPos,int yPos){
	//draw_rect(17+FONT_W*14,16,182,192,0x0,true);
	uint8_t left = ((256/2)-(PREVIEW_WIDTH/2))/FONT_W;
	uint8_t width = (PREVIEW_WIDTH/FONT_W)+FONT_W;

	for(uint8_t segment=0;segment<3;segment++){
		for(uint8_t symbol_row=0;symbol_row<8;symbol_row++){
			for(uint8_t y=0;y<FONT_H;y++){
				for(uint8_t x=left;x<width;x++){			
					uint16_t pixel_addr = (segment*2048)+x+(y*256)+(symbol_row*32);
					uint16_t attr_addr = 0x1800+x+(symbol_row*32)+(segment*256);
					int yt=(y+yPos+(segment*64)+(symbol_row*FONT_W));//16+11
					int xt=((x*FONT_W)+xPos-(FONT_W*left));//17+FONT_W*14+11
					//printf("x:[%d]\ty:[%d]\txt:[%d]\tyt:[%d]\tseg:[%d]\tsymb:[%d]\taddr[%d]\n",x,y,xt,yt,segment,symb,pixel_addr);
					//printf("seg:[%d]\tsymbol_row:[%d]\taddr[%d]\tattr[%d]\n",segment,symbol_row,pixel_addr,attr_addr);
					uint8_t pixel_data=buffer[pixel_addr];
					uint8_t color_data=buffer[attr_addr];
					for(int i=0;i<8;i++){
						if((xt<(PREVIEW_POS_X+PREVIEW_WIDTH))&&(yt<(PREVIEW_POS_Y+PREVIEW_HEIGHT))){
							uint8_t bright_color = (color_data&0b01000000)>>3;
							uint8_t fg_color = color_data&0b00000111;
							uint8_t bg_color = (color_data&0b00111000)>>3;
							//printf("bright_color:[%02X]\tfg_color:[%02X]\tbg_color[%02X]\n",bright_color,fg_color,bg_color);
							if (pixel_data&(1<<(8-1)))draw_pixel(xt++,yt,fg_color^bright_color); else draw_pixel(xt++,yt,bg_color^bright_color); 
						}
						pixel_data<<=1;
					}
				}
			}
		}
	}
}


void draw_text5x7(int x,int y,char* text,color_t colorText,color_t colorBg){
	if(strlen(text)==0) return;
	for(int line=0;line<FONT_5x7_H;line++){
		uint8_t* symb=(uint8_t*)text;
		int yt=y+line;
		int xt=x;
		while(*symb){
		   uint8_t symb_data=FONT_5x7_DATA[*symb][line];
		   for(int i=0;i<FONT_5x7_W;i++) 
		   {
				if (symb_data&(1<<(FONT_5x7_W-1)))draw_pixel(xt++,yt,colorText); else draw_pixel(xt++,yt,colorBg);
				symb_data<<=1;
		   }
		   symb++;
		}
	}
}

void draw_text5x7_len(int x,int y,char* text,color_t colorText,color_t colorBg,int len){
	if(strlen(text)==0) return;
	//printf("text[%s]\t",text);

	for(int line=0;line<FONT_5x7_H;line++){
		uint8_t* symb=(uint8_t*)text;
		//printf("\nch[%2X]\t",*symb);
		//printf("addr[%4X]\t",*symb*FONT_5x7_H);
		int yt=y+line;
		int xt=x;
		int inx_symb=0;
		while(*symb){
			uint8_t symb_data=FONT_5x7_DATA[*symb][line];
			//printf("[%2X]\t",symb_data);
			for(int i=0;i<FONT_5x7_W;i++){
				if (symb_data&(1<<(FONT_5x7_W-1)))draw_pixel(xt++,yt,colorText); else draw_pixel(xt++,yt,colorBg);
				symb_data<<=1;
			}
			symb++;
			inx_symb++;
			if (inx_symb>=len) break;
		}
		while (inx_symb<len){
			uint8_t symb_data=0;
			for(int i=0;i<FONT_5x7_W;i++){
				if (symb_data&(1<<(FONT_5x7_W-1)))draw_pixel(xt++,yt,colorText); else draw_pixel(xt++,yt,colorBg);
				symb_data<<=1;
			}
			inx_symb++;
		}
	}
	//printf("\n");
}

void draw_bufline_text_len(uint8_t* ptr, int line, char* text,color_t colorText,color_t colorBg, int len){
	if(strlen(text)==0) return;
	if(line>FONT_H) return;
	uint8_t* symb=(uint8_t*)text;
	int inx_symb=0;
	int inx_ptr=0;
	while(*symb){
		uint8_t symb_data=FONT_8x8_DATA[*symb][line];
		//printf("[%2X]\t",symb_data);
		for(int i=0;i<FONT_W;i++){
			if (symb_data&(1<<(FONT_W-1))){
				ptr[inx_ptr++]=colorText;
			} else {
				ptr[inx_ptr++]=colorBg;
			}
			symb_data<<=1;
	   }
	   symb++;
	   inx_symb++;
	   if (inx_symb>=len) break;
	}
	while (inx_symb<len){
		uint8_t symb_data=0x00;
		for(int i=0;i<FONT_W;i++){
			if (symb_data&(1<<(FONT_W-1))){
				ptr[inx_ptr++]=colorText;
			} else {
				ptr[inx_ptr++]=colorBg;
			}
			symb_data<<=1;
		}
		inx_symb++;
	}
}

void draw_bufline_text5x7_len(uint8_t* ptr, int line,char* text,color_t colorText,color_t colorBg,int len){
	if(strlen(text)==0) return;
	if(line>FONT_5x7_H) return;
	uint8_t* symb=(uint8_t*)text;
	int inx_symb=0;
	int inx_ptr=0;
	while(*symb){
		uint8_t symb_data=FONT_5x7_DATA[*symb][line];
		//printf("[%02X]\t",symb_data);
		for(int i=0;i<FONT_5x7_W;i++){
			if (symb_data&(1<<(FONT_5x7_W-1))){
				ptr[inx_ptr++]=colorText;
			} else {
				ptr[inx_ptr++]=colorBg;
			}
			symb_data<<=1;
		}
		symb++;
		inx_symb++;
		if (inx_symb>=len) break;
	}
	while (inx_symb<len){
		uint8_t symb_data=0;
		for(int i=0;i<FONT_5x7_W;i++){
			if (symb_data&(1<<(FONT_5x7_W-1))){
				ptr[inx_ptr++]=colorText;
			} else {
				ptr[inx_ptr++]=colorBg;
			}
			symb_data<<=1;
		}
		inx_symb++;
	}
}


void draw_logo_header(short int xPos,short int yPos){
	draw_stripes(xPos, yPos);
	draw_text_len(xPos+SPEC_LOGO_W,yPos,"MURMULATOR",COLOR_ITEXT,CL_EMPTY,10);
}
