// ****************************************************************************
//
//	bmp.c
//
// ****************************************************************************
//
//	Copyright 2014 by Radex AG, Switzerland. All rights reserved.
//	Written by Marcel Galliker 
//
// ****************************************************************************


//--- includes ----------------------------------------------------------------
// #include "stdafx.h"
#ifdef WIN32
	#include <share.h>
#endif
#include "rx_common.h"
#include "rx_file.h"
#include "bmp.h"

//---  statics --------------------------------------------------------------------

#ifdef linux

	// defined in <wingdi.h>
#pragma pack(1)

	#define WORD	UINT16
//	#define DWORD	UINT32
//	#define LONG	INT32
	typedef DWORD   COLORREF;

	typedef struct tagBITMAPFILEHEADER 
	{
		WORD    bfType;
		DWORD   bfSize;
		WORD    bfReserved1;
		WORD    bfReserved2;
		DWORD   bfOffBits;
	} BITMAPFILEHEADER;//, FAR *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

	typedef struct tagBITMAPINFOHEADER
	{
        DWORD      biSize;
        INT32      biWidth;
        INT32      biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        INT32      biXPelsPerMeter;
        INT32      biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
	} BITMAPINFOHEADER;// , FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

	#define LPBITMAPINFOHEADER BITMAPINFOHEADER*

	#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#pragma pack()
#endif

//--- bmp_get_size --------------------------------------------------------------
int bmp_get_size(const char *path, UINT32 *width, UINT32 *height, UINT8 *bitsPerPixel, UINT32 *memsize)
{
	FILE				*file;
	BITMAPFILEHEADER	header;
	BITMAPINFOHEADER	headerInfo;
	int reply = REPLY_OK;

	*width = *height = *bitsPerPixel = *memsize = 0;

	file = rx_fopen(path, "rb", _SH_DENYNO);
	memset(&headerInfo, 0, sizeof(headerInfo));
	if (!file) return REPLY_NOT_FOUND;
	if (fread(&header, 1, sizeof(header), file)!=sizeof(header))			 reply=REPLY_NOT_FOUND;
	if (strncmp((char*)&header.bfType, "BM", 2))							 reply=REPLY_NOT_FOUND;
	if (fread(&headerInfo, 1, sizeof(headerInfo), file)!=sizeof(headerInfo)) reply=REPLY_NOT_FOUND;
	fclose(file);
	if (reply!=REPLY_OK) return reply;

	*width			= headerInfo.biWidth;
	*height			= abs(headerInfo.biHeight);
	*bitsPerPixel	= (UINT8)headerInfo.biBitCount;
	*memsize		= ((((headerInfo.biWidth + 15) & ~15) * headerInfo.biBitCount) / 8) *(*height);

	return REPLY_OK;
}

//--- bmp_load ----------------------------------------------------------------
int bmp_load(const char *path, BYTE **buffer, int bufsize, SBmpInfo *info)
{
	FILE				*file;
	BITMAPFILEHEADER	header;
	LPBITMAPINFOHEADER	pHeaderInfo;
	UINT32				*color_table;
	long				size;
	int					palsize;
	int					ret;
	UINT64				i;
	BYTE				*pdat;

	file = rx_fopen(path, "rb", _SH_DENYNO);
	if (file!=NULL)
	{
		if (fread(&header, 1, sizeof(header), file))
		{					
			if (strncmp((char*)&header.bfType, "BM", 2)) 
			{
				printf("ERROR: File >>%s<< is not a valid BMP file", path);
				return 1;
			}

			size = header.bfOffBits - sizeof(BITMAPFILEHEADER);
			pHeaderInfo = (LPBITMAPINFOHEADER) malloc(size);
			if (!fread(pHeaderInfo, 1, size, file)) 
			{
				free(pHeaderInfo);
				fclose(file);
				printf("Error: File >>%s<< is not a valid BMP file", path);
				return 2;
			}
			if(pHeaderInfo->biClrUsed) palsize = pHeaderInfo->biClrUsed;
			else
			{
				switch(pHeaderInfo->biBitCount) {
					case 1:		palsize = 2;		break;
					case 4:		palsize = 16;		break;
					case 8:		palsize = 256;		break;
					case 16:
					case 24:
					case 32:	palsize = 0;		break;
					default:	printf("Error: File >>%s<< is not a valid BMP file", path); return 3;
				}
			}

			// --- Bitmap color entry smaller than 256 ---
			if (palsize) color_table = (UINT32*)((UINT8*) pHeaderInfo + sizeof(BITMAPINFOHEADER));
			else		 color_table = NULL;

			info->srcWidthPx	= pHeaderInfo->biWidth;
			info->topFirst		= pHeaderInfo->biHeight<0;
			info->lengthPx		= abs(pHeaderInfo->biHeight);
			info->bitsPerPixel  = pHeaderInfo->biBitCount;
		//	info->lineLen		= (((info->srcWidthPx) + 15) & ~15)  * (info->bitsPerPixel) / 8;
			if (pHeaderInfo->biSizeImage>0)
				info->lineLen	= pHeaderInfo->biSizeImage / info->lengthPx;
			else
				info->lineLen	= (((info->srcWidthPx) + 15) & ~15)  * (info->bitsPerPixel) / 8;
			info->dataSize		= (UINT64)(info->lineLen) * (info->lengthPx);
		//	info->resol.x		= pHeaderInfo->biXPelsPerMeter * 254 / 10000;
		//	info->resol.y		= pHeaderInfo->biYPelsPerMeter * 254 / 10000;
			/*
			if (info->dataSize>bufsize) 
			{
				info->dataSize = bufsize;
				Error(WARN, 0, "File  >>%s<< larger than bufffer, cut from %d to %d bytes", path, pHeaderInfo->biSizeImage, info->dataSize);
			};
			*/
			if (pHeaderInfo->biBitCount==1 && color_table && (color_table[0]&0x00ffffff)==0) info->inverseColor	= TRUE;
			else info->inverseColor  = FALSE;

			// quickest is reading the whole file at once!
			if  ((*buffer)==NULL) *buffer=malloc(info->dataSize);
			if (pHeaderInfo->biHeight>0)
			{
				int h;
				for (h=pHeaderInfo->biHeight-1; h>=0; h--)
				{
					ret=(int)fread(pdat=&(*buffer)[h*info->lineLen], 1, info->lineLen, file);
					if (info->inverseColor) 
						for(i=info->lineLen; i; i--, pdat++)
							(*pdat) = ~(*pdat);						  
				}
			}
			else 
			{	
				ret=(int)fread(*buffer, 1, (size_t)info->dataSize, file);
				if (info->inverseColor) 
					for(i=info->dataSize, pdat=*buffer; i; i--, pdat++)
						(*pdat) = ~(*pdat);						  
			}
			info->inverseColor  = FALSE;

			fclose(file);
		}
		return REPLY_OK;
	}
	return REPLY_NOT_FOUND;
}


// ----------------------------------------------------------------------------
// Fuction:		bmp_write
// Return:		
// Description:	
// ----------------------------------------------------------------------------
int bmp_write(const char *filename, UCHAR *pBuffer, int bitsPerPixel, int width, int length, int bytesPerLine, int invert)
{
// write Print Memory to file
	BITMAPFILEHEADER	header;
	BITMAPINFOHEADER	bmpInfo;
	UINT32				bmiColors[16];
	USHORT				code;
	int					bmpSize;
	int					lineLen;
	int					len, h, align, n;
	int					bbpx;
	BYTE				filler;
	UCHAR				*pdat;
	FILE				*file=NULL;

	if (pBuffer==NULL) return 0;
	
	file = rx_fopen(filename, "wb", _SH_DENYNO);
	if (file)
	{		
		bbpx    =  bitsPerPixel;
		if (bitsPerPixel==2)
		{
			bitsPerPixel=4;
			width = (width+3) & ~3; // works better if it is a multiple of 4
		} 
		if (bbpx==1)	
		{
			lineLen = ((width+31)/32)*4;
			bmpSize = length*lineLen;
		}
		else bmpSize = (width*length)/8*bitsPerPixel;

		memset(&header, 0, sizeof(header));

		memcpy(&header.bfType, &"BM", 2);
		header.bfOffBits		= sizeof(header)+sizeof(bmpInfo)+sizeof(bmiColors);
		header.bfSize			= header.bfOffBits+bmpSize;
		
		fwrite(&header, 1, sizeof(header), file);

		memset(&bmpInfo, 0, sizeof(bmpInfo));
		bmpInfo.biSize			= sizeof(bmpInfo);
		bmpInfo.biWidth			= width;
		bmpInfo.biHeight		= -length;
		bmpInfo.biPlanes		= 1;
		bmpInfo.biBitCount		= bitsPerPixel;
		bmpInfo.biCompression	= 0;
		bmpInfo.biSizeImage		= bmpSize;
		bmpInfo.biClrImportant	= 0;
		bmpInfo.biClrUsed		= 0;
		bmpInfo.biXPelsPerMeter = 0; // ResX*10000/254;
		bmpInfo.biYPelsPerMeter = 0; // ResY*10000/254;

		memset(bmiColors, 0, sizeof(bmiColors));
		if (bbpx==1)
		{
			if (invert)
			{
				bmiColors[0] = RGB(0x00,0x00,0x00);
				bmiColors[1] = RGB(0xff,0xff,0xff);
			}
			else
			{
				bmiColors[0] = RGB(0xff,0xff,0xff);
				bmiColors[1] = RGB(0x00,0x00,0x00);
			}
		}
		else
		{
			bmiColors[0] = RGB(0xff,0xff,0xff);
			bmiColors[1] = RGB(0x7f,0x7f,0x7f);
			bmiColors[2] = RGB(0x3f,0x3f,0x3f);
			bmiColors[3] = RGB(0x1f,0x1f,0x1f);
			bmiColors[4] = RGB(0x0f,0x0f,0x0f);
		}
		fwrite(&bmpInfo, 1, sizeof(bmpInfo), file);
		fwrite(&bmiColors, 1, sizeof(bmiColors), file);

		if (bbpx==1) 
		{
			for (h=0; h<length; h++)
			{
				len=(int)fwrite(pBuffer, 1, lineLen, file);
				pBuffer += bytesPerLine;
			}
		}
		else if (bbpx==2)
		{
			/*
			for (;bmpSize;  bmpSize-=2, pBuffer++)
			{
			//	code= ((*pBuffer & 0x03)<<8) | (((*pBuffer & 0x0c)>>2)<<12) | (((*pBuffer & 0x30)>>4)<<0) | (((*pBuffer & 0xc0)>>6)<<4);
				code= ((*pBuffer & 0x03)<<8) | ((*pBuffer & 0x0c)<<10)      | ((*pBuffer & 0x30)>>4)	  | ((*pBuffer & 0xc0)>>2);
				fwrite(&code, 1, 2, file);
			}

			*/
				align = (width/4)&1;
				filler = 0x00;
				for (h=0; h<length; h++)
				{
					for (n=0, pdat=pBuffer; n<width; n+=4)
					{
						code= ((*pdat & 0x03)<<8) | ((*pdat & 0x0c)<<10) | ((*pdat & 0x30)>>4) | ((*pdat & 0xc0)>>2);
						fwrite(&code, 1, 2, file);
						pdat++;
					}
					if (align) fwrite(&filler, 2, align, file);
					pBuffer+=bytesPerLine;
				}
		}
		else if (bbpx==4)
		{
			align = bytesPerLine&1;
			if (align)
			{
				filler = 0x00;
				for (h=0; h<length; h++)
				{
					fwrite(pBuffer, 1, bytesPerLine, file);
					fwrite(&filler, 1, align, file);
					pBuffer+=bytesPerLine;
				}
			}
			else len=(int)fwrite(pBuffer, 1, bmpSize, file);
		}
		else len=(int)fwrite(pBuffer, 1, bmpSize, file);
		fclose(file);

		return TRUE;
	}
	return FALSE;
}
