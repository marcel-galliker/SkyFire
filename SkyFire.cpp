// *************************************************************************************************
//																				
//	SkyFire: 
//																				
// *************************************************************************************************
//
//  
//
//
//
// *************************************************************************************************
//
//    Copyright 2024 Galliker-Engineering GmbH. All rights reserved.		
//    Written by Marcel Galliker, marcel@galliker-engineering.ch.								
//																				
// *************************************************************************************************
#include <iostream>
#include "rx_common.h"
#include "Samba_def.h"
#include "SkyFire_def.h"
#include "bmp.h"

//--- prototypes -----------------------------------------
static int _convert_bmp(char *path);

//--- _convert_bmp --------------------------------------------
static int _convert_bmp(char *path)
{
	SBmpInfo srcInfo;
	BYTE *srcBuf=NULL;
	int ret;
	ret = bmp_load(path, &srcBuf, 0, &srcInfo);
	if (ret) return ret;

	if (srcInfo.bitsPerPixel>1)
	{
		printf("ERROR: only 1 bit/pixel implemented\n");
		free(srcBuf);
		return 1;
	}

	SBmpInfo dstInfo;
	BYTE *dstBuf=NULL;
	{
		memcpy(&dstInfo, &srcInfo, sizeof(dstInfo));
		dstInfo.srcWidthPx  = 1536;
		dstInfo.lengthPx	= srcInfo.lengthPx+1024;
		dstInfo.lineLen		= (dstInfo.srcWidthPx*dstInfo.bitsPerPixel+7)/8;
		dstInfo.dataSize    = dstInfo.lineLen*dstInfo.lengthPx;
		dstBuf = (BYTE*) malloc(dstInfo.dataSize);
	}
	
	{
		memset(dstBuf, 0, dstInfo.dataSize);
		for (int y=0; y<srcInfo.lengthPx; y++)
		{
			for (int n=0; n<SF_NOZZLE_CNT; n++)
			{
				int x=SF_NOZZLE_CNT-1-LineMapping[n].x;
				int delay = LineMapping[n].y;
				if (x>=0 && x<srcInfo.srcWidthPx && y+delay<srcInfo.lengthPx)
				{
					BYTE data = srcBuf[(y+delay)*srcInfo.lineLen + x/8];
					if (data & 0x80>>(x%8)) dstBuf[y*dstInfo.lineLen+n/8] |= 0x80>>(n%8);
				}
			}
			// memcpy(dst, src, srcInfo.lineLen);
			// dst += dstInfo.lineLen;
		}
	}

	char dir[MAX_PATH];
	char name[MAX_PATH];
	char ext[10];
	
	split_path(path, dir, name, ext);

	char dst[MAX_PATH];
	sprintf(dst, "%s%s-conv%s", dir, name, ext);
	bmp_write(dst, dstBuf, dstInfo.bitsPerPixel, dstInfo.srcWidthPx, dstInfo.lengthPx, dstInfo.lineLen, FALSE);

	free(srcBuf);
	free(dstBuf);

	return 0;
}

int main(int argc, char *argv[])
{
	Samba_initLineMapping(FALSE, FALSE);
	SkyFire_initLineMapping(FALSE, FALSE);

	for (int i=1; i<argc; i++)
	{
		printf("arg[%d]: >>%s<<\n", i, argv[i]);
		_convert_bmp(argv[i]);
	}
}
