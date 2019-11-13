//==============================================================================
//
// VersICaL impedance bridge client
//
// Copyright 2018-2019	Massimo Ortolano <massimo.ortolano@polito.it> 
//                		Martina Marzano <m.marzano@inrim.it>
//
// This code is licensed under MIT license (see LICENSE.txt for details)
//
//==============================================================================

#ifndef DADSS_UTILITY_H
#define DADSS_UTILITY_H

#ifdef __cplusplus
	extern "C" {
#endif

//==============================================================================
// Include files

#include <utility.h> // For the function RoundRealToNearestInteger

//==============================================================================
// Constants

#define DADSS_CHANNELS 7
#define DADSS_RANGE_COUNT 4
#define DADSS_SAMPLES_MIN 10
#define DADSS_SAMPLES_MAX 50000
#define DADSS_CLOCKFREQUENCY_MIN 1.0
#define DADSS_CLOCKFREQUENCY_MAX 20.0
#define DADSS_FREQUENCY_MAX 20000.0
#define DADSS_FREQUENCY_MIN 20.0
#define DADSS_FREQUENCY_MAX 20000.0
#define DADSS_MDAC1_CODE_RANGE 0x20000
#define DADSS_MDAC2_CODE_RANGE 0x40000
#define DADSS_MDAC2_CODE_MIN 0x00000  
#define DADSS_MDAC2_CODE_MAX (DADSS_MDAC2_CODE_RANGE-1)
#define DADSS_MDAC2_VALUE_MIN 0.0
#define DADSS_MDAC2_VALUE_MAX ((double)DADSS_MDAC2_CODE_MAX/DADSS_MDAC2_CODE_RANGE)
#define DADSS_MDAC2_VALUE_LSB (1.0/DADSS_MDAC2_CODE_RANGE)
#define DADSS_AMPLITUDE_MIN 0.0
#define DADSS_AMPLITUDE_MAX 11.0
#define DADSS_PHASE_MIN -3.14159265358979
#define DADSS_PHASE_MAX 3.14159265358979
#define DADSS_ADJ_DELAY 1.0
#define DADSS_REFERENCE_VOLTAGE 3.0
#define DADSS_MAX_RMS_OUTPUT_CURRENT 0.1

#ifdef __cplusplus
	extern "C" {
#endif
		
//==============================================================================
// Types

typedef enum {
	DADSS_OVERRANGE = -1,
	DADSS_RANGE_1V = 0, 
	DADSS_RANGE_2V5, 
	DADSS_RANGE_5V, 
	DADSS_RANGE_10V
} DADSS_RangeList;

//==============================================================================
// Global functions

int inline DADSS_Mdac2CodeToValue(unsigned int code, double *value)
{
	return code > DADSS_MDAC2_CODE_MAX ? -1 : (*value = code/(double)DADSS_MDAC2_CODE_RANGE, 0);	
}

int inline DADSS_Mdac2ValueToCode(double value, unsigned int *code)
{
	if (value < 0)
		return -1;
	*code = (unsigned int)RoundRealToNearestInteger(value*DADSS_MDAC2_CODE_RANGE);
	return *code > DADSS_MDAC2_CODE_MAX ? -1 : 0; 	
}

int DADSS_SetRange(int, DADSS_RangeList);
int DADSS_SetWaveformParametersPolar(int channel, double, double);
int DADSS_GetWaveformParametersPolar(int channel, double *, double *);
int DADSS_SetWaveformParametersCartesian(int channel, double, double);
int DADSS_GetWaveformParametersCartesian(int channel, double *, double *);
DADSS_RangeList DADSS_GetMinimumRange(double);

//==============================================================================
// Global variables

extern const double DADSS_RangeMultipliers[];
extern const double DADSS_RangeMaxAmplitudes[];

#ifdef __cplusplus
	}
#endif

#endif /* DADSS_UTILITY_H */
