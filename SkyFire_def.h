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

#include "Samba_def.h"

//--- nozzle alignment --------------------------------------
#define SF_SR_CNT	24
#define SF_SR_LEN	64
#define SF_NOZZLE_CNT	(SF_SR_CNT*SF_SR_LEN)


extern SLineMapping	SF_LineMapping[SF_NOZZLE_CNT];
extern SLineMapping	LineMapping[SF_NOZZLE_CNT];

void SkyFire_initLineMapping(int backwards, int rightToLeft);
