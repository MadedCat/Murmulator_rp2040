#pragma once

#define MAX_OPEN_FILES  2
#define MAX_OPEN_DIRS 1

#include "globals.h"
#include "../lib/sdcard/sdcard.h"
#include "../lib/fatfs/f_util.h"
#include "../lib/fatfs/ff.h"


typedef struct FileRec{
	char filename[(FILE_NAME_LEN-1)];
	uint8_t attr;
} __attribute__((packed)) FileRec;

extern int file_descr; // индикатор ошибки чтения файла
extern FATFS fs;
extern FIL sd_file;
extern DIR sd_dir;
extern FILINFO sd_file_info;
extern FRESULT sd_res;
extern size_t sd_f_size;
 
extern char dirs[DIRS_DEPTH+5][FILE_NAME_LEN];
extern char files[MAX_FILES+5][FILE_NAME_LEN];
extern char dir_path[(DIRS_DEPTH+5)*FILE_NAME_LEN];
extern char activefilename[400];
extern char afilename[FILE_NAME_LEN+1];
//extern uint8_t sd_buffer[SD_BUFFER_SIZE]; //буфер для работы с файлами
extern char filename[260];

#define TOP_DIR_ATTR 0x40
#define SELECTED_FILE_ATTR 0x80


bool init_filesystem(void);
const char* get_file_extension(const char *filename);
int get_files_from_dir(char *dir_name,char* nf_buf, int MAX_N_FILES);
void get_path(int dir_index);
int read_select_dir(int dir_index);

char* get_lfn_from_dir(char *dir_name,FileRec* short_name);

int sd_open_file(FIL *file, char* file_name, BYTE mode);
int sd_read_file(FIL *file, void* buffer, unsigned int bytestoread, unsigned int* bytesreaded);
int sd_write_file(FIL *file, const void* buffer, UINT bytestowrite, UINT* byteswrited);
int sd_flush_file(FIL *file);
int sd_seek_file(FIL *file, FSIZE_t offset);
int sd_close_file(FIL *file);
DWORD sd_file_pos(FIL *file);
DWORD sd_file_size(FIL *file);
int sd_mkdir(const TCHAR* path);
int sd_rename(const TCHAR* path_old, const TCHAR* path_new);
int sd_opendir(DIR* dp,const TCHAR* path);
int sd_readdir(DIR* dp, FILINFO* fno);
int sd_closedir(DIR *dp);

int sd_delete(const TCHAR* path);
int sd_delete_node(TCHAR* path, UINT sz_buff, FILINFO* fno);