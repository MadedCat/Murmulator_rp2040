#include "pico.h"
#include "inttypes.h"
#include "stdbool.h"
#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "util_sd.h"
#include "string.h"
#include "util_cfg.h"

extern uint8_t sd_buffer[SD_BUFFER_SIZE];

uint8_t cfg_boot_scr;
uint8_t cfg_hud_enable;
uint8_t cfg_tap_load_mode;
uint8_t cfg_tape_load_pin;
uint8_t cfg_def_joy1_mode;
uint8_t cfg_def_joy2_mode;
uint8_t cfg_def_kbd_mode;
uint8_t cfg_res_before_mode;
uint8_t cfg_sound_out_mode;
uint8_t cfg_sound_mode;
short int cfg_volume;
uint8_t cfg_tspin_mode;
uint8_t cfg_tsspeed_mode;
uint8_t cfg_tschip_order;
uint8_t cfg_video_out;
uint8_t cfg_frame_rate;
uint8_t cfg_lcd_video_out;
uint8_t cfg_rotate;
uint8_t cfg_inversion;
uint8_t cfg_pixels;
uint8_t cfg_brightness;
uint8_t cfg_mobile_mode;

//bool cfg_auto_load_tap = false;
//bool cfg_fast_load_tap = false;
//uint8_t cfg_def_joy_mode = 0;

const char __in_flash()*default_config_file[]={
";version:"FW_VERSION" \r\n\0",
"; [Boot logo display]\r\n\0",
"; 0 - SHOW LOGO \r\n\0",
"; 1 - SHOW FILE MANAGER \r\n\0",
"; 2 - EMULATION \r\n\0",
"boot_scr:0\r\n\0",
"; [Head Up Display]\r\n\0",
"; 0 - OFF \r\n\0",
"; 1 - On \r\n\0",
"; 2 - Time \r\n\0",
"hud_enable:1\r\n\0",
"; [Tape .tap file loading mode]\r\n\0",
"; 0 - Standard tape emulation play via pause between blocks\r\n\0",
"; 1 - Autorun of the tape via interception of the ROM call\r\n\0",
"; 2 - Fast tape loading via ROM call interception\r\n\0",
"tap_load_mode:0\r\n\0",
"; [External tape loading pin assignment]\r\n\0",
"; 0 - GPIO23 used for loader\r\n\0",
"; 1 - GPIO29 used for loader\r\n\0",
"; 2 - External input off\r\n\0",
"tape_load_pin:0\r\n\0",
"; [Joystick 1/2 mode]\r\n\0",
"; 0 - External joystick maps to Kempston joystick\r\n\0",
"; 1 - External joystick maps to Cursor joystick\r\n\0",
"; 2 - External joystick maps to Sinclair 1 joystick\r\n\0",
"; 3 - External joystick maps to Sinclair 2 joystick\r\n\0",
"; 4 - External joystick maps to QAOPM keys\r\n\0",
"def_joy1_mode:0\r\n\0",
"def_joy2_mode:0\r\n\0",
"; [Keyboard mode]\r\n\0",
"; 0 - Keyboard keys maps to Cursor joystick\r\n\0",
"; 1 - Keyboard keys maps to Kempston joystick\r\n\0",
"; 2 - Keyboard keys maps to Sinclair 1 joystick\r\n\0",
"; 3 - Keyboard keys maps to Sinclair 2 joystick\r\n\0",
"; 4 - Keyboard keys maps to QAOPM keys\r\n\0",
"def_kbd_mode:0\r\n\0",
"; [Reboot ZX before load images mode]\r\n\0",
"; 0 - No restart emulation before load data\r\n\0",
"; 1 - Restart emulation before load data\r\n\0",
"res_before_mode:0\r\n\0",
"; [Sound OUT]\r\n\0",
"; 0 - PWM\r\n\0",
"; 1 - PCM\r\n\0",
"sound_out:0\r\n\0",
"; [Sound mode]\r\n\0",
"; 0 - -=[Sound OFF]=-\r\n\0",
"; 1 -   Only Beeper  \r\n\0",
"; 2 -  Beeper+SoftAY \r\n\0",
"; 3 -  Beeper+SoftTS \r\n\0",
"; 4 -   Beeper + TS  \r\n\0",
"sound_mode:3\r\n\0",
"; [Sound Volume level ]\r\n\0",
"; 0-256 - 0-low level, 256 - high level  \r\n\0",
"volume:104\r\n\0",
"; [Ext TS Clock Pin]\r\n\0",
"; 0 - Off \r\n\0",
"; 1 - GPIO21 used for out External 1.75Mhz clock \r\n\0",
"; 2 - GPIO29 used for out External 1.75Mhz clock \r\n\0",
"tspin_mode:0\r\n\0",
"; [TS Chip order]\r\n\0",
"; 0 - 0xFF > 0 chip, 0xFE > 1 chip \r\n\0",
"; 1 - 0xFF > 1 chip, 0xFE > 0 chip \r\n\0",
"tschip_order:0\r\n\0",
"; [Video output mode]\r\n\0",
"; 0 - AUTO \r\n\0",
"; 1 - VGA \r\n\0",
"; 2 - HDMI \r\n\0",
"; 3 - ST7789 \r\n\0",
"; 4 - ST7789V \r\n\0",
"; 5 - ILI9341 \r\n\0",
"; 6 - ILI9341V \r\n\0",
"; 7 - GC9A01 \r\n\0",
"video_out:0\r\n\0",
"; [Video Framerate mode]\r\n\0",
"; 0 - 60Hz \r\n\0",
"; 1 - 72Hz \r\n\0",
"; 2 - 75Hz \r\n\0",
"; 3 - 85Hz \r\n\0",
"frame_rate:0\r\n\0",
"; [Mobile Murmulator Mode]\r\n\0",
"; 0 - OFF, 1 - ON \r\n\0",
"mobile_mode:0\r\n\0",
"; [LCD Video output mode for auto mode]\r\n\0",
"; 3 - ST7789 \r\n\0",
"; 4 - ST7789V \r\n\0",
"; 5 - ILI9341 \r\n\0",
"; 6 - ILI9341V \r\n\0",
"; 7 - GC9A01 \r\n\0",
"tft_video_out:0\r\n\0",
"; [TFT Brightness level ]\r\n\0",
"; 0-10 - 0-low level, 10 - high level  \r\n\0",
"tft_bright:5\r\n\0",
"; [TFT orientation]\r\n\0",
"; 0 - Horizont Left \r\n\0",
"; 1 - Vertical Left \r\n\0",
"; 2 - Horizont Right \r\n\0",
"; 3 - Vertical Right \r\n\0",
"tft_rotate:0\r\n\0",
"; [TFT inversion]\r\n\0",
"; 0 - Normal image\r\n\0",
"; 1 - Invert image \r\n\0",
"tft_inversion:0\r\n\0",
"; [TFT pixels format]\r\n\0",
"; 0 - BGR \r\n\0",
"; 1 - RGB \r\n\0",
"tft_pixels:0\r\n\0",
";[EOF]\r\n\0",
"\0",
};

const uint8_t load_pins[2] = {22,29};

const char cfg_version[24] = ";version:"FW_VERSION"\0";

char line[150];
char param[50];


void config_defaults() {
	cfg_boot_scr = DEF_CFG_BOOT_SCR_MODE;
	cfg_hud_enable = DEF_CFG_HUD_MODE;
	cfg_tap_load_mode = DEF_CFG_LOAD_TAP;
	cfg_tape_load_pin=DEF_CFG_TAPELOAD_PIN;
	cfg_def_joy1_mode = DEF_CFG_JOY_MODE;
	cfg_def_joy2_mode = DEF_CFG_JOY_MODE;
	cfg_def_kbd_mode = DEF_CFG_KBD_MODE;
	cfg_res_before_mode = DEF_CFG_RES_MODE;
	cfg_sound_out_mode = DEF_CFG_OUT_MODE;
	cfg_sound_mode = DEF_CFG_SND_MODE;
	cfg_volume = DEF_CFG_VOLUME_MODE;
	cfg_tspin_mode = DEF_CFG_TSPIN_MODE;
	cfg_tschip_order = DEF_CFG_TSORDER_MODE;
	cfg_tsspeed_mode = DEF_CFG_TSSPEED_MODE;
	cfg_video_out = DEF_CFG_VIDEO_MODE;
	cfg_frame_rate = DEF_CFG_VIDEO_FREQ_MODE;
	cfg_lcd_video_out = DEF_CFG_LCD_VIDEO_MODE;
	cfg_brightness = DEF_CFG_BRIGHT_MODE;
	cfg_rotate = DEF_CFG_ROTATE;
	cfg_inversion = DEF_CFG_INVERSION;
	cfg_pixels = DEF_CFG_PIXELS;
	cfg_mobile_mode = DEF_CFG_MOBILE_MODE;
}


bool config_read() {
	uint8_t chr;
	uint8_t lineptr=0;

	size_t bytesRead=0;
	UINT bytesToRead=0;
	int fd=0;

	memset(sd_buffer, 0, sizeof(sd_buffer));
	memset(activefilename, 0, sizeof(activefilename));
	memset(line, 0, sizeof(line));
	memset(param, 0, sizeof(param));

	strcpy(activefilename,"0:");
	strcat(activefilename,CONFIG_FILENAME);
	
	fd = sd_open_file(&sd_file,activefilename,FA_READ);
	if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
	
	
	fd = sd_read_file(&sd_file,&param,strlen(cfg_version),&bytesRead);
	if (fd!=FR_OK){sd_close_file(&sd_file);return false;}	

	if(strncmp(param,cfg_version,strlen(cfg_version))!=0){
		//printf("Old Version - Write Defaults\n");
		config_write_defaults();		
	}
	//printf("File Version:%s  %d\n",param,strncmp(param,cfg_version,14));
	
	////printf("Begin read\n");
	// Boot config file
	printf("Loading config file '%s':\n", CONFIG_FILENAME);

	bytesToRead = sd_file_size(&sd_file);
	//printf("CFG Size:%d\n",bytesToRead);
	if (bytesToRead==0){sd_close_file(&sd_file);return false;}
	if (bytesToRead<sizeof(sd_buffer)){
		fd = sd_read_file(&sd_file,&sd_buffer,bytesToRead,&bytesRead);
		if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
		////printf("Readed:%d\n",bytesRead);
	}
   
	/*//printf("------\n");
	uint16_t ptr=0;
	uint8_t* memory = sd_buffer;
	do{
		//printf("Page[%04X]",ptr);
		for (uint8_t col=0;col<16;col++){
			//printf("	 %02X",memory[ptr]);
			ptr++;
		}
		//printf("\n");
	} while(ptr<bytesRead);
	//printf("\n");  */

	uint16_t ptr=0;
	uint8_t* memory = sd_buffer;
	do{
		//if (ptr%16==0){//printf("\n"); //printf("[%04X]",ptr);};
		chr = memory[ptr];
		////printf("	 %02X",chr);
		if (chr == 0x3B) { // ';'
			////printf("\nskip comment line\n");
			while((chr != 0x0A)&&(ptr<bytesRead)) {
				chr = memory[ptr];
				ptr++;
			}
			continue;
		}
		if (chr == 0x3A) { // ':'
			strcpy(param,line);
			memset(line, 0, sizeof(line));
			lineptr=0;
			ptr++;
			continue;
		}

		if (chr == 0x0A) { // '\n'
			////printf("Readed %s>%s\n",param,line);

			if(strcasecmp(param, "boot_scr") == 0){
				if(strlen(line)>0){
					cfg_boot_scr = atoi(line);
				} else {
					cfg_boot_scr=DEF_CFG_BOOT_SCR_MODE;
				}
				//printf("  + cfg_boot_scr:%d\n", cfg_boot_scr);
			} else 
			if(strcasecmp(param, "hud_enable") == 0){
				if(strlen(line)>0){
					cfg_hud_enable = atoi(line);
				} else {
					cfg_hud_enable=DEF_CFG_HUD_MODE;
				}
				//printf("  + cfg_hud_enable:%d\n", cfg_hud_enable);
			} else 			
			if(strcasecmp(param, "tap_load_mode") == 0){
				if(strlen(line)>0){
					cfg_tap_load_mode = atoi(line);
				} else {
					cfg_tap_load_mode=DEF_CFG_LOAD_TAP;
				}
				//printf("  + cfg_tap_load_mode:%d\n", (cfg_tap_load_mode));
			} else 
			if(strcasecmp(param, "tape_load_pin") == 0){
				if(strlen(line)>0){
					cfg_tape_load_pin = atoi(line);
				} else {
					cfg_tape_load_pin=DEF_CFG_TAPELOAD_PIN;
				}
				//printf("  + cfg_tape_load_pin:%d\n", (cfg_tape_load_pin));
			} else 
			if(strcasecmp(param, "def_joy1_mode") == 0){
				if(strlen(line)>0){
					cfg_def_joy1_mode = atoi(line);
				} else {
					cfg_def_joy1_mode=DEF_CFG_JOY_MODE;
				}
				//printf("  + cfg_def_joy_mode:%d\n", cfg_def_joy_mode);
			} else 
			if(strcasecmp(param, "def_joy2_mode") == 0){
				if(strlen(line)>0){
					cfg_def_joy2_mode = atoi(line);
				} else {
					cfg_def_joy2_mode=DEF_CFG_JOY_MODE;
				}
				//printf("  + cfg_def_joy_mode:%d\n", cfg_def_joy_mode);
			} else 
			if(strcasecmp(param, "def_kbd_mode") == 0){
				if(strlen(line)>0){
					cfg_def_kbd_mode = atoi(line);
				} else {
					cfg_def_kbd_mode=DEF_CFG_KBD_MODE;
				}
				//printf("  + cfg_def_joy_mode:%d\n", cfg_def_joy_mode);
			} else 
			if(strcasecmp(param, "res_before_mode") == 0){
				if(strlen(line)>0){
					cfg_res_before_mode = atoi(line);
				} else {
					cfg_res_before_mode=DEF_CFG_RES_MODE;
				}
				//printf("  + cfg_res_before_mode:%d\n", cfg_res_before_mode);
			} else 
			if(strcasecmp(param, "sound_out") == 0){
				if(strlen(line)>0){
					cfg_sound_out_mode = atoi(line);
				} else {
					cfg_sound_out_mode=DEF_CFG_OUT_MODE;
				}
				//printf("  + cfg_sound_out_mode:%d\n", cfg_sound_out_mode);
			} else
			if(strcasecmp(param, "sound_mode") == 0){
				if(strlen(line)>0){
					cfg_sound_mode = atoi(line);
				} else {
					cfg_sound_mode=DEF_CFG_SND_MODE;
				}
				//printf("  + cfg_sound_mode:%d\n", cfg_sound_mode);
			} else 
			if(strcasecmp(param, "volume") == 0){
				if(strlen(line)>0){
					cfg_volume = atoi(line);
				} else {
					cfg_volume=DEF_CFG_VOLUME_MODE;
				}
				//printf("  + cfg_volume:%d\n", cfg_volume);
			} else
			if(strcasecmp(param, "tspin_mode") == 0){
				if(strlen(line)>0){
					cfg_tspin_mode = atoi(line);
				} else {
					cfg_tspin_mode=DEF_CFG_TSPIN_MODE;
				}
				//printf("  + cfg_tschip_order:%d\n", cfg_tschip_order);
			} else
			if(strcasecmp(param, "tschip_order") == 0){
				if(strlen(line)>0){
					cfg_tschip_order = atoi(line);
				} else {
					cfg_tschip_order=DEF_CFG_TSORDER_MODE;
				}
				//printf("  + cfg_tschip_order:%d\n", cfg_tschip_order);
			} else 
			if(strcasecmp(param, "video_out") == 0){
				if(strlen(line)>0){
					long video_out = strtol(line, NULL, 10);
					cfg_video_out = (video_out >= 0 && video_out <= MAX_CFG_VIDEO_MODE)
						? (uint8_t)video_out : DEF_CFG_VIDEO_MODE;
				} else {
					cfg_video_out=DEF_CFG_VIDEO_MODE;
				}
				//printf("  + cfg_video_out:%d\n", cfg_video_out);
			} else 
	
			if(strcasecmp(param, "frame_rate") == 0){
				if(strlen(line)>0){
					cfg_frame_rate = atoi(line);
				} else {
					cfg_frame_rate=DEF_CFG_VIDEO_FREQ_MODE;
				}
				//printf("  + cfg_frame_rate:%d\n", cfg_frame_rate);
			} else 
			if(strcasecmp(param, "mobile_mode") == 0){
				if(strlen(line)>0){
					cfg_mobile_mode = atoi(line);
				} else {
					cfg_mobile_mode=DEF_CFG_MOBILE_MODE;
				}
				//printf("  + cfg_mobile_mode:%d\n", cfg_mobile_mode);
			} else 
			if(strcasecmp(param, "tft_video_out") == 0){
				if(strlen(line)>0){
					cfg_lcd_video_out = atoi(line);
				} else {
					cfg_lcd_video_out=DEF_CFG_LCD_VIDEO_MODE;
				}
				//printf("  + cfg_lcd_video_out:%d\n", cfg_lcd_video_out);
			} else				
			if(strcasecmp(param, "tft_bright") == 0){
				if(strlen(line)>0){
					cfg_brightness = atoi(line);
				} else {
					cfg_brightness=DEF_CFG_BRIGHT_MODE;
				}
				//printf("  + cfg_brightness:%d\n", cfg_brightness);
			} else
			if(strcasecmp(param, "tft_rotate") == 0){
				if(strlen(line)>0){
					cfg_rotate = atoi(line);
				} else {
					cfg_rotate=DEF_CFG_ROTATE;
				}
				//printf("  + cfg_rotate:%d\n", cfg_rotate);
			} else 
			if(strcasecmp(param, "tft_inversion") == 0){
				if(strlen(line)>0){
					cfg_inversion = atoi(line);
				} else {
					cfg_inversion=DEF_CFG_INVERSION;
				}
				//printf("  + cfg_inversion:%d\n", cfg_inversion);
			} else 
			if(strcasecmp(param, "tft_pixels") == 0){
				if(strlen(line)>0){
					cfg_pixels = atoi(line);
				} else {
					cfg_pixels=DEF_CFG_PIXELS;
				}
				//printf("  + cfg_pixels:%d\n", cfg_pixels);
			}

			memset(line, 0, sizeof(line));
			lineptr=0;
		} else {
			////printf(" %02d",lineptr);
			if(lineptr<sizeof(line)){
				////printf(" %02X",chr);
				line[lineptr++]=chr;
			}
		}
		ptr++;
	} while(ptr<bytesRead);

	sd_close_file(&sd_file);
	printf("Config file loaded OK\n");
	memset(activefilename, 0, sizeof(activefilename));
	return true;
}

// Dump actual config to FS
bool config_save(){
	int fd=0;
	uint8_t chr;
	uint16_t lineptr=0;
	uint16_t ptr=0;
	size_t bytesRead;
	UINT bytesToRead;
	UINT bytesToWrite;
	UINT bytesWritten;

	memset(sd_buffer, 0, sizeof(sd_buffer));
	memset(activefilename, 0, sizeof(activefilename));
	memset(line, 0, sizeof(line));
	memset(param, 0, sizeof(param));

	printf("Saving config file '%s':\n", CONFIG_FILENAME);

	strcpy(activefilename,"0:");
	strcat(activefilename,CONFIG_FILENAME);
	strcpy(line,"0:");
	strcat(line,CONFIG_FILENAME);
	strcat(line,"_bak");

	fd = sd_delete(line);
	//printf("delete bak:%d\n",fd);
	if ((fd!=FR_NO_FILE)&&(fd!=FR_OK)){return false;}

	fd = sd_rename(activefilename, line);
	//printf("rename:%d\n",fd);
	if (fd!=FR_OK){return false;}

	fd = sd_open_file(&sd_file,line,FA_READ);
	//printf("open bak:%d\n",fd);
	if (fd!=FR_OK){sd_close_file(&sd_file);return false;}

	bytesToRead = sd_file_size(&sd_file);
	//printf("CFG Size:%d\n",bytesToRead);
	if (bytesToRead<sizeof(sd_buffer)){
		fd = sd_read_file(&sd_file,&sd_buffer,bytesToRead,&bytesRead);
		if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
		//printf("Readed:%d\n",bytesRead);
	}
	sd_close_file(&sd_file);

	////printf("cfg file name:%s\n",activefilename);
   	fd = sd_open_file(&sd_file,activefilename,FA_WRITE);
	if (fd==FR_NO_FILE){
		fd = sd_open_file(&sd_file,activefilename,FA_WRITE|FA_CREATE_NEW);
	}
	//printf(" file open:%d\n",fd);
	if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
	
	memset(line, 0, sizeof(line));


	uint8_t* memory = sd_buffer;
	do{
		//if (ptr%16==0){//printf("\n"); //printf("[%04X]",ptr);};
		chr = memory[ptr];
		////printf("	 %02X",chr);
		if (chr == 0x3B) { // ';'
			////printf("\ncopy comment line\n");
			while((chr != 0x0A)&&(ptr<bytesRead)) {
				chr = memory[ptr];
				if(lineptr<sizeof(line)){
					line[lineptr++]=chr;
				}
				ptr++;
			}
			bytesToWrite = strlen(line);
			fd = sd_write_file(&sd_file,line,bytesToWrite,&bytesWritten);
			if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			memset(line, 0, sizeof(line));
			memset(param, 0, sizeof(param));
			lineptr=0;			
			continue;
		}
		if (chr == 0x3A) { // ':'
			strcpy(param,line);
			memset(line, 0, sizeof(line));
			lineptr=0;
			ptr++;
			continue;
		}

		if (chr == 0x0A) { // '\n'
			////printf("Readed %s>%s\n",param,line);
			if(strcasecmp(param, "boot_scr") == 0){
				//printf("  + boot_scr:%d\n", cfg_boot_scr);
				sprintf(line,"boot_scr:%d\n",cfg_boot_scr);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "hud_enable") == 0){
				//printf("  + hud_enable:%d\n", cfg_hud_enable);
				sprintf(line,"hud_enable:%d\n",cfg_hud_enable);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "tap_load_mode") == 0){
				//printf("  + tap_load_mode:%d\n", cfg_tap_load_mode);
				sprintf(line,"tap_load_mode:%d\n",cfg_tap_load_mode);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "tape_load_pin") == 0){
				//printf("  + tape_load_pin:%d\n", cfg_tape_load_pin);
				sprintf(line,"tape_load_pin:%d\n",cfg_tape_load_pin);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "def_joy1_mode") == 0){
				//printf("  + def_joy_mode:%d\n", cfg_def_joy_mode);
				sprintf(line,"def_joy1_mode:%d\n",cfg_def_joy1_mode);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "def_joy2_mode") == 0){
				//printf("  + def_joy_mode:%d\n", cfg_def_joy_mode);
				sprintf(line,"def_joy2_mode:%d\n",cfg_def_joy2_mode);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "def_kbd_mode") == 0){
				//printf("  + def_joy_mode:%d\n", cfg_def_joy_mode);
				sprintf(line,"def_kbd_mode:%d\n",cfg_def_kbd_mode);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else
			if(strcasecmp(param, "res_before_mode") == 0){
				//printf("  + res_before_mode:%d\n", cfg_res_before_mode);
				sprintf(line,"res_before_mode:%d\n",cfg_res_before_mode);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else
			if(strcasecmp(param, "sound_out") == 0){
				//printf("  + sound_out:%d\n", cfg_sound_out_mode);
				sprintf(line,"sound_out:%d\n",cfg_sound_out_mode);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 			
			if(strcasecmp(param, "sound_mode") == 0){
				//printf("  + sound_mode:%d\n", cfg_sound_mode);
				sprintf(line,"sound_mode:%d\n",cfg_sound_mode);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "volume") == 0){
				//printf("  + volume:%d\n", cfg_volume);
				sprintf(line,"volume:%d\n",cfg_volume);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "tspin_mode") == 0){
				//printf("  + tspin_mode:%d\n", cfg_tspin_mode);
				sprintf(line,"tspin_mode:%d\n",cfg_tspin_mode);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 			
			if(strcasecmp(param, "tschip_order") == 0){
				//printf("  + tschip_order:%d\n", cfg_tschip_order);
				sprintf(line,"tschip_order:%d\n",cfg_tschip_order);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "video_out") == 0){
				//printf("  + video_out:%d\n", cfg_video_out);
				sprintf(line,"video_out:%d\n",cfg_video_out);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "frame_rate") == 0){
				//printf("  + frame_rate:%d\n", cfg_frame_rate);
				sprintf(line,"frame_rate:%d\n",cfg_frame_rate);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "mobile_mode") == 0){
				//printf("  + mobile_mode:%d\n", cfg_mobile_mode);
				sprintf(line,"mobile_mode:%d\n",cfg_mobile_mode);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "tft_video_out") == 0){
				//printf("  + tft_video_out:%d\n", cfg_lcd_video_out);
				sprintf(line,"tft_video_out:%d\n",cfg_lcd_video_out);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else			
			if(strcasecmp(param, "tft_bright") == 0){
				//printf("  + tft_bright:%d\n", cfg_brightness);
				sprintf(line,"tft_bright:%d\n",cfg_brightness);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else			
			if(strcasecmp(param, "tft_rotate") == 0){
				//printf("  + tft_rotate:%d\n", cfg_rotate);
				sprintf(line,"tft_rotate:%d\n",cfg_rotate);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "tft_inversion") == 0){
				//printf("  + tft_inversion:%d\n", cfg_inversion);
				sprintf(line,"tft_inversion:%d\n",cfg_inversion);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			} else 
			if(strcasecmp(param, "tft_pixels") == 0){
				//printf("  + tft_pixels:%d\n", cfg_pixels);
				sprintf(line,"tft_pixels:%d\n",cfg_pixels);
				bytesToWrite = strlen(line);
				fd = sd_write_file(&sd_file,line, bytesToWrite,&bytesWritten);
				if (fd!=FR_OK){sd_close_file(&sd_file);return false;}
			}

			memset(line, 0, sizeof(line));
			memset(param, 0, sizeof(param));
			lineptr=0;
		} else {
			////printf(" %02d",lineptr);
			if(lineptr<sizeof(line)){
				////printf(" %02X",chr);
				line[lineptr++]=chr;
			}
		}
		ptr++;
	} while(ptr<bytesRead);

	memset(sd_buffer, 0, sizeof(sd_buffer));
	memset(activefilename, 0, sizeof(activefilename));
	memset(line, 0, sizeof(line));
	memset(param, 0, sizeof(param));

	sd_flush_file(&sd_file);
	sd_close_file(&sd_file);

	printf("Config saved OK\n");
	return true;
} 

bool config_write_defaults(){
	int fd=0;
	size_t bytesRead;
	UINT bytesToRead;
	UINT bytesToWrite;
	UINT bytesWritten;

	memset(activefilename, 0, sizeof(activefilename));
	memset(line, 0, sizeof(line));

	config_defaults();

	printf("Saving DEFAULT config file '%s':\n", CONFIG_FILENAME);

	strcpy(activefilename,"0:");
	strcat(activefilename,CONFIG_FILENAME);
	strcpy(line,"0:");
	strcat(line,CONFIG_FILENAME);
	strcat(line,"_bak");

	fd = sd_delete(line);
	////printf("delete bak:%d  %s\n",fd,line);
	if ((fd!=FR_NO_FILE)&&(fd!=FR_OK)){return false;}

	fd = sd_rename(activefilename, line);
	////printf("rename:%d %s>%s\n",fd,activefilename, line);
	if ((fd!=FR_NO_FILE)&&(fd!=FR_OK)){return false;}

   	fd = sd_open_file(&sd_file,activefilename,FA_WRITE);
	////printf(" file open:%d %s\n",fd,activefilename);
	if (fd==FR_NO_FILE){
		fd = sd_open_file(&sd_file,activefilename,FA_WRITE|FA_CREATE_NEW);
		////printf(" file create:%d %s\n",fd,activefilename);
	}
	if (fd!=FR_OK){sd_close_file(&sd_file);return false;}

	uint16_t ptr =0;

	while(strlen(default_config_file[ptr])>0) { //while(*default_config_file[ptr]>0) {
		bytesToWrite = strlen(default_config_file[ptr]);
		////printf("BTW:%d > %s\n",bytesToWrite,default_config_file[ptr]);
		fd = sd_write_file(&sd_file,default_config_file[ptr],bytesToWrite,&bytesWritten);
		//printf(" file write:%d %s\n",fd,default_config_file[ptr]);
		if (fd!=FR_OK){
			//printf("ERROR file close:%d %s\n",fd,activefilename);
			sd_flush_file(&sd_file);
			sd_close_file(&sd_file);
			return false;
		}
		////printf("Next line\n");
		ptr++;
	}
	////printf("file close\n");
	sd_flush_file(&sd_file);
	sd_close_file(&sd_file);
	printf("Config saved OK\n");
	return true;
}

