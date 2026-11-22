#include "string.h"
#include "stdbool.h"
#include "stdio.h"
#include <stdalign.h> //Выравнивание массивов в памяти

#include "zx_machine.h"
#include "aySoundSoft.h"

//#include "util.h"
#include "zx_ROM.h"
#include "128-0.h" // 128k
#include "128-1.h" // 48k
#include "turbo48.h" // Turbo48
#include "pentagon.h" //with TRDOS menu 
#include "48turbo.h" // Turbo48/2
#include "2006.h" // Turbo48/2
#include "gluk_rom.h" // Turbo48/2


#include "z80.h"
#include "../globals.h"
#include "hardware/structs/systick.h"
#include "hardware/clocks.h"
#include "../wd1793.h"
#include "../util_tap.h"

/*
#if VGA_HDMI
	#include "video.h"
#endif
*/

//#define Z80_DEBUG
//extern bool allow_repaint;

bool ack_input;
bool stateFlash=true;
bool z80_gen_nmi_from_main = false;
bool im_z80_stop = false;
bool im_ready_loading = false;
bool vbuf_en=true;
bool slow_fr=false;
bool fr_changed=false;
bool covox_mode = false;// режим записи в ЦАП covox железного турбосаунда
uint32_t free_time_ticks;

//uint8_t tape_active;

//uint8_t zx_machine_last_out_7ffd;
uint8_t zx_machine_last_out_fffd;


volatile z80 cpu;

ZX_Input_t* zx_read_buffer;
ZX_Input_t* zx_write_buffer;
ZX_Input_t zx_input_emu[2];

bool zx_state_48k_MODE_BLOCK=false;
uint8_t zx_RAM_bank_active=3;

uint8_t zx_Border_color=0x00;
alignas(64) static uint32_t zx_colors_2_pix32[448];//предпосчитанные сочетания 2 цветов
static uint8_t* zx_colors_2_pix=(uint8_t*)&zx_colors_2_pix32;//предпосчитанные сочетания 2 цветов

uint8_t* zx_cpu_ram[4];//Адреса 4х областей памяти CPU при использовании страниц
uint8_t* zx_video_ram;//4 области памяти CPU

uint8_t* zx_ram_bank[8];//Хранит адреса 8ми банков памяти
uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))

typedef struct zx_vbuf_t{
	uint8_t* data;
	bool is_displayed;
}zx_vbuf_t;

zx_vbuf_t zx_vbuf[ZX_NUM_GBUF];
zx_vbuf_t* zx_vbuf_active;

//выделение памяти может быть изменено в зависимости от платформы
uint8_t RAM[ZX_RAM_PAGE_SIZE*ZX_RAM_PAGES]; //Реальная память куском 128Кб

uint8_t zx_7ffd_lastOut=0;

uint32_t __not_in_flash_func(get_ticks)(){
    return (uint32_t)(0xffffff-((uint32_t)systick_hw->cvr))&0xffffff;
}

uint8_t FAST_FUNC(zx_keyboardDecode)(uint8_t addrH){
	
	//быстрый опрос
	ack_input = true;

	switch (addrH){
		case 0b11111111: return 0xff;break;
		case 0b11111110: return ~zx_read_buffer->kb_data[0];break;
		case 0b11111101: return ~zx_read_buffer->kb_data[1];break;
		case 0b11111011: return ~zx_read_buffer->kb_data[2];break;
		case 0b11110111: return ~zx_read_buffer->kb_data[3];break;
		case 0b11101111: return ~zx_read_buffer->kb_data[4];break;
		case 0b11011111: return ~zx_read_buffer->kb_data[5];break;
		case 0b10111111: return ~zx_read_buffer->kb_data[6];break;
		case 0b01111111: return ~zx_read_buffer->kb_data[7];break;
	}
	
	//несколько адресных линий в 0 - медленный опрос
	uint8_t dataOut=0;
	
	for(uint8_t i=0;i<8;i++){
		if ((addrH&1)==0) dataOut|=zx_read_buffer->kb_data[i];//работаем в режиме нажатая клавиша=1
		addrH>>=1;
	};
	
	return ~dataOut;//инверсия, т.к. для спектрума нажатая клавиша = 0;
};

//функции чтения памяти и ввода-вывода
static uint8_t FAST_FUNC(read_z80)(void* userdata, uint16_t addr)
{
	if (addr<ZX_RAM_PAGE_SIZE) return zx_cpu_ram[0][addr];
	if (addr<32768) return zx_cpu_ram[1][addr-ZX_RAM_PAGE_SIZE];
	if (addr<49152) return zx_cpu_ram[2][addr-(ZX_RAM_PAGE_SIZE*2)];
	return zx_cpu_ram[3][addr-(ZX_RAM_PAGE_SIZE*3)];
}

static void FAST_FUNC(write_z80)(void* userdata, uint16_t addr, uint8_t val){
	if (addr<ZX_RAM_PAGE_SIZE) return;//запрещаем писать в ПЗУ
	if (addr<32768) {zx_cpu_ram[1][addr-ZX_RAM_PAGE_SIZE]=val;return;};
	if (addr<49152) {zx_cpu_ram[2][addr-(ZX_RAM_PAGE_SIZE*2)]=val;return;};
	zx_cpu_ram[3][addr-(ZX_RAM_PAGE_SIZE*3)]=val;
}

unsigned long prev_ticks, cur_ticks;

static uint8_t FAST_FUNC(in_z80)(z80* const z, uint8_t port) {
	uint8_t portH=z->_hi_addr_port;
	uint8_t portL=port;
	uint16_t port16=(portH<<8)|portL;

	#ifdef TRDOS_COMPILE
	if (TRDOS_mode && !TRDOS_disabled){
		//wd1793_port_busy=true;
		
		//printf("TRDos ports in: %02X\n",port);
		if (port == 0xFF){
			//printf("i(FF): %02X\n", (WD1793_GetRequests() & 0b11000000) | 0b00111111);
			return ((WD1793_GetRequests() & 0b11000000) | 0b00111111);
		}

		if ((port & 0x7F) == port){ //((port == 0x7F) || (port == 0x5F) || (port == 0x3F) || (port == 0x1F))
			//printf("i(%02X)(%02X): %02X\n",port,(port>>5) & 0b11), WD1793_Read((port>>5) & 0b11);
			return WD1793_Read((port>>5) & 0b11); // Read from 0x7F to 0x1F port
		}
		//printf("i(z80:%02X)\n",port);
		return 0xFF;
	}
	#endif
	
	
	if (port16&1){
		//printf("Read port: %04X\n",port16);
		uint16_t not_port16=~port16;
		
		//if (port16 == 0x7ffd) return 0xFF;  //fffd
		//if (((not_port16 & 0x8002) == 0x8002))//7ffd
		//if (port16!=0x7FFD)G_PRINTF("port 7FFD: %x\n",port16);
		
		//if (not_port16&0x20) {return zx_read_buffer->kempston;}//kempston{return 0xff;};
		
		//   //if (port16 == 0xfffd) return AY_get_reg();  //fffd
		//   if (((not_port16&0x0002)==0x0002)&&((port16&0xC000)==0xC000)) 
		//   {
		//	 //if (port16!=0xFFFD)G_PRINTF("port FFFD: %x\n",port16);		 
		//	 return AY_get_reg();  //fffd
		//   }
		if ((port16&0xC002)==0xC000){
			return AY_get_reg(0);  //fffd
		} 
		if (port16==0xFBDF){
			//printf("Read X: %02X\n",zx_read_buffer->kempston_mouse_x);
			return zx_read_buffer->kempston_mouse_x;
		} 
		if (port16==0xFFDF){
			//printf("Read Y: %02X\n",zx_read_buffer->kempston_mouse_y);
			return zx_read_buffer->kempston_mouse_y;
		} 
		if (port16==0xFADF){
			//printf("Read btn: %02X\n",zx_read_buffer->kempston_mouse_btn);
			return zx_read_buffer->kempston_mouse_btn;
		} 
		if (port16==0x021F){
			//printf("Read joy 0x021F: %02X\n",zx_read_buffer->kempston);
			//return zx_read_buffer->kempston & 0b00011111;
			return zx_read_buffer->kempston & 0b11111111;
		} else
		if ((port16&0x001F)==0x001F){
			//printf("Read joy 0x001F: %02X\n",zx_read_buffer->kempston);
			//return zx_read_buffer->kempston & 0b01111111;
			return zx_read_buffer->kempston & 0b11111111;
		}
	} else {
		//if (port16!=0x7FFE) printf(": %X ", port16);
		/*
			if ((port)==0xFE)
			{
			cur_ticks=cpu.cyc;
			printf("%d\n",cur_ticks-prev_ticks);
			prev_ticks=cur_ticks;
			}
		*/
		//загрузка с магнитофона и опрос клавиатуры
		ack_input=true;
		uint8_t out_data=zx_keyboardDecode(portH);
		out_data&=0b10111111;
		if(hw_zx_get_bit_LOAD()){
			out_data|=1<<6;
		}
		return(out_data);
	}
	return 0xFF;
}

void zx_machine_set_7ffd_out(uint8_t val){
	zx_RAM_bank_active=(val&0x7);
	printf("7FFD Page&data :%02X Bank:%02X\n",val,zx_RAM_bank_active);
	
	//			zx_cpu_ram[3]=zx_ram_bank[val&0x7];
	zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active];
	
	if (val&8) zx_video_ram=zx_ram_bank[7];else zx_video_ram=zx_ram_bank[5];
	//if (val&8) zx_video_ram=zx_ram_bank[6];else zx_video_ram=zx_ram_bank[4];
	if (val&16){
		zx_cpu_ram[0]=zx_rom_bank[0]; 
	} else {
		zx_cpu_ram[0]=zx_rom_bank[1]; //5bit = {1 - 48k[R0], 0 - 128k[R1]}
	}
	if (val&32) zx_state_48k_MODE_BLOCK=true; // 6bit = 1 48k mode block
	
};

uint8_t zx_machine_get_7ffd_lastOut(){return zx_7ffd_lastOut;}

static void FAST_FUNC(out_z80)(z80* const z, uint8_t port, uint8_t val) {
	uint8_t portH=z->_hi_addr_port;
	uint8_t portL=port;
	uint16_t port16=(portH<<8)|portL;
	

	#ifdef TRDOS_COMPILE
	if (TRDOS_mode && !TRDOS_disabled){ 
		//printf("TRDos ports out: %02X",port);
		if (port == 0xFF){
			wd1793_PortFF = val; 
			//printf("o(%02X), %02X\n", port, val);		  
		} 
		else
		if ((port & 0x7F) == port){ //((port == 0x7F) || (port == 0x5F) || (port == 0x3F) || (port == 0x1F))
			// Это нога, чья надо нога ;)
			if ((port == 0x1F) && ((val == 0xFC) || (val == 0x1C))) val = 0x18;

			//if (port == 0x1F) 
			//{
				//graph_buf[159] = 0xCC; // Мигаем когда есть комманды
				//graph_buf[319] = 0xCC; // Мигаем когда есть комманды
				//graph_buf[479] = 0xCC; // Мигаем когда есть комманды
				//printf("o(%02X)(%02X), %02X\n", port, (port>>5) & 0b11, val);
			//}
			WD1793_Write((port>>5) & 0b11, val); // Write to 0x7F 0x5F 0x3F 0x1F port
		}
		return;
	} else {
			//------------------------------------------soundrive&covox for soft PWM sound-----------------------------------
			Soundrive(port,val);	
			//-----------------------------------------covox for hardware TS---------------------------------
			if (port == 0xFB){
				if (!covox_mode){
					AY_select_reg(0xFF);		// выбор второго чипа
					AY_select_reg(0x07);		// выбор регистра разрешений
					AY_set_reg(0x80);			//разрешение записи в регистр portB
					AY_select_reg(0x0F);		// выбор IO регистра port B 
					covox_mode = true;
				}
				AY_set_reg(val);			// запись значения в регистр port B второго чипа
			}	
	}
	#endif	

	if (port16&1){
		
		//SAA1099
		if(port16 == 0x1FF){saa1099_write(1,val);}					
		if(port16 == 0x0FF){saa1099_write(0,val);}
		uint16_t not_port16=~port16;
		//чип AY
		//		  if (port16 == 0xFFFD){AY_select_reg(val);return;} //fffd
		//		   if (port16 == 0xBFFD){AY_set_reg(val);return;} //bffd
		
		if (((not_port16&0x0002)==0x0002)&&((port16&0xc000)==0xc000)){ 
			//if (port16!=0xFFFD)G_PRINTF("port FFFD: %x=%x\n",port16,val);	 
			//G_PRINTF("F:%x ",val);
			AY_select_reg(val); 
			zx_machine_last_out_fffd = val;
			covox_mode = false;					// режим записи в ЦАП covox железного турбосаунда отключить	
		} //fffd
		
		if (((not_port16&0x4002)==0x4002)&&((port16&0x8000)==0x8000)){ 
			//if (port16!=0xBFFD)G_PRINTF("port BFFD: %x=%x\n",port16,val);
			//G_PRINTF("B:%x ",val);
			AY_set_reg(val);
			return;
		} //bffd
		
		
		//Расширение памяти и экран Spectrum-128
		
		//  if ((port16==0x7FFD) && ((not_port16 & 0x8002) == 0x8002))
		//  z80_gen_nmi_from_main=true;
		//if ((port16==0x7FFD))
		if (((not_port16 & 0x8002) == 0x8002)){//7ffd
			// if (port16!=0x7FFD) 
			//	G_PRINTF("port16 must be 7FFD, but it is %X=val(%x)\n",port16,val);
			
			//G_PRINTF("[%X=%x] \n",port16,val);
			
			//zx_machine_last_out_7ffd = val;
			zx_7ffd_lastOut=val;
			
			if (zx_state_48k_MODE_BLOCK) return; //защёлка для 48к, запрешает манипуляцию банками
			
			
			//переключение банка памяти
			zx_RAM_bank_active=(val&0x7);
			//printf("7FFD Page&data :%02X\n",val);
			
			//			zx_cpu_ram[3]=zx_ram_bank[val&0x7];
			zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active];
			
			if (val&8) zx_video_ram=zx_ram_bank[7];else zx_video_ram=zx_ram_bank[5];
			//if (val&8) zx_video_ram=zx_ram_bank[6];else zx_video_ram=zx_ram_bank[4];
			if (val&16) {
				//memcpy();
				zx_cpu_ram[0]=zx_rom_bank[0];
			} else {
				zx_cpu_ram[0]=zx_rom_bank[1]; //5bit = {1 - 48k[R0], 0 - 128k[R1]}
			};
			if (val&32) zx_state_48k_MODE_BLOCK=true; // 6bit = 1 48k mode block
			return;
			//
		}; 
	} else {
		#define OUT_SND_MASK   (0b00011000)
		
		//hw_zx_set_snd_out (val&0b10000); // 10000
		//hw_zx_set_save_out(val&0b01000); // 01000
		hw_zx_set_beep_out(val&OUT_SND_MASK); // 01000
		

		//	if ((val&OUT_SND_MASK)!=oldSoundOutValue) {outSoundOutState^=1;digitalWrite(ZX_OUT_BEEP,outSoundOutState);};
		//	oldSoundOutValue=value&OUT_SND_MASK;
		//
		//digital_Write(ZX_OUT_BEEP,(value&0b00010000)?1:0); //если только звук без выхода записи#if (ZX_NUM_GBUF==1)
		zx_Border_color=((val&0x7)<<4)|(val&0x7);//дублируем для 4 битного видеобуфера
	}
	
};


//
//uint8_t ROM_BUF[2][ZX_RAM_PAGE_SIZE];

void zx_machine_init(){
	zx_read_buffer = &zx_input_emu[0];
	zx_write_buffer = &zx_input_emu[1];
	//привязка реальной RAM памяти к банкам
	memset(&RAM[0],0x00,ZX_RAM_PAGE_SIZE*ZX_RAM_PAGES);
	for(int i=0;i<ZX_RAM_PAGES;i++){ //uint8_t RAM[ZX_RAM_PAGE_SIZE*ZX_RAM_PAGES]
		zx_ram_bank[i]=&RAM[i*ZX_RAM_PAGE_SIZE];
	}
	
	
	//zx_ram_bank[0]=&RAM[0];
	//memset(zx_ram_bank[0],0xFF,ZX_RAM_PAGE_SIZE);

	/*
	memset(zx_ram_bank[0],0,ZX_RAM_PAGE_SIZE);
	zx_ram_bank[1]=&RAM[0*ZX_RAM_PAGE_SIZE];
	zx_ram_bank[2]=&RAM[1*ZX_RAM_PAGE_SIZE];
	zx_ram_bank[3]=&RAM[2*ZX_RAM_PAGE_SIZE];
	zx_ram_bank[4]=&RAM[3*ZX_RAM_PAGE_SIZE];
	zx_ram_bank[5]=&RAM[4*ZX_RAM_PAGE_SIZE];
	zx_ram_bank[6]=&RAM[5*ZX_RAM_PAGE_SIZE];
	zx_ram_bank[7]=&RAM[6*ZX_RAM_PAGE_SIZE];	
	*/
	//	привязка ROM памяти
	
	//zx_rom_bank[0]=&ROM[3*ZX_RAM_PAGE_SIZE];//48k
	//zx_rom_bank[1]=&ROM[2*ZX_RAM_PAGE_SIZE];//128k
	
	/*main rom config*/
	zx_rom_bank[0]=(uint8_t*)&ROM_PENTAGON[1*ZX_RAM_PAGE_SIZE];//48k		   //&turbo48_rom;//48k
	zx_rom_bank[1]=(uint8_t*)&ROM_PENTAGON[0*ZX_RAM_PAGE_SIZE];//128k с пунктом меню "TR-DOS"
	zx_rom_bank[2]=(uint8_t*)&ROM[1*ZX_RAM_PAGE_SIZE];//TRDOS
	zx_rom_bank[3]=(uint8_t*)&ROM[0*ZX_RAM_PAGE_SIZE];//GLUK

	/*main rom config*/
	/*experimental rom config*/
	/*
	zx_rom_bank[0]=&z2006_ROM[0];//base rom
	zx_rom_bank[1]=&GLUKPEN_ROM[0];//TR-DOS
	zx_rom_bank[2]=&GLUKPEN_ROM[0];//TR-DOS
	zx_rom_bank[3]=&ROM_PENTAGON[0*ZX_RAM_PAGE_SIZE];//128k с пунктом меню "TR-DOS"
	*/
	/*experimental rom config*/

	
	/*experimental rom config/
	zx_rom_bank[0]=&ROM[0*ZX_RAM_PAGE_SIZE];//48k &ROM[1*ZX_RAM_PAGE_SIZE];//GLUK
	zx_rom_bank[1]=&GLUKPEN_ROM[0];//TRDOS
	zx_rom_bank[2]=NULL;
	zx_rom_bank[3]=NULL;
	/*experimental rom config*/

	

	/*
	// memcpy(ROM_BUF[0],&ROM[3*ZX_RAM_PAGE_SIZE],ZX_RAM_PAGE_SIZE);
	// memcpy(ROM_BUF[1],&ROM[2*ZX_RAM_PAGE_SIZE],ZX_RAM_PAGE_SIZE);
	// zx_rom_bank[0]=ROM_BUF[0];
	// zx_rom_bank[1]=ROM_BUF[1];
	//zx_rom_bank[0]=fuse_roms_turbo48_rom;  //48k turbo  
	// zx_rom_bank[0]=fuse_roms_128_1_rom;//48k
	// zx_rom_bank[1]=fuse_roms_128_0_rom;//128k
	*/
	
	zx_cpu_ram[0]=zx_rom_bank[1]; // 0x0000 - 0x3FFF
	//zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF

	//zx_cpu_ram[0]=zx_rom_bank[3]; // 0x0000 - 0x3FFF //Autoboot TRD

	zx_cpu_ram[1]=zx_ram_bank[5]; // 0x4000 - 0x7FFF
	zx_cpu_ram[2]=zx_ram_bank[2]; // 0x8000 - 0xBFFF
	zx_cpu_ram[3]=zx_ram_bank[3]; // 0xC000 - 0x7FFF
	zx_video_ram=zx_ram_bank[5];
	//zx_video_ram=zx_ram_bank[4];
	zx_RAM_bank_active=3;
	zx_state_48k_MODE_BLOCK=false;

	//printf("\nInit cpu_ram %04X rom_bank %04X\n", zx_cpu_ram[0], zx_rom_bank[0]);
	//выделение графических буферов
	// for(int i=0;i<ZX_NUM_GBUF;i++){
	//	 zx_vbuf[i].is_displayed=true;
	//	 zx_vbuf[i].data=&VBUFS[i*(ZX_SCREENW*ZX_SCREENH*ZX_BPP/8)];
	// }
	zx_vbuf[0].is_displayed=true;
	zx_vbuf[0].data=graph_buf;
	zx_vbuf_active=&zx_vbuf[0];
	
	//инициализация процессора
	
	z80_init((z80*)&cpu);
	cpu.read_byte = read_z80;   // Присваиваем процедуру read_z80 структуре z80 (Процедура cpu.readbyte)
	cpu.write_byte = write_z80; // Аналогично
	cpu.port_in = in_z80;	   // Аналогично
	cpu.port_out = out_z80;	 // Аналогично
	
	
	printf("zx machine initialized\n");
};

void zx_machine_reset(bool trdos){

	memset(&RAM[0],0x00,ZX_RAM_PAGE_SIZE*ZX_RAM_PAGES);
	z80* z= (z80*)&cpu;
	// z->cyc = 0;

	TRDOS_mode = false;	 // Текущий режим - ROM TRDOS или стандартный ROM 48k
	TRDOS_disabled = false; // Запрет входить в TRDOS
	tap_loader_active&=~TAPE_EXTERNAL;

	//memcpy(&RAM[0], &ROM_PENTAGON[1*ZX_RAM_PAGE_SIZE],ZX_RAM_PAGE_SIZE);

	//zx_cpu_ram[0]=zx_rom_bank[1]; // 0x0000 - 0x3FFF
	
	zx_cpu_ram[0]=zx_rom_bank[1]; // 0x0000 - 0x3FFF

	//zx_cpu_ram[0]=zx_rom_bank[2]; // 0x0000 - 0x3FFF  //Autoboot TRD
	//TRDOS_mode = true;
	

	zx_cpu_ram[1]=zx_ram_bank[5]; // 0x4000 - 0x7FFF
	zx_cpu_ram[2]=zx_ram_bank[2]; // 0x8000 - 0xBFFF
	zx_cpu_ram[3]=zx_ram_bank[3]; // 0xC000 - 0x7FFF
	//zx_video_ram=zx_ram_bank[5];
	zx_video_ram=zx_ram_bank[5];

	zx_RAM_bank_active=3;
	zx_state_48k_MODE_BLOCK=false;
	
	zx_vbuf[0].is_displayed=true;
	zx_vbuf[0].data=graph_buf;
	zx_vbuf_active=&zx_vbuf[0];						
	
	z->pc = 0;
	z->sp = 0xFFFF;
	z->ix = 0;
	z->iy = 0;
	z->mem_ptr = 0;
	
	// af and sp are set to 0xFFFF after reset,
	// and the other values are undefined (z80-documented)
	z->a = 0xFF;
	z->b = 0;
	z->c = 0;
	z->d = 0;
	z->e = 0;
	z->h = 0;
	z->l = 0;
	
	z->a_ = 0;
	z->b_ = 0;
	z->c_ = 0;
	z->d_ = 0;
	z->e_ = 0;
	z->h_ = 0;
	z->l_ = 0;
	z->f_ = 0;
	
	z->i = 0;
	z->r = 0;
	
	z->sf = 1;
	z->zf = 1;
	z->yf = 1;
	z->hf = 1;
	z->xf = 1;
	z->pf = 1;
	z->nf = 1;
	z->cf = 1;
	
	z->iff_delay = 0;
	z->interrupt_mode = 0;
	z->iff1 = 0;
	z->iff2 = 0;
	z->halted = 0;
	z->int_pending = 0;
	z->nmi_pending = 0;
	z->int_data = 0;
	AY_reset(0);

	#ifdef TRDOS_COMPILE
	if (trdos){
		TRDOS_disabled = false;
		TRDOS_mode = true;
		zx_7ffd_lastOut|=16;
		zx_cpu_ram[0]=zx_rom_bank[2]; // подмена ПЗУ на TRDOS
		//zx_cpu_ram[0]=zx_rom_bank[2]; // подмена ПЗУ на TRDOS
	}
	#endif	

};


void FAST_FUNC(zx_machine_input_set)(){
	ZX_Input_t* temp_buffer_ptr = zx_read_buffer;
	zx_read_buffer = zx_write_buffer;
	zx_write_buffer = temp_buffer_ptr;
	memcpy(zx_write_buffer,zx_read_buffer,sizeof(ZX_Input_t));
};

void zx_machine_NMI(){
	z80_gen_nmi_from_main=true;
}


uint8_t* FAST_FUNC(zx_machine_screen_get)(uint8_t* current_screen){
	#if (ZX_NUM_GBUF==1)
		return zx_vbuf[0].data; //если буфер 1, то вариантов нет
	#else
		//для нескольких буферов надо возвращать ранее неотображённый, если найдётся
		uint8_t* out_data=current_screen;
		zx_vbuf_t* current_out_zx_vbuf=NULL;
		for(int i=0;i<ZX_NUM_GBUF;i++)
		{
			if (zx_vbuf[i].data==current_screen) current_out_zx_vbuf=&zx_vbuf[i];//запомнить текущий буфер
			if (!zx_vbuf[i].is_displayed) out_data=zx_vbuf[i].data;//неотображённый ещё буфер
			
		}
		//если нашли неотображённый ранее буфер экрана, прошлый надо освободить для отрисовки
		if ((out_data!=current_screen)&&(current_out_zx_vbuf!=NULL)) current_out_zx_vbuf->is_displayed=true;
		
		
		return out_data;
		
	#endif
};

void FAST_FUNC(zx_machine_flashATTR)(void){
	stateFlash^=1;
	if (stateFlash) memcpy(zx_colors_2_pix+512,zx_colors_2_pix,512); else memcpy(zx_colors_2_pix+512,zx_colors_2_pix+1024,512);
}
//инициализация массива предпосчитанных цветов
void init_zx_2_pix_buffer(){
	for(uint16_t i=0;i<384;i++)	{
		uint8_t color=(uint8_t)i&0x7f;
		uint8_t color0=(color>>3)&0xf;
		uint8_t color1=(color&7)|(color0&0x08);
		
		//убрать ярко чёрный
		//  if (color0==0x80) color0=0;
		//  if (color1==0x80) color1=0;
		
		if (i>128){
			//инверсные цвета для мигания
			uint8_t color_tmp=color0;
			color0=color1;
			color1=color_tmp;
		}
		
		for(uint8_t k=0;k<4;k++){
			switch (k){
				case 0:	zx_colors_2_pix[i*4+k]=(color0<<4)|color0;break;
				case 2: zx_colors_2_pix[i*4+k]=(color0<<4)|color1;break;
				case 1: zx_colors_2_pix[i*4+k]=(color1<<4)|color0;break;
				case 3: zx_colors_2_pix[i*4+k]=(color1<<4)|color1;break;
				/*
				case 0:	zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color0<<4)|color0:(zx_color[color0]<<8)|zx_color[color0];break;
				case 2: zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color0<<4)|color1:(zx_color[color0]<<8)|zx_color[color1];break;
				case 1: zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color1<<4)|color0:(zx_color[color1]<<8)|zx_color[color0];break;
				case 3: zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color1<<4)|color1:(zx_color[color1]<<8)|zx_color[color1];break;
				*/
			}
		}
	}
}

#ifdef TRDOS_COMPILE
	uint32_t dt_cpu=0;
#endif

uint8_t* active_screen_buf=NULL;

uint8_t getF() {
	uint8_t val = 0;
	val |= cpu.cf << 0;
	val |= cpu.nf << 1;
	val |= cpu.pf << 2;
	val |= cpu.xf << 3;
	val |= cpu.hf << 4;
	val |= cpu.yf << 5;
	val |= cpu.zf << 6;
	val |= cpu.sf << 7;
	return val;
}

void setF(uint8_t val) {
	cpu.cf = (val >> 0) & 1;
	cpu.nf = (val >> 1) & 1;
	cpu.pf = (val >> 2) & 1;
	cpu.xf = (val >> 3) & 1;
	cpu.hf = (val >> 4) & 1;
	cpu.yf = (val >> 5) & 1;
	cpu.zf = (val >> 6) & 1;
	cpu.sf = (val >> 7) & 1;
}

uint16_t getAF() {
	uint16_t val = 0;
	val = (cpu.a<<8) | getF();
	return val;
}

void setAF(uint16_t v) {
	cpu.a = (v & 0xFF00) >> 8;
	setF(v & 0xFF);
}

uint16_t getAF_() {
	uint16_t val = 0;
	val = (cpu.a_<<8) | cpu.f_;
	return val;
}

void setAF_(uint16_t v) {
	cpu.a_ = (v & 0xFF00) >> 8;
	cpu.f_ = (v & 0xFF);
}

void FastTapeLoad() {
	uint16_t af = getAF();
	uint16_t af_ = getAF_();

	if(cpu.cf!=1) return;

	uint16_t blocktype = cpu.a;

	uint16_t auxAddress = cpu.ix;
 	uint16_t auxLen = (cpu.d<<8)|cpu.e;

	printf("FastTapeLoad> id>%02X   start>%04x   len>%04x\n",blocktype,auxAddress,auxLen);

	//printf("> AF>%04X  AF`>%04x\n",af,af_);

	if (FastLoadTAP(blocktype,auxAddress,auxLen)){
		af_=getAF_();
		setAF_(af_ | 64);
		printf("Normal load\n");
		cpu.pc=0x05E2;
	} else {
		af_=getAF_();
		setAF_(af_ & 190);
		printf("Load Error\n");
		cpu.pc=0x056B;
	}
	printf("FastTapeLoad return\n");
}

void FAST_FUNC(zx_machine_main_loop_start)(){
	//void (zx_machine_main_loop_start)(){
	//void __not_in_flash_func(zx_machine_main_loop_start)(){
	
	//char icon[2];
	//переменные для отрисовки экрана
	uint32_t inx_tick_screen=0;
	uint64_t tick_cpu=0; // Количество тактов до выполнения команды Z80
	uint32_t x=0;
	uint32_t y=0;
	zx_screen_refresh=false;
	init_zx_2_pix_buffer();
	uint8_t* p_scr_str_buf=NULL;
	uint8_t* p_zx_video_ram=NULL;
	uint8_t* p_zx_video_ramATTR=NULL;
	
	printf("zx mashine starting\n");
	
	// uint64_t dst_time_ns=ext_get_ns();
	uint32_t d_dst_time_ticks=0; // Количесто тактов реального процессора на текущую выполненную команду Z80
	uint32_t t0_time_ticks=0;	// Количество реальных тактов процессора после запуска машины Z80

	//int ticks_per_cycle=72;//72;	 //от 254МГц: 72 - 3.5МГц, 63 - 4Мгц;
	//int ticks_per_cycle=82;//72;	 //от 290.4МГц: 82 - 3.5МГц, 72 - 4Мгц;
	int ticks_per_cycle=(long)clock_get_hz(clk_sys)/3500000;//82;//72;	 //от 290.4МГц: 82 - 3.5МГц, 72 - 4Мгц;
	printf("Real ticks_per_cycle:%d @ %ld\n",ticks_per_cycle,(long)clock_get_hz(clk_sys));

	systick_hw->csr = 0x5;
	systick_hw->rvr = 0xFFFFFF;
	
	//G_PRINTF_DEBUG("time_tick=%ld dtime tick=%d\n",get_ticks(),(1-0xffff00)&0xffffff);//test
	//G_PRINTF_DEBUG("time_tick=%ld dtime tick=%d\n",get_ticks(),(1-0xffff00)&0xffffff);//test
	
	//		bool get_next_PC = 0;
	//uint16_t get_next_codes = 0;
	#ifdef Z80_DEBUG
		bool ttest = false; // Флаг для вызова дебажной информации
	#endif
	
	//init_screen(graph_buf,320,240);
	active_screen_buf=graph_buf;
	p_scr_str_buf=active_screen_buf; 
	
	int skip_frame=1;

	//смещение начала изображения от прерывания
	//const int shift_img=(16+40)*224+44;////8888;////Пентагон=(16+40)*224+48;
	const int shift_img=(16+40)*224+50;////8888;////Пентагон=(16+40)*224+48;
	//вспомогательный индекс такта внутри картинки
	int draw_img_inx=0;
	const int ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A //const
	const int T_per_line = 224; //224
	bool int_en=true;
	//работа с аттрибутами цвета
	register uint8_t old_c_attr=0;
	register uint8_t old_zx_pix8=0;
	register uint32_t colorBuf;
	
	uint16_t af;
	uint16_t af_;
	uint32_t dt_cpu;

	while(1){

		while (im_z80_stop){
			sleep_ms(5);
			if (im_ready_loading==false) im_ready_loading = true;
			// inx_tick_screen=0;
			#ifdef Z80_DEBUG
				ttest=true;
			#endif
			cpu.int_pending = false;
		}
	
		/*
		#ifdef Z80_DEBUG
			if (ttest){
				sleep_ms(10);
				z80_debug_output(&cpu);
				printf("R_Bank: %d\n", zx_RAM_bank_active);
				ttest=false;
			}
		#endif
		*/
		if(tapeFileSize>0){
			if((cpu.pc==0x0556)||(cpu.pc==0x056C)){ // START LOAD
				tap_loader_active|=TAPE_ROM_READY;
			}
			if(cpu.pc==0x05E2){ // END LOAD / SAVE
				tap_loader_active&=~TAPE_ROM_READY;
			}			
			if(tap_loader_active&TAPE_INTERNAL_AUTO){
				if((cpu.pc==0x0556)){ // START LOAD //||(cpu.pc==0x056C)
					//printf("zx>tap_loader_active:[%02X][%02X][%08lX]\n",tap_loader_active,TapeStatus,tapeFileSize);
					if (TapeStatus!=TAPE_LOADING && tapeFileSize>0){
						TAP_Play();
					}
				}
			
				if(cpu.pc==0x04d0){ // START SAVE (used for rerouting mic out to speaker in Ports.cpp)
				SaveStatus=TAPE_SAVING;
			}
				if(cpu.pc==0x053F){// END LOAD / SAVE
				if(tap_loader_active&TAPE_INTERNAL_AUTO){
					//printf("Tap load autostop %d\n",TapeStatus);
					//printf("Autostop3 %d\n",tape_autoload_status);
					if (TapeStatus!=TAPE_STOPPED){
						if (cpu.cf){
							TapeStatus=TAPE_PAUSED;
							//ticks_per_frame>>1;
							//printf("ROM TAPE_PAUSED %X\n",cpu.cf);
						} else{
							TapeStatus=TAPE_STOPPED;
							//printf("ROM TAPE_STOPPED %X\n",cpu.cf);
							//ticks_per_frame>>1;
							tap_loader_active = TAPE_OFF;
						}
						SaveStatus=SAVE_STOPPED;
					} 
				}
				/*if(cpu.pc==0x05E2){ // END LOAD / SAVE
					//printf("Autostop2 %d\n",tape_autoload_status);
					tap_loader_active = TAPE_OFF;
				}*/
			}
			} else 
			if(tap_loader_active&TAPE_INTERNAL_ROM){
				if((cpu.pc==0x0556)||(cpu.pc==0x056C)){ // START LOAD
					FastTapeLoad();
				}
			}
		} else if(tapeFileSize==0){
			if(cpu.pc==0x0556){ // START LOAD
				//printf("Set TAPE_EXTERNAL[%04X]\n",cpu.pc);
				tap_loader_active|=TAPE_EXTERNAL;
			}
			/*if(cpu.pc==0x053F){// END LOAD / SAVE
				printf("Clear TAPE_EXTERNAL[%04X]\n",cpu.pc);
				tap_loader_active&=~TAPE_EXTERNAL;
			}*/
		}

		#ifdef TRDOS_COMPILE
			if (!TRDOS_disabled){
				if (TRDOS_mode==false){
					//if(((cpu.pc & 0x3D00) == 0x3D00)  && (zx_7ffd_lastOut&16)){ /*&&  (!TRDOS_mode)) */ //  (zx_7ffd_lastOut&16) 1 - 48k, 0-128k
					if(((cpu.pc >= 0x3D00)&&(cpu.pc < 0x3FFF))  && (zx_7ffd_lastOut&16)){
						#ifdef Z80_DEBUG
						z80_debug_output(&cpu);
						printf("zx_7ffd_lastOut:%02X \n\n",zx_7ffd_lastOut);
						#endif
						TRDOS_mode=true;
						zx_cpu_ram[0]=zx_rom_bank[2]; // подмена ПЗУ на TRDOS
						//zx_cpu_ram[0]=zx_rom_bank[2]; // подмена ПЗУ на TRDOS
					}
				}
				if ((cpu.pc > 0x3FFF) && (TRDOS_mode)){
					TRDOS_mode=false;
					zx_cpu_ram[0]=zx_rom_bank[!(zx_7ffd_lastOut & 16)];
				}
			}
		#endif

		//if (inx_tick_screen<36) z80_gen_int(&cpu,0xFF);
		// while (ext_get_ns()<dst_time_ns) ;//ext_delay_us(1);
		// Цикл ождания пока количество потраченных тактов рального процессора
		// меньше количества расчетных тактов реального процессора на команду Z80
		
		t0_time_ticks=(t0_time_ticks+d_dst_time_ticks)&0xffffff;		 
		tick_cpu=cpu.cyc;				 // Запоминаем количество тактов Z80 до выполнения команды Z80
		z80_step((z80*)&cpu);				   // Выполняем очередню команду Z80
		dt_cpu=cpu.cyc-tick_cpu; // Вычисляем количество тактов Z80 на выполненную команду.
		d_dst_time_ticks=dt_cpu*ticks_per_cycle; // Расчетное количесто тактов реального процессора на воплненную команду Z80
		inx_tick_screen+=dt_cpu;		  //Увеличиваем на количество тактов Z80 на текущую выполненную команду.
		

		#ifdef TRDOS_COMPILE
			if (TRDOS_mode && !TRDOS_disabled){
				WD1793_timer(dt_cpu);
				WD1793_Execute();
			}
			//trd_50hz_fired=true;
		#endif


		while (((get_ticks()-t0_time_ticks)&0xffffff)<d_dst_time_ticks);

		//if ((inx_tick_screen<32)&&(int_en)) {z80_gen_int(&cpu,0xFF);int_en=false;}
		// if ((inx_tick_screen<32)&&(int_en)) {int_en=false;}
		if ((inx_tick_screen<32)&&(int_en)) {
			int_en=false;
			ack_input=true;

		} //ack_input=true;

		if (inx_tick_screen>=ticks_per_frame){	   // Если прошла 1/50 сек, 71680 тактов процессора Z80
			int_en=true;
			z80_gen_int((z80*)&cpu,0xFF);
			inx_tick_screen-=ticks_per_frame; //Такты Z80 1/50 секунды
			//start_menu_tick();
			x=0;y=0;
			draw_img_inx=0; 
			p_scr_str_buf=active_screen_buf; 
			if (inx_tick_screen==0) continue;
		};

		if(int_en){
			free_time_ticks = inx_tick_screen;
			/*if(inx_tick_screen>ticks_per_frame){
				continue;
			}*/
		}

		// Если нажаликлавишу NMI
		if (z80_gen_nmi_from_main){
			z80_gen_nmi((z80*)&cpu);
			z80_gen_nmi_from_main = false;
		}

		
		/*if (slow_fr&&fr_changed){
			ticks_per_cycle=ticks_per_cycle>>3;
			//printf("TPC:%d\n",ticks_per_cycle);
			fr_changed=false;
		}
		if ((!slow_fr)&&fr_changed){
			ticks_per_cycle=ticks_per_cycle<<3;
			//printf("TPC:%d\n",ticks_per_cycle);
			fr_changed=false;
		}*/
		

		/*if (skip_frame%15==0){
			//printf("Skip frame\n");
			continue;
		}*/		


		//if(d_dst_time_ticks>1700){continue;};

		//if(((get_ticks()-t0_time_ticks)&0xffffff)>d_dst_time_ticks){ continue;};
		/*
		*/
		//if (active_screen_buf==NULL) continue;
		
		if (!vbuf_en) continue;
		//if (allow_repaint) continue;		
		//новая прорисовка
		register int img_inx=(inx_tick_screen-shift_img);
		if (img_inx<0 || (img_inx>=(T_per_line*240))){ //область изображения, если вне, то не рисуем
			continue;
		}
		//смещения бордера
		const int dy=24;
		const int dx=32;
		zx_screen_refresh=true;
		for(;draw_img_inx<img_inx;){
			
			if (x==T_per_line*2) {
				x=0;
				y++;
				int ys=y-dy;//номер строки изображения
				p_zx_video_ram=zx_video_ram+(((ys&0b11000000)|((ys>>3)&7)|((ys&7)<<3))<<5);
				//указатель на начало строки байтов цветовых аттрибутов
				p_zx_video_ramATTR=zx_video_ram+(6144+((ys<<2)&0xFFE0));
				
			}; 
			if ((x>=(SCREEN_W))||(y>=(SCREEN_H))){
				x+=8;
				draw_img_inx+=4;
				continue;
			} 
			if((y<dy)||(y>=192+dy)||(x>=256+dx)||(x<dx)){//условия для бордера
				int i_c;
				if (x<dx) i_c=MIN((dx-x)/2,img_inx-draw_img_inx);
				else i_c=MIN((SCREEN_W-x)/2,img_inx-draw_img_inx);
				
				register uint8_t bc=zx_Border_color;
				for(int i=i_c;i--;) *p_scr_str_buf++=bc;

				//  p_scr_str_buf+=i_c;//test
				draw_img_inx+=i_c;
				x+=i_c<<1;

				continue;
			}
			uint8_t c_attr=*p_zx_video_ramATTR++;
			uint8_t zx_pix8=*p_zx_video_ram++;
			
			if (old_c_attr!=c_attr){//если аттрибуты цвета не поменялись - используем последовательность с прошлого шага
				// uint8_t* zx_colors_2_pix_current=zx_colors_2_pix+(4*(c_attr));
				// colorBuf=zx_colors_2_pix_current);
				colorBuf=*((zx_colors_2_pix32+c_attr));
				old_c_attr=c_attr;
				
			}
			
			
			//вывод блока из 8 пикселей
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0xc0)>>3);
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0x30)>>1);
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0x0c)<<1);
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0x03)<<3);
			
			// p_scr_str_buf+=4;//test
			x+=8;
			draw_img_inx+=4;
			
		};
		zx_screen_refresh=false;
		//graphics_update_screen();
		//ack_input=true;
		//continue; 
	}//while(1)
};

/*
			if(tape_autoload_status==0x08){
				if((cpu.pc==0x0556)||(cpu.pc==0x056C)){ // START LOAD
					FastTapeLoad();
				}
				/if(	(cpu.pc==0x0556)||
					(cpu.pc==0x056C)||
					(cpu.pc==0x0574)||
					(cpu.pc==0x0580)||
					(cpu.pc==0x058F)||
					(cpu.pc==0x05A9)||
					(cpu.pc==0x05B3)||
					(cpu.pc==0x05E3)||
					(cpu.pc==0x05E7)||
					(cpu.pc==0x05E9)||
					(cpu.pc==0x05ED)){ // START LOAD
					printf("Fast Hook:%04X\n",cpu.pc);
				}/
				if(cpu.pc==0x05E2){ // END LOAD / SAVE
					RomLoading=false;
				}				
			}
*/

// void zx_machine_set_vbuf(uint8_t* vbuf){active_screen_buf=vbuf;};
void zx_machine_enable_vbuf(bool en_vbuf){
	vbuf_en=en_vbuf;
};

bool zx_machine_get_vbuf_en(){
	return vbuf_en;
};

void zx_machine_set_pc(uint16_t pc){
	im_z80_stop=true;
	im_z80_stop = true;
	while (im_z80_stop){
		busy_wait_ms(10);
		if (im_ready_loading){
			cpu.pc=pc;
			im_z80_stop = false;
			im_ready_loading = false;
		}
	}
};

/*void zx_machine_slow_FR(bool fr_slow){
	slow_fr=fr_slow;
	fr_changed=true;
};*/


/*uint8_t zx_machine_tape_active(){
	if(tap_loader_active){
	printf("GET autostart\n");
	return tape_active;
	}
	return 0;
};*/

/*
	void zx_machine_set_tape_pos(uint16_t pos){
	tape_position = pos;
	};
*/
