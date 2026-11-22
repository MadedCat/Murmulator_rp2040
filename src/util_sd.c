#include <stdio.h>
#include "string.h"
#include <fcntl.h>
#include "zx_emu/z80.h"
#include "util_sd.h"
#include "utf_handle.h"
#include "globals.h"


int file_descr; // индикатор ошибки чтения файла
static FATFS file_sys;
FIL sd_file;
DIR sd_dir;
FILINFO sd_file_info;
FRESULT sd_res;
size_t sd_f_size;

char dirs[DIRS_DEPTH+5][FILE_NAME_LEN];
char files[MAX_FILES+5][FILE_NAME_LEN];
char dir_path[(DIRS_DEPTH+5)*FILE_NAME_LEN];
char activefilename[400];
char afilename[FILE_NAME_LEN+1];
extern uint8_t sd_buffer[SD_BUFFER_SIZE]; //буфер для работы с файлами
char filename[260];

//extern uint8_t zx_color[];

//#define USE_FATFS

//file_descr =-1;
int last_error=0;

// #define file_name "0:/z80/f3.z80" // Имя файла образа на SD

//#define DIRS_DEPTH (10)
//#define MAX_FILES  (500)
//#define SD_BUFFER_SIZE 0x4000  //16к буфер для работы с файлами

//uint8_t dirs[DIRS_DEPTH][FILE_NAME_LEN];
//uint8_t dir_patch[DIRS_DEPTH*FILE_NAME_LEN];
//char activefilename[160];
//char sd_buffer[SD_BUFFER_SIZE]; //16к буфер для работы с файлами

//работа с каталогами и файлами

bool init_filesystem(void){
	file_descr =-1;
	strcpy(dirs[0],"0:");
	strcpy(dirs[1],"z80");
	strcpy(dirs[2],"ra");
	strcpy(files[0],"..");
	files[0][(FILE_NAME_LEN-1)]|=AM_DIR;
	files[0][(FILE_NAME_LEN-1)]|=TOP_DIR_ATTR;
	//read_select_dir(1);
	strcpy(activefilename,"");
	file_descr=f_mount(&file_sys, dirs[0], 1);
	if (file_descr!=FR_OK) return false;
	file_descr=f_opendir(&sd_dir,dirs[0]);
	if (file_descr!=FR_OK) return false;
	printf("Open SD-OK\n");
	return true;
}

const char* get_file_extension(const char *filename){
    //printf("fn>%s\n",filename);
	char *dot = strrchr(filename, '.');
    if((dot==0) || (dot == filename)) return ""; // || (strlen(dot)>FILE_NAME_LEN)
    //printf("fn>%s>%08lX\n",filename,dot);
	return dot + 1;
}

void sortfiles(char* nf_buf, int N_FILES){

	int inx=0;
	if (N_FILES==0) return;
	uint8_t tmp_buf[FILE_NAME_LEN];

	while(inx!=(N_FILES-1)){
		FileRec* file1 = (FileRec*)&nf_buf[sizeof(FileRec)*inx];
		FileRec* file2 = (FileRec*)&nf_buf[sizeof(FileRec)*(inx+1)];
		if ((file1->attr&AM_DIR)>(file2->attr&AM_DIR)){inx++; continue;}
		if (((file1->attr&AM_DIR)<(file2->attr&AM_DIR))||(strcmp(file1->filename,file2->filename)>0)){
			memcpy(tmp_buf,nf_buf+sizeof(FileRec)*inx,sizeof(FileRec));
			memcpy(nf_buf+sizeof(FileRec)*inx,nf_buf+sizeof(FileRec)*(inx+1),sizeof(FileRec));
			memcpy(nf_buf+sizeof(FileRec)*(inx+1),tmp_buf,sizeof(FileRec));
			if (inx) inx--;
			continue;
		}
		inx++;
	}
}

/*
void sortfiles(char* nf_buf, int N_FILES){
	int inx=0;
	if (N_FILES==0) return;
	uint8_t tmp_buf[FILE_NAME_LEN];
	while(inx!=(N_FILES-1)){
		if (nf_buf[FILE_NAME_LEN*inx+(FILE_NAME_LEN-1)]>nf_buf[FILE_NAME_LEN*(inx+1)+(FILE_NAME_LEN-1)]){inx++; continue;}

		if ((nf_buf[FILE_NAME_LEN*inx+(FILE_NAME_LEN-1)]<nf_buf[FILE_NAME_LEN*(inx+1)+(FILE_NAME_LEN-1)])||(strcmp(nf_buf+FILE_NAME_LEN*inx,nf_buf+FILE_NAME_LEN*(inx+1))>0)){
			memcpy(tmp_buf,nf_buf+FILE_NAME_LEN*inx,FILE_NAME_LEN);
			memcpy(nf_buf+FILE_NAME_LEN*inx,nf_buf+FILE_NAME_LEN*(inx+1),FILE_NAME_LEN);
			memcpy(nf_buf+FILE_NAME_LEN*(inx+1),tmp_buf,FILE_NAME_LEN);
			if (inx) inx--;
			continue;
		}
		inx++;
	}
}
*/

int get_files_from_dir(char *dir_name,char* nf_buf, int MAX_N_FILES){

	char temp[FILE_NAME_LEN+1];
	file_descr =-1;
	//"0:/z80"
	//file_descr = f_opendir(&sd_dir,dir_name);
	file_descr=f_opendir(&sd_dir,dir_name);
	if (file_descr!=FR_OK){
		printf("Error:%d\n",file_descr);
		return 0;
	}

	//printf("Get dir: %s\n", dir_name);
	int inx=0;
	while (1){   
		file_descr=f_readdir(&sd_dir,&sd_file_info);
		if (file_descr!=FR_OK){
			printf("Error:%d\n",file_descr);
			return 0;
		}
		if (strlen(sd_file_info.fname)==0){
			//printf("Files read:%d\n",inx);
			break;
		} 
		FileRec* file = (FileRec*)&nf_buf[sizeof(FileRec)*inx];

		file->attr=sd_file_info.fattrib;

		if(strlen((char *)sd_file_info.fname)>(FILE_NAME_LEN-1)){
			strncpy(temp,(char *)sd_file_info.altname,(FILE_NAME_LEN-1)) ;
		} else {
		    strncpy(temp,(char *)sd_file_info.fname,(FILE_NAME_LEN-1)) ;
		}
		for(uint8_t ch=0;ch<(FILE_NAME_LEN-1);ch++){
			if(file->attr&AM_DIR){
				//temp[ch]=toupper(temp[ch]);
				temp[ch]=cp866_upper_char(temp[ch]);
			} else {
				//temp[ch]=tolower(temp[ch]);
				temp[ch]=cp866_lower_char(temp[ch]);
			}
		}
		strncpy(file->filename,temp,(FILE_NAME_LEN-1));
		//printf("FN:%s\n",file->filename);
		inx++; 
		if (inx>=MAX_N_FILES) break;
	}
	f_closedir(&sd_dir);
	sortfiles(nf_buf,inx);
	return inx;
}
/*

		if(!((sd_file_info.fattrib&AM_HID)||(sd_file_info.fattrib&AM_SYS))){
			memset(temp,0,FILE_NAME_LEN);
			if(strlen(sd_file_info.fname)>(FILE_NAME_LEN-1)){
				strncpy(temp,sd_file_info.altname,(FILE_NAME_LEN-1));
			} else {
				strncpy(temp,sd_file_info.fname,(FILE_NAME_LEN-1)) ;
			}
			temp[FILE_NAME_LEN-1]=sd_file_info.fattrib;
			//if (&AM_DIR) temp[FILE_NAME_LEN]=AM_DIR; else temp[FILE_NAME_LEN-1]=0;
			for(uint8_t ch=0;ch<(FILE_NAME_LEN-1);ch++){
				if(sd_file_info.fattrib&AM_DIR){
					temp[ch]=cp866_upper_char(temp[ch]);
				} else {
					temp[ch]=cp866_lower_char(temp[ch]);
				}
			}
			strncpy(nf_buf+FILE_NAME_LEN*inx,temp,FILE_NAME_LEN);
			inx++; 
		}


int get_files_from_dir(char *dir_name,char* nf_buf, int MAX_N_FILES){
	file_descr =-1;
	//"0:/z80"
	//file_descr = f_opendir(&sd_dir,dir_name);
	file_descr=f_opendir(&sd_dir,dir_name);
	if (file_descr!=FR_OK){return 0;}

	memset(nf_buf,0,(MAX_FILES*FILE_NAME_LEN));
	//printf("\nFile file_descr: %d\n", file_descr);
	int inx=0;
	while (1){   
		file_descr=f_readdir(&sd_dir,&sd_file_info);
		if (file_descr!=FR_OK){return 0;}
		if (strlen(sd_file_info.fname)==0) break;
		if(strlen(sd_file_info.fname)>(FILE_NAME_LEN-1)){
			strncpy(nf_buf+FILE_NAME_LEN*inx,sd_file_info.altname,(FILE_NAME_LEN-1)) ;	
		} else {
			strncpy(nf_buf+FILE_NAME_LEN*inx,sd_file_info.fname,(FILE_NAME_LEN-1)) ;
		}
		nf_buf[FILE_NAME_LEN*inx+(FILE_NAME_LEN-1)]=sd_file_info.fattrib; // else nf_buf[FILE_NAME_LEN*inx+(FILE_NAME_LEN-1)]&=~AM_DIR;
		inx++; 
		if (inx>=MAX_N_FILES) break;
		//if (sd_file_info.fname) break;
		//file_list[i] = sd_file_info.fname;
		//printf("[%s] %d\n",sd_file_info.fname,strlen(sd_file_info.fname));
	}
	f_closedir(&sd_dir);
	sortfiles(nf_buf,inx);
	return inx;
}
*/

char* get_file_from_dir(char *dir_name,int inx_file){
	file_descr =-1;
	memset(filename, 0, sizeof(filename));
	file_descr = f_opendir(&sd_dir,dir_name);
	if (file_descr!=FR_OK) return filename;
	//printf("\nFile file_descr: %d\n", file_descr);
	int inx=0;
	while (1){   
		file_descr = f_readdir(&sd_dir,&sd_file_info);
		if (file_descr!=FR_OK) return filename;
		if (strlen(sd_file_info.fname)==0) break;
		if (inx++==inx_file){
			//strncpy(filename,sd_file_info.fname,FILE_NAME_LEN) ;
			strncpy(filename,sd_file_info.fname,sizeof(sd_file_info.fname)) ;
			if (sd_file_info.fattrib == AM_DIR) strcat(filename,"*");
			break;
		}
		//if (sd_file_info.fname) break;
		//file_list[i] = sd_file_info.fname;
	}
	f_closedir(&sd_dir);
	//printf("[%s] %d\n",filename,strlen(filename));
	return filename;
}

char* get_lfn_from_dir(char *dir_name,FileRec* short_name){

    file_descr=-1;
    file_descr = sd_opendir(&sd_dir,dir_name);
    if (file_descr!=FR_OK) return filename;
    //printf("\nFile file_descr: %d\n", file_descr);
    int inx=0;
    while (1){   
        file_descr = f_readdir(&sd_dir,&sd_file_info);
        if (file_descr!=FR_OK) return filename;
        if (strlen((char *)sd_file_info.fname)==0) break;
        memset(filename, 0, sizeof(filename));
        strncpy(filename,(char *)sd_file_info.altname,strlen((char *)sd_file_info.altname));
        /*if(short_name->attr&AM_DIR){
            printf("Dir>%s \n",short_name->filename);
        }*/
        for(uint8_t ch=0;ch<strlen((char *)sd_file_info.fname);ch++){
            if(short_name->attr&AM_DIR){
                filename[ch]=cp866_upper_char(filename[ch]);
            } else {
                filename[ch]=cp866_lower_char(filename[ch]);
            }
        }        
        if (strncmp(filename,short_name->filename,(FILE_NAME_LEN-1))==0){
            inx=0;
            break;
        }
        inx++;
    }
    if(inx==0){
        memset(filename, 0, sizeof(filename));
        strncpy(filename,(char *)sd_file_info.fname,strlen((char *)sd_file_info.fname));
    } else {
        memset(filename, 0, sizeof(filename));
        strncpy(filename,short_name->filename,(FILE_NAME_LEN-1));
    }
    f_closedir(&sd_dir);
    //printf(">[%s] %d\n",filename,strlen(filename));
    return filename;

}

void get_path(int dir_index){
	dir_path[0]=0;
	for(int i=0;i<=dir_index;i++){
	  if (dir_index>=DIRS_DEPTH) break;
	  strcat(dir_path,dirs[i]);
	  if (i!=dir_index) strcat(dir_path,"/");
	}
}

int read_select_dir(int dir_index){
	get_path(dir_index);
	//printf("open dir:%s  idx:%d\n",dir_path,dir_index);
	if(dir_index>0){
		memset(files,0,FILE_NAME_LEN*MAX_FILES);
		strcpy(files[0],"..");
		files[0][(FILE_NAME_LEN-1)]|=AM_DIR;
		files[0][(FILE_NAME_LEN-1)]|=TOP_DIR_ATTR;
	}	
	return get_files_from_dir(dir_path,files[1],MAX_FILES);
}

int sd_mkdir(const TCHAR* path){
	return f_mkdir(path);
}

int sd_open_file(FIL *file,char *file_name,BYTE mode){
	return f_open(file,file_name,mode);
}

int sd_read_file(FIL *file,void* buffer, UINT bytestoread, UINT* bytesreaded){
	return f_read(file,buffer,bytestoread,bytesreaded);
}

int sd_write_file(FIL *file, const void* buffer, UINT bytestowrite, UINT* byteswrited){
	return f_write(file,buffer,bytestowrite,byteswrited);
}

int sd_flush_file(FIL *file){
	return f_sync (file);
}

int sd_rename(const TCHAR* path_old, const TCHAR* path_new){
	return f_rename (path_old, path_new);
}


int sd_seek_file(FIL *file,FSIZE_t offset){
	return f_lseek(file,offset);
}

int sd_close_file(FIL *file){
	return f_close(file);
}

DWORD sd_file_pos(FIL *file){
	return file->fptr;
}

DWORD sd_file_size(FIL *file){
	return file->obj.objsize;
}

int sd_opendir(DIR* dp,const TCHAR* path){
	return f_opendir(dp,path);
}

int sd_readdir(DIR* dp, FILINFO* fno){
	return f_readdir(dp,fno);
}

int sd_closedir(DIR *dp){
	return f_closedir (dp);
}

int sd_delete(const TCHAR* path){
	return f_unlink (path);
}

int sd_delete_node(
    TCHAR* path,    /* Path name buffer with the sub-directory to delete */
    UINT sz_buff,   /* Size of path name buffer (items) */
    FILINFO* fno    /* Name read buffer */
){
    UINT i, j;
    FRESULT fr;
    DIR dir;

    fr = f_opendir(&dir, path); /* Open the sub-directory to make it empty */
    if (fr != FR_OK) return fr;

    for (i = 0; path[i]; i++) ; /* Get current path length */
    path[i++] = _T('/');

    for (;;) {
        fr = f_readdir(&dir, fno);  /* Get a directory item */
        if (fr != FR_OK || !fno->fname[0]) break;   /* End of directory? */
        j = 0;
        do {    /* Make a path name */
            if (i + j >= sz_buff) { /* Buffer over flow? */
                fr = 100; break;    /* Fails with 100 when buffer overflow */
            }
            path[i + j] = fno->fname[j];
        } while (fno->fname[j++]);
        if (fno->fattrib & AM_DIR) {    /* Item is a sub-directory */
            fr = delete_node(path, sz_buff, fno);
        } else {                        /* Item is a file */
            fr = f_unlink(path);
        }
        if (fr != FR_OK) break;
    }

    path[--i] = 0;  /* Restore the path name */
    f_closedir(&dir);

    if (fr == FR_OK) fr = f_unlink(path);  /* Delete the empty sub-directory */
    return fr;
}