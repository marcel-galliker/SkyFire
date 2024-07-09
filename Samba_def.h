// ****************************************************************************
//
//	
//
// ****************************************************************************
//
//	Copyright 2013 by Radex AG, Switzerland. All rights reserved.
//	Written by Marcel Galliker 
//
// ****************************************************************************

#pragma once

//--- function reply values -----------------------------------
#define REPLY_OK	0
#define REPLY_ERROR 1

//--- macros -------------------------------------------------

// #define SIZEOF(x) (sizeof(x)/sizeof(*(x)))
#define UINT8	unsigned char

//--- nozzle alignment --------------------------------------
#define SB_SR_CNT			64
#define SB_SR_LEN			32
#define SB_NOZZLE_CNT		(SB_SR_CNT*SB_SR_LEN)

void Nozzles_init();


typedef struct
{
	int x;
	int y; // delay
} SLineMapping;

extern SLineMapping	SB_LineMapping[SB_NOZZLE_CNT]; // organized [SB_SR_CNT][SB_SR_LEN]

void Samba_initLineMapping(int backwards, int rightToLeft);
