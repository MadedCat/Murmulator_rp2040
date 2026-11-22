#pragma once
#include "inttypes.h"
#include "stdbool.h"
#include "strings.h"

typedef struct ConvLetter {
	char    win1251;
	int     unicode;
} Letter;

char cp866_upper_char(char cp866);
char cp866_lower_char(char cp866);
int conv_utf_w1251(char *utf8, char* windows1251, size_t n);
int conv_utf_cp866(const char *utf8, char* cp866, size_t n);
int cp866_win1251(char *cp866, char* windows1251, size_t n);
int ucs16_win1251(char *ucs, char* windows1251, size_t n);
