// ****************************************************************************
//
//	rx_common.c
//
// ****************************************************************************
//
//	Copyright 2013 by Radex AG, Switzerland. All rights reserved.
//	Written by Marcel Galliker
//
// ****************************************************************************

#ifdef linux
	#include <stdio.h>
	#include <time.h>
	#include <sys/time.h>
	#include <sys/times.h>
	#include <ctype.h>
	#include "errno.h"
#else
	#include <stdio.h>
	#include <string.h>
	#include <share.h>
	#include <windows.h>
	#include <time.h>
	#include <direct.h>
	#include <io.h>
	#include <conio.h>
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/utime.h>
//	#include "Winuser.h"
#endif
#include "rx_common.h"

#ifdef DEBUG
	const int RX_Debug=TRUE;
#else
	const int RX_Debug=FALSE;
#endif

//--- RX_UdpBlockSize ----------------------------------
UINT32 RX_UdpBlockSize[4] = {(45*32), (90*32), (180*32), (270*32)};

const char *RX_MonthStr[12]= {"JAN", "FEB", "MAR", "APR", "MAI", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"}; 


//--- rx_init --------------------------------------
void rx_init(void)
{
}

//--- rx_end ---------------------------------------
void rx_end(void)
{
//	Trace_end();
}

//--- rx_get_ticks -----------------------------------------
#ifdef linux
static UINT64 sTick0 = 0; // ms
int rx_get_ticks()
{
	if (sTick0==0)
	{
		struct timespec tp;
		clock_gettime(0, &tp);
		sTick0 = 1000*tp.tv_sec+tp.tv_nsec/1000000;
	}
	struct timespec tp;

	clock_gettime(0, &tp);
	return (1000*tp.tv_sec+tp.tv_nsec/1000000)-sTick0;
}
#endif

#ifdef linux
time_t rx_get_system_sec(void)
{
	struct timespec now;
	clock_gettime( CLOCK_REALTIME, &now);
	return now.tv_sec;
}

void rx_get_system_time(UINT64 *pFileTime)
{
	#define EPOCH_DIFF 11644473600LL
		
	struct timespec spec;
	struct timeval tv;
	unsigned long long result = EPOCH_DIFF;

	gettimeofday(&tv, NULL);
	result += tv.tv_sec;
	result *= 10000000LL;
	result += tv.tv_usec * 10;
		
	*pFileTime = result;
}

void rx_get_system_hms(int *hour, int *min, int *sec)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	*hour = tm.tm_hour;
	*min  = tm.tm_min;
	*sec  = tm.tm_sec;
}

void rx_get_system_time_str(char *str, char separator)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(str, "%d%c%s%c%d %d:%02d", tm.tm_mday, separator, RX_MonthStr[tm.tm_mon], separator, tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
}

void rx_get_system_day_str(char *str, char separator)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(str, "%d%c%s%c%d", tm.tm_mday, separator, RX_MonthStr[tm.tm_mon], separator, tm.tm_year + 1900);
}
#else

time_t rx_get_system_sec(void)
{
	time_t time;
	_time64(&time);
	return time;
}

void rx_get_system_time(UINT64 *pFileTime)
{
	GetSystemTimeAsFileTime((LPFILETIME) pFileTime);
}

void rx_get_system_hms(int *hour, int *min, int *sec)
{
	SYSTEMTIME time;
	GetSystemTime(&time);
	*hour = time.wHour;
	*min  = time.wMinute;
	*sec  = time.wSecond;
}

void rx_get_system_time_str(char *str, char separator)
{
	SYSTEMTIME time;
	GetSystemTime(&time);
	sprintf(str, "%d%c%s%c%d %d:%02d", time.wDay, separator, RX_MonthStr[time.wMonth-1], separator, time.wYear, time.wHour, time.wMinute);
}

void rx_get_system_day_str(char *str, char separator)
{
	SYSTEMTIME time;
	GetSystemTime(&time);
	sprintf(str, "%d%c%s%c%d", time.wDay, separator, RX_MonthStr[time.wMonth-1], separator, time.wYear);
}
#endif

//--- rx_get_ticks ----------------------------------------------------------
#ifdef WIN32
static int sTick0 = 0;
int rx_get_ticks()
{
	if (!sTick0) sTick0 = GetTickCount();
	return GetTickCount()-sTick0;
}
#endif

//--- TimetToFileTime -----------------------------------------------------
#ifdef WIN32
void TimetToFileTime( time_t t, LPFILETIME pft )
{
    LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
    pft->dwLowDateTime = (DWORD) ll;
    pft->dwHighDateTime = ll >>32;
}
#endif

//--- FiletimeToTimet --------------------------------------------------
#ifdef WIN32
time_t FiletimeToTimet(const LPFILETIME pft)
{
   ULARGE_INTEGER ull;
   ull.LowPart = pft->dwLowDateTime;
   ull.HighPart = pft->dwHighDateTime;

   return ull.QuadPart / 10000000ULL - 11644473600ULL;
}
#else
time_t FiletimeToTimet(const UINT64 fileTime)
{
   return fileTime / 10000000ULL - 11644473600ULL;
}
#endif

//--- BitSet -------------------------------------
void BitSet  (UINT32 *pBitset, int bitNo)
{
	*pBitset |= (0x01<<bitNo); 
}

//--- BitClear ------------------------------------
void BitClear(UINT32 *pBitset, int bitNo)
{
	*pBitset &= ~(0x01<<bitNo); 
}

//--- BitIsSet -------------------------------------
int  BitIsSet (UINT32   bitset, int bitNo)
{
	return (bitset & (0x01<<bitNo))!=0; 
}

//--- unicode_to_utf8 ---------------------------------------

//	Bits of code point
// 	|	First code point
// 	|	|			Last code point
//		|			|			Bytes in sequence
// 	|	|			|			|	Byte 1		Byte 2		Byte 3		Byte 4		Byte 5		Byte 6
//  7	U+0000		U+007F		1	0xxxxxxx
// 11	U+0080		U+07FF		2	110xxxxx	10xxxxxx
// 16	U+0800		U+FFFF		3	1110xxxx	10xxxxxx	10xxxxxx
// 21	U+10000		U+1FFFFF	4	11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
// 26	U+200000	U+3FFFFFF	5	111110xx	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx
// 31	U+4000000	U+7FFFFFFF	6	1111110x	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx

void unicode_to_utf8(const UINT16 *unicode, char *utf8, int size)
{
	UINT16	uc;
	while (*unicode && size>0)
	{
		if (*unicode < 128)
		{
			*utf8++ = (char)*unicode;
			size--;
		}
		else
		{
			int first_bits = 6;
			const int other_bits = 6;
			int first_val = 0xC0;
			int t = 0, cnt=0;
			char code[8];
			uc = *unicode;
			while (uc >= (1 << first_bits))
			{
				{
					t = 128 | (*unicode & ((1 << other_bits)-1));
					uc >>= other_bits;
					first_val |= 1 << (first_bits);
					first_bits--;
				}
				code[cnt++]=t;
			}
			t = first_val | uc;
			code[cnt++]=t;
			if (size>cnt)
			{
				size-=cnt;
				while(cnt) *utf8++ = code[--cnt];
			}
		}
		unicode++;
	}
	*utf8=0;
}

//--- utf8_to_unicode ------------------------------------------
void utf8_to_unicode(const char *utf8, UINT16 *unicode, int size)
{
	UINT8 ch, cnt;
	UINT32	u32;
	while ((ch=*utf8++) && size>0)
	{
		if (ch < 0x80)
		{
			*unicode++ = ch;
			size--;
		}
		else if ((ch&0xe0)==0xc0) *unicode++ = (ch&0x1f)<<6 | (*utf8++ & 0x3f);
		else
		{
			if ((ch&0xf0)==0xe0)
			{
				u32 = (ch&0x0f);
				cnt = 2;
			}
			else if ((ch&0xf8)==0xf0)
			{
				u32 = (ch&0x07);
				cnt = 3;
			}
			else if ((ch&0xfc)==0xf8)
			{
				u32 = (ch&0x03);
				cnt = 4;
			}
			else if ((ch&0xfe)==0xfc)
			{
				u32 = (ch&0x01);
				cnt = 5;
			}
			while (cnt--)	u32 = u32<<6 | (*utf8++ & 0x3f);
			if (u32<0x10000) *unicode++ = (UINT16)u32;
			else
			{
				u32-=0x10000;
				*unicode++ = 0xd80000 | (UINT16)(u32&0x3ff); // lsb
				*unicode++ = 0xdc0000 | (UINT16)((u32 >> 10) & 0x3ff); // msb
			}
		}
	}
	*unicode++ = 0;
}

//--- char_to_wchar	---------------------------------------------------
wchar_t * char_to_wchar(const char *str, wchar_t *wstr, int size)
{
	int i;
	BYTE* dst = (BYTE*)wstr;
	for (i=0; str[i] && i+1<size; i++)
	{
		*dst++=str[i];
		*dst++=0;	
	}
	*dst++=0;	
	*dst++=0;	
	return wstr;
}

//--- char_to_wchar32	---------------------------------------------------
wchar_t * char_to_wchar32(const char *str, wchar_t *wstr, int size)
{
	int i,n;
	BYTE* dst = (BYTE*)wstr;
	for (i=0; str[i] && i+1<size; i++)
	{
		*dst++=str[i];
		for (n=3;n--;)*dst++=0;	
	}
	for (n=4;n--;)*dst++=0;	
	return wstr;
}

//--- utf16_to_wchar --------------------------------------------------
wchar_t * utf16_to_wchar(const UTF16 *str, wchar_t *wstr, int size)
{
	int i;
	for (i=0; str[i] && i+1<size; i++)
	{
		wstr[i]=(UTF16)str[i];
	}
	wstr[i]=0;
	return wstr;
}

//--- utf16_to_char ---------------------------------------------
char* utf16_to_char(const UTF16 *wstr, char *str, int size)
{
	int i;
	for (i=0; wstr[i] && i+1<size; i++)
	{
		str[i]=(char)wstr[i];
	}
	str[i]=0;
	return str;
}


//--- wchar_to_char ---------------------------------------------------
char* wchar_to_char(const wchar_t *wstr, char *str, int size)
{
	int i;
	UCHAR *src = (UCHAR*)wstr;
	for (i=0; src && i+1<size; i++)
	{
		str[i]=*src;
		src+=2;
	}
	str[i]=0;
	return str;
}

//--- char_to_lower -------------------------------------------------
char* char_to_lower(const char *str, char *out)
{
	while (*str)
	{
		if (*str>='A' && *str<='Z') *out++=(*str++)+'a'-'A';
		else *out++=*str++;
	}

	*out=0;
	return out;
}

//--- getchar_nolock ---------------------------------------
char getchar_nolock(void)
{
	#ifdef linux
	    struct timeval tv = { 0L, 0L };
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		if (select(1, &fds, NULL, NULL, &tv))
			return getchar();
		else
			return 0;
	#else
		return _kbhit()? _getch():0;
	#endif
}

//--- rx_mbstowcs --------------------------------------------------
size_t rx_mbstowcs(UTF16 *wcstr, const char *mbstr, size_t count)
{
	#ifdef WIN32
		return mbstowcs(wcstr, mbstr, count);
	#else
		int len;
		UTF16	*dst = wcstr;
		char    *src = (char*)mbstr;
		/*
		len=mbstowcs(src, mbstr, count);
		*/
		
		len=count;
		while(count--) *dst++ = (UTF16)*src++;
		*dst=0;

		return len;
	#endif
}

//--- microns_to_px ------------------------------------------------------------------------
// converts micro meters to pixels
UINT32 microns_to_px(UINT32 microns, UINT32 dpi)
{
	return (UINT32)((double)microns*dpi/25400.0+0.5);
}

//--- str_start ---------------------------------------------------------------------------
int str_start(const char *str, const char *start)
{
	int i, l, n;
	n=(int)strlen(start);
	l=(int)strlen(str)-n;
	for (i=0; i<=l; i++)
	{
		if (!strncmp(&str[i], start, n)) return i+n;
	}
	return 0;
}

//--- str_start_cut ---------------------------------------------------------------------------
const char *str_start_cut(const char *str, char *start)
{
	int pos=str_start(str, start);
	return &str[pos];
}

//--- str_end ---------------------------------------------------------------------------
int str_end(const char *str, const char *end)
{
	int l, n;
	n=(int)strlen(end);
	l=(int)strlen(str)-n;
	if (l<0) return -1;
	return stricmp(&str[l], end);
}

//--- str_tolower -----------------------------------------------------------------------------
char *str_tolower(char *str)
{
	char *src=str;
	while (*str) 
	{
		*str=tolower(*str);
		str++;
	}
	return src;
}

//--- memempty --------------------------------------
int	memempty(void *ptr, int length)
{
	BYTE *ch;
	for (ch=ptr; length; length--)
	{
		if (*ch++) return FALSE;
	}
	return TRUE;
}

//--- color_path ---------------------------------------------------------------------------
char *color_path(const char *path, const char *colorname, char *colorPath, int size)
{
	char name[128], ext[10];
	int len;

	split_path(path, colorPath, name, ext);
	len = (int)strlen(name)-1;
	while (len>0 && name[len]!='_') name[len--]=0;
	if (len>0) sprintf(&colorPath[strlen(colorPath)], "%s%s%s", name, colorname, ext);
	else       strcpy(colorPath, path); // test
	return colorPath;
}

//--- rx_process_name ---------------------
const char RX_Process_Name[64];

void rx_process_name(const char *arg0)
{
	char ext[16];
	split_path(arg0, NULL, (char*)RX_Process_Name, ext);
}


//--- rx_malloc --------------------------------------------
void *rx_malloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr) memset(ptr, 0, size);
	return ptr;
}

//--- exe_name ---------------------------------------------------------------------------
char *exe_name(const char *deviceStr, char *name, int nameSize)
{
	char str[100];
	strcpy(str, deviceStr);
	str_tolower(str);
	*str = tolower(*str);
	sprintf(name, "rx_%s_ctrl", str);
	return name;
}

//--- split_path --------------------------------------------------------------
// splits a file-path into its components 
void split_path(const char *path, char *dir, char *name, char *ext)
{
	int len;

	if (dir)	*dir=0;
	if (name)	*name=0;
	if (ext)	*ext=0;
	for (len=(int)strlen(path); len; len--)
	{
		if (ext && path[len]=='.')
		{
			strcpy(ext, &path[len]);
		}
		if (path[len-1]==L'/' || path[len-1]==L'\\')
		{
			if (name) 
			{
				strcpy(name, &path[len]);
				if (ext) name[strlen(name)-strlen(ext)]=0;
			}
			if (dir)
			{ 
				memcpy(dir, path, len);
				dir[len]=0;
			}
			return;
		}
	}
	if (name && *name==0)
	{
		strcpy(name, path);
		if (ext) name[strlen(name)-strlen(ext)]=0;
	}
}

//--- add_root -----------------------------------------------
// adds a root to the filepath if there was none
void add_root(char *path, const char *filePath, char *defaultRoot)
{
	char root[MAX_PATH];
	char name[128];

	split_path(filePath, root, name, NULL);
	if (*root)	strcpy (path, filePath);
	else		sprintf(path, "%s%s", defaultRoot, name);
}

//--- goto_xy ---------------------------------------
void goto_xy(int x, int y) 
{ 
#ifdef WIN32
    COORD pos = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
#else
	printf("\x1b[%d;%df",y,x);
#endif
}

//--- swap_uint16 ----------------------------------------$
#ifndef linux
UINT16 swap_uint16(UINT16 val)
{
	BYTE *pval = (BYTE*)&val;
	BYTE res[2];
	res[0] = pval[1];
	res[1] = pval[0];
	return *(UINT16*)res;
}
#endif

//--- swap_uint32 ----------------------------------------$
#ifndef linux
UINT32 swap_uint32(UINT32 val)
{
	BYTE *pval = (BYTE*)&val;
	BYTE res[4];
	res[0] = pval[3];
	res[1] = pval[2];
	res[2] = pval[1];
	res[3] = pval[0];
	return *(UINT32*)res;
}
#endif

//--- swap_uint64 ----------------------------------------
#ifndef linux
UINT64 swap_uint64(UINT64 val)
{
	BYTE *pval = (BYTE*)&val;
	BYTE res[8];
	res[0] = pval[7];
	res[1] = pval[6];
	res[2] = pval[5];
	res[3] = pval[4];
	res[4] = pval[3];
	res[5] = pval[2];
	res[6] = pval[1];
	res[7] = pval[0];
	return *(UINT64*)res;
}
#endif

//--- mac_as_i64 ------------------------------------------
UINT64 mac_as_i64(char *macStr)
{
	int i, l;
	UINT64 res=0, r, offset;

	l=(int)strlen(macStr);
	offset = 0x1;
	for (i=0; i<l; i+=3)
	{
		r = strtol(&macStr[i], NULL, 16);
		res += r*offset;
		offset = offset<<8;
	}
	return res;
}

//--- rx_str_to_enum --------------------------------
BYTE rx_str_to_enum(const char *str, enumToStr converter)
{
	BYTE val;
	for (val=0; val<255; val++)
	{
		if (!stricmp(str, converter(val))) return val;
	}
	return 0;
}