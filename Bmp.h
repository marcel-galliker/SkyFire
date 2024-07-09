// ****************************************************************************
//
//	bmp.h
//
// ****************************************************************************
//
//	Copyright 2014 by Radex AG, Switzerland. All rights reserved.
//	Written by Marcel Galliker 
//
// ****************************************************************************

#pragma once

#include "rx_common.h"

#ifdef __cplusplus
extern "C"{
#endif

//--- SBmpInfo ---------------------------------------------

typedef struct
{
	INT32 srcWidthPx;
	INT32 memWidthPx;
	INT32 lengthPx;
	INT32 bitsPerPixel;
	INT32 lineLen;		// in bytes
	INT32 aligment;	// 8, 16, 32 ,....
	UINT32 dataSize;
	INT32 inverseColor;	// value 1=white
	INT32 topFirst;		// first line is top line
	INT32 printMode;
#define PM_UNDEF				0
#define PM_SCANNING				1
#define PM_SINGLE_PASS			2
#define PM_TEST					3
#define PM_TEST_SINGLE_COLOR	4
#define PM_TEST_JETS			5
#define PM_SCAN_MULTI_PAGE		6

	INT32 scanCopies;
	INT32 planes;		// number of planes
	INT32 colorCode;
	INT32 inkSupplyNo;
	PBYTE *buffer;
	INT8 multiCopy;
	INT8 colorCnt;
	INT8 screening;
} SBmpInfo;



int bmp_load	(const char *path, BYTE **buffer, int bufsize, SBmpInfo *info);

int bmp_write	  (const char *filename, UCHAR *pBuffer, int bitsPerPixel, int width, int length, int bytesPerLine, int invert);

#ifdef __cplusplus
}
#endif