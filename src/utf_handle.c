#include "inttypes.h"
#include "stdbool.h"
#include "strings.h"
#include <ctype.h>
#include "utf_handle.h"

char cp866_upper_char(char cp866){
	//(0x80)-(0x9F) - А-Я
	//(0xA0)-(0xAF) - а-п
	//(0xE0)-(0xEF) - р-я
	if((cp866>=0x80)&&(cp866<=0x9F)){
		return cp866;
	}
	if((cp866>=0xA0)&&(cp866<=0xAF)){
		return cp866-0x20;
	}
	if((cp866>=0xE0)&&(cp866<=0xEF)){
		return cp866-0x50;
	}
	if((cp866>=0x20)&&(cp866<0x80)) return toupper(cp866);
	//if(cp866<0x20) return 0x20;
	return 0;
}

char cp866_lower_char(char cp866){
	//(0x80)-(0x9F) - А-Я
	//(0xA0)-(0xAF) - а-п
	//(0xE0)-(0xEF) - р-я
	if((cp866>=0x80)&&(cp866<=0x8F)){
		return cp866+0x20;
	}
	if((cp866>=0x90)&&(cp866<=0x9F)){
		return cp866+0x50;
	}
	if((cp866>=0xA0)&&(cp866<=0xAF)){
		return cp866;
	}
	if((cp866>=0xE0)&&(cp866<=0xEF)){
		return cp866;
	}

	if((cp866>=0x20)&&(cp866<0x80)) return tolower(cp866);
	//if(cp866<0x20) return 0x20;
	return 0;
}

int conv_utf_w1251(char *utf8, char* windows1251, size_t n){
	int i = 0;
	int j = 0;
	for(; i < (int)n && utf8[i] != 0; ++i){
		char prefix = utf8[i];
		char suffix = utf8[i+1];
		if ((prefix & 0x80) == 0){
			windows1251[j] = (char)prefix;
			++j;
		} else if ((~prefix) & 0x20){
			int first5bit = prefix & 0x1F;
			first5bit <<= 6;
			int sec6bit = suffix & 0x3F;
			int unicode_char = first5bit + sec6bit;
			if ( unicode_char >= 0x410 && unicode_char <= 0x44F ) {
				windows1251[j] = (char)(unicode_char - 0x350);
			} else if (unicode_char >= 0x80 && unicode_char <= 0xFF) {
				windows1251[j] = (char)(unicode_char);
			} else if (unicode_char >= 0x402 && unicode_char <= 0x403) {
				windows1251[j] = (char)(unicode_char - 0x382);
			} else {
				/*int count = sizeof(g_letters) / sizeof(Letter);
				for (int k = 0; k < count; ++k) {
					if (unicode_char == g_letters[k].unicode) {
						windows1251[j] = g_letters[k].win1251;
						goto NEXT_LETTER;
					}
				}*/
				// can't convert this char
				return 0;
			}
            //NEXT_LETTER:
			++i;++j;
		} else {
			// can't convert this chars
			return 0;
		}
	}
	windows1251[j] = 0;
	return 1;
}

int conv_utf_cp866(const char *utf8, char* cp866, size_t n){
	int i = 0;
	int j = 0;
	for(; i < (int)n && utf8[i] != 0; ++i){
		char prefix = utf8[i];
		char suffix = utf8[i+1];
		if ((prefix & 0x80) == 0){
			cp866[j] = (char)prefix;
			++j;
		} else if ((~prefix) & 0x20){
			int first5bit = prefix & 0x1F;
			first5bit <<= 6;
			int sec6bit = suffix & 0x3F;
			int unicode_char = first5bit + sec6bit;
			if ( unicode_char >= 0x410 && unicode_char <= 0x42F ) { //А-Я
				cp866[j] = (char)(unicode_char - 0x0390);
			} else
			if (unicode_char >= 0x0430 && unicode_char <= 0x043F) { //а-п
				cp866[j] = (char)(unicode_char - 0x0390);
			} else
			if (unicode_char >= 0x0440 && unicode_char <= 0x044F){ //р-я
				cp866[j] = (char)(unicode_char - 0x0360);
			} else {
				return 0;
			}
			++i;++j;
		} else {
			return 0;
		}
	}
	cp866[j] = 0;
	return 1;
}

int cp866_win1251(char *cp866, char* windows1251, size_t n){
	//(0x80)-(0xAF)+0x40
	//(0xE0)-(0xEF)+0x10
	int i = 0;
	for(; i < (int)n && cp866[i] != 0; ++i){
		char pref = cp866[i];
		if((pref>=0x80)&&(pref<=0x9F)){
			windows1251[i] = pref+0x40;
		}
		if((pref>=0xA0)&&(pref<=0xAF)){
			windows1251[i] = pref+0x40;
		}
		if((pref>=0xE0)&&(pref<=0xEF)){
			windows1251[i] = pref+0x10;
		}
		if((pref>=0x20)&&(pref<0x80)){
			windows1251[i] = pref;
		}
		//if((pref>0)&&(pref<0x20)){
		//windows1251[i] = pref;
		//}

	}
	//windows1251[i] = 0;
	return i;
}

int ucs16_win1251(char *ucs, char* windows1251, size_t n){
	int i = 0;
	int j = 0;
	for(; j < (int)n && ucs[i] != 0; ++i){
		char prefix = ucs[i];
		char suffix = ucs[i+1];
		if ((prefix == 0x84)&&((suffix>=0x40)&&(suffix<=0x45))){ //C0
			windows1251[j] = (char)suffix+0x80;
			++j;
			++i;
			continue;
		} else 
		if ((prefix == 0x84)&&(suffix==0x46)){ //A8
			windows1251[j] = (char)suffix+0x62;
			++j;
			++i;
			continue;
		} else 
		if ((prefix == 0x84)&&((suffix>=0x47)&&(suffix<=0x60))){ //C6
			windows1251[j] = (char)suffix+0x7F;
			++j;
			++i;
			continue;
		} else 
		if ((prefix == 0x84)&&((suffix>=0x70)&&(suffix<=0x75))){ //E0
			windows1251[j] = (char)suffix+0x70;
			++j;
			++i;
			continue;
		} else 
		if ((prefix == 0x84)&&(suffix==0x76)){ //E0
			windows1251[j] = (char)suffix+0x42;
			++j;
			++i;
			continue;
		} else 
		if ((prefix == 0x84)&&((suffix>=0x77)&&(suffix<=0x7E))){ //E6
			windows1251[j] = (char)suffix+0x6F;
			++j;
			++i;
			continue;
		} else
		if ((prefix == 0x84)&&((suffix>=0x80)&&(suffix<=0x91))){ //E6
			windows1251[j] = (char)suffix+0x6E;
			++j;
			++i;
			continue;
		}		
		if((prefix>=0x20)&&(prefix<0x80)){
			windows1251[j] = prefix;
			++j;
			continue;
		}
		if(prefix<0x20){
			windows1251[j] = 0x20;
			++j;
			continue;
		}			
	}
	return 1;

}