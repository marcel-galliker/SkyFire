// ****************************************************************************
//
//	rx_common.h
//
// ****************************************************************************
//
//	Copyright 2013 by Radex AG, Switzerland. All rights reserved.
//	Written by Marcel Galliker 
//
// ****************************************************************************

#pragma once

#ifdef linux
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <stdint.h>
	#include <fcntl.h>
	#include <limits.h>
	#include <errno.h>
	#undef _WIN32
#endif
#ifdef WIN32
	#include <Windows.h>
	#include <io.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//--- DLL Export --------------------------------------------------
#ifdef linux
	#define DLL_EXPORT 
#else
	#define DLL_EXPORT EXTERN_C _declspec(dllexport)
#endif

#ifdef _DEBUG
	#define DEBUG
#endif

//--- types ----------------------------------------------
#ifdef linux 
	#define UCHAR unsigned char
#endif
#define UINT8	unsigned  char
#define INT8	char

#if defined(linux)
	#if __WORDSIZE == 64
		#define INT32	int
		#define UINT32	unsigned int
	#else
		#define INT32	long
		#define UINT32	unsigned long
	#endif

	#define BYTE	UCHAR
	typedef BYTE    *PBYTE;
	#define INT16	short
	#define UINT16	unsigned short

	#define UINT	UINT32
	#define INT64	int64_t
	#define UINT64	uint64_t
	#define FLOAT	float
	#define DOUBLE  double
	#define BOOL    INT32
	#define CHAR	char
	#define UTF16	UINT16
	#define DWORD	INT32
	#define SHORT	INT16
	#define USHORT	UINT16
//	#define LONG	INT32
//	#define ULONG	UINT32
	#define LONG	long
	#define ULONG	DWORD

	#define MAXINT	INT_MAX

	#define _stricmp strcasecmp
	#define stricmp _stricmp
	#define _access access

	#define max(a,b)    (((a) > (b)) ? (a) : (b))
	#define min(a,b)    (((a) < (b)) ? (a) : (b))

#elif defined(WIN32)
	#define INT16	short
	#define UINT16	unsigned short
	#define INT32	__int32
	#define UINT32	unsigned __int32
	#define INT64	__int64
	#define UINT64	unsigned __int64
	#define DOUBLE  double
	#define UTF16	wchar_t
	#define BYTE	UCHAR
	typedef int                 INT;

#elif defined(NIOS)
	#include <alt_types.h>
	#define UCHAR alt_u8
	#define BYTE alt_u8
	#define UINT16 alt_u16
	#define INT16 alt_16
	#define UINT32 alt_u32
	#define INT32 alt_32
	#define INT64 alt_64

	#define REPLY_OK 0
	#define REPLY_ERROR 1

	#define RX_TYPES
	
#elif defined(COND)
	#include <stdint.h>

	#define UCHAR 	uint8_t
	#define BYTE 	uint8_t
	#define UINT16 	uint16_t
	#define INT16 	int16_t
	#define UINT32 	uint32_t
	#define INT32 	int32_t
	#define UINT64 	uint64_t 
	#define INT64 	int64_t
	#define UTF16	UINT16
#endif

#define BIT128 UINT64[2]

#ifndef HANDLE
	#define HANDLE void*
#endif

#ifndef FALSE
	#define FALSE	0
#endif
#ifndef TRUE
	#define TRUE	1
#endif

#ifndef NULL
	#define NULL 0
#endif

#define _SH_DENYRW      0x10    /* deny read/write mode */
#define _SH_DENYWR      0x20    /* deny write mode */
#define _SH_DENYRD      0x30    /* deny read mode */
#define _SH_DENYNO      0x40    /* deny none mode */
#define _SH_SECURE      0x80    /* secure mode */

//--- functions ------------------------------------------
#ifdef WIN32
	#define snprintf _snprintf
#endif

//--- common error codes ---------------------------------
#define REPLY_OK		0
#define REPLY_ERROR		1
#define REPLY_NOT_FOUND 2
#define REPLY_NEW_JOB	3
#define REPLY_ABORT		4

#define MAX_PATH 260

#define PI       3.14159265358979323846

#define SIZEOF(x) (sizeof((x))/sizeof((*x)))
#define ReleasePtr(X)	{if (X) { free(X); X=NULL; } }
#define _FL_	__FILE__, __LINE__

#ifdef __cplusplus
extern "C"{
#endif

void 	rx_init(void);
void 	rx_end(void);

extern	const int RX_Debug;

int 	rx_get_ticks(void);
#ifdef WIN32
void	TimetToFileTime( time_t t, LPFILETIME pft );
time_t	FiletimeToTimet(const LPFILETIME pft);
#else
time_t	FiletimeToTimet(const UINT64 pft);
#endif
time_t rx_get_system_sec(void);
void rx_get_system_time(UINT64 *pFileTime);
void rx_get_system_time_str(char *str, char separator);
void rx_get_system_hms(int *hour, int *min, int *sec);
void rx_get_system_day_str(char *str, char separator);

void BitSet  (UINT32 *pBitset, int bitNo);
void BitClear(UINT32 *pBitset, int bitNo);
int  BitIsSet(UINT32   bitset, int bitNo);

#define BIT_SET(bitset, bit)	(bitset) |= (bit) 
#define BIT_CLR(bitset, bit)	(bitset) &= ~(bit) 
	
int	memempty(void *ptr, int length);

void unicode_to_utf8(const UINT16 *unicode, char *utf8, int size);
void utf8_to_unicode(const char *utf8, UINT16 *unicode, int size);

UINT32 microns_to_px(UINT32 microns, UINT32 dpi);

int str_start(const char *str, const char *start);
const char* str_start_cut(const char *str, char *start);
int str_end  (const char *str, const char *end);
char *str_tolower(char *str);
wchar_t * char_to_wchar(const char *str, wchar_t *wstr, int size);
wchar_t * char_to_wchar32(const char *str, wchar_t *wstr, int size);
wchar_t *_char_to_wchar(const char *str);
char*	 wchar_to_char(const wchar_t *wstr, char *str, int size);
char*	_wchar_to_char(const wchar_t *wstr);
wchar_t * utf16_to_wchar(const UTF16 *str, wchar_t *wstr, int size);
char*	  utf16_to_char(const UTF16 *wstr, char *str, int size);
size_t rx_mbstowcs(UTF16 *wcstr, const char *mbstr, size_t count);

char* char_to_lower(const char *str, char *out);
char getchar_nolock(void);

extern const char RX_Process_Name[64];
extern const char *RX_MonthStr[12];
	
void rx_process_name(const char *arg0);

void *rx_malloc(size_t size);

char *exe_name(const char *deviceStr, char *name, int nameSize);
char *color_path(const char *path, const char *fileNameEnd, char *colorPath, int size);
void split_path(const char *path, char *dir, char *name, char *ext);
void add_root(char *path, const char *filePath, char *defaultRoot);

void goto_xy(int x, int y) ;

// void swap_uint32(UINT32 *val);
//
#ifdef linux
	#define swap_uint16(val) __builtin_bswap16(val);
	#define swap_uint32(val) __builtin_bswap32(val);
	#define swap_uint64(val) __builtin_bswap64(val);
#else
	UINT16 swap_uint16(UINT16 val);
	UINT32 swap_uint32(UINT32 val);
	UINT64 swap_uint64(UINT64 val);
#endif

UINT64 mac_as_i64(char *macStr);

typedef char* (*enumToStr)(BYTE value);

BYTE rx_str_to_enum(const char *str, enumToStr converter);	


#ifdef __cplusplus
}
#endif

