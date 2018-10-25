//==============================================================================
//
// VersICaL impedance bridge client
//
// Copyright 2018 Massimo Ortolano <massimo.ortolano@polito.it> 
//                Martina Marzano <martina.marzano@polito.it>
//
// This code is licensed under MIT license (see LICENSE.txt for details)
//
//==============================================================================

//==============================================================================
// Include files

#include <ansi_c.h>
#include <gpib.h>

#include "lockin.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// Static global variables

//==============================================================================
// Static functions

//==============================================================================
// Global variables

//==============================================================================
// Global functions

unsigned long ReadLockinRaw(int lockinDesc, LockinReading *lockinReading)	
{
	char buf[GPIB_BUF_SZ];
	unsigned long ret;

	// Read lock-in time constant
	snprintf(buf, GPIB_BUF_SZ, "OFLT?");
	if ((ret = ibwrt(lockinDesc, buf, strlen(buf))) & ERR ||
		(ret = ibrd(lockinDesc, buf, GPIB_READ_LEN)) & ERR)
		return ret;
	sscanf(buf, "%d", &lockinReading->timeConstantCode);

	// Convert code to actual time constant
	lockinReading->timeConstant = (lockinReading->timeConstantCode % 2) ? 
						 	3.0*pow(10.0,(lockinReading->timeConstantCode-11)/2) : 
							pow(10.0,(lockinReading->timeConstantCode-10)/2);
	lockinReading->adjDelay = AUTOZERO_ADJ_DELAY_FACTOR*lockinReading->timeConstant + AUTOZERO_ADJ_DELAY_BASE;
		 
	snprintf(buf, GPIB_BUF_SZ, "OUTP?1");	// Channel X
	if ((ret = ibwrt(lockinDesc, buf, strlen(buf))) & ERR ||
		(ret = ibrd(lockinDesc, buf, GPIB_READ_LEN)) & ERR)
		return ret;
	sscanf(buf, "%lf", &lockinReading->real);
	
	snprintf(buf, GPIB_BUF_SZ, "OUTP?2");	// Channel Y
	if ((ret = ibwrt(lockinDesc, buf, strlen(buf))) & ERR ||
		(ret = ibrd(lockinDesc, buf, GPIB_READ_LEN)) & ERR)
		return ret;
	sscanf(buf, "%lf", &lockinReading->imag);
	
	return ret;
}

unsigned long SetLockinInputRaw(int lockinDesc, LockinInputSettings lockinInputSettings)	
{
	char buf[GPIB_BUF_SZ];
	unsigned long ret;
	
	snprintf(buf, GPIB_BUF_SZ, "ISRC %d", lockinInputSettings.lockinInputType);	
	if ((ret = ibwrt(lockinDesc, buf, strlen(buf))) & ERR)
		return ret;
	
	snprintf(buf, GPIB_BUF_SZ, "RMOD %d", lockinInputSettings.lockinReserveType);	
	if ((ret = ibwrt(lockinDesc, buf, strlen(buf))) & ERR)
		return ret;
	
	snprintf(buf, GPIB_BUF_SZ, "ILIN %d", lockinInputSettings.lockinFiltersType);	
	if ((ret = ibwrt(lockinDesc, buf, strlen(buf))) & ERR)
		return ret;
		
	snprintf(buf, GPIB_BUF_SZ, "IGND %d", lockinInputSettings.lockinGroundConnection);	
	if ((ret = ibwrt(lockinDesc, buf, strlen(buf))) & ERR)
		return ret;
	
	snprintf(buf, GPIB_BUF_SZ, "ICPL %d", lockinInputSettings.lockinCouplingType);	
	if ((ret = ibwrt(lockinDesc, buf, strlen(buf))) & ERR)
		return ret;

	return ret;
}
