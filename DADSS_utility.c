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

#include <math.h>

#include "DA_DSS_cvi_driver.h"
#include "DADSS_utility.h"

const double DADSS_RangeMultipliers[DADSS_RANGE_COUNT] = {0.5, 1.0, 2.0, 4.0}; 

//==============================================================================
// Static functions

static void inline PolarToCartesian(double mag, double arg, double *x, double *y)
{
	*x = mag*cos(arg);
	*y = mag*sin(arg);
}

static void inline CartesianToPolar(double x, double y, double *mag, double *arg)
{
	*mag = sqrt(x*x+y*y);
	*arg = atan2(y,x);
}

//==============================================================================
// Global functions

/// HIFN  Set MDAC1 and MDAC2 ranges for 1 V, 2.5 V, 5 V and 10 V full ranges
/// HIPAR channel/Channel number
/// HIPAR range/Range value
/// HIRET The return value is 0 on success or a negative value on failure
int DADSS_SetRange(int channel, DADSS_RangeList range)
{
	int ret;
	
	switch(range)
	{
		case DADSS_RANGE_1V:
			if ((ret = DADSS_SetRangeMDAC1(channel, 2)) < 0 ||	// Range MDAC1 x0.5
				(ret = DADSS_SetRangeMDAC2(channel, 0)) < 0)	// Range MDAC2 x1
				return ret;
			break;
		case DADSS_RANGE_2V5:
			if ((ret = DADSS_SetRangeMDAC1(channel, 3)) < 0 ||	// Range MDAC1 x1
				(ret = DADSS_SetRangeMDAC2(channel, 0)) < 0)	// Range MDAC2 x1
				return ret;
			break;
		case DADSS_RANGE_5V:
			if ((ret = DADSS_SetRangeMDAC1(channel, 3)) < 0 ||	// Range MDAC1 x1
				(ret = DADSS_SetRangeMDAC2(channel, 1)) < 0)	// Range MDAC2 x2
				return ret;
			break;
		case DADSS_RANGE_10V:
			if ((ret = DADSS_SetRangeMDAC1(channel, 4)) < 0 ||	// Range MDAC1 x2
				(ret = DADSS_SetRangeMDAC2(channel, 1)) < 0)	// Range MDAC2 x2
				return ret;
			break;
		default:
			return -2;
	}
	return 0;
}

/// HIFN  Reads waveform parameters from the source in polar form (amplitude and phase)
/// HIPAR channel/Channel number
/// HIPAR amplitude/
/// HIPAR phase/
/// HIRET The return value is 0 on success or a negative value on failure
int DADSS_GetWaveformParametersPolar(int channel, double *amplitude, double *phase)
{
	int ret;
	
	if ((ret = DADSS_GetAmplitude(channel, amplitude)) < 0 ||
			(ret = DADSS_GetPhase(channel, phase)) < 0)
		return ret;
	return 0;
}

/// HIFN  Sets waveform parameters in polar form (amplitude and phase)
/// HIPAR channel/Channel number
/// HIPAR amplitude/
/// HIPAR phase/
/// HIRET The return value is 0 on success or a negative value on failure
int DADSS_SetWaveformParametersPolar(int channel, double amplitude, double phase)
{
	int ret;
	
	if ((ret = DADSS_SetAmplitude(channel, amplitude)) < 0 ||
			(ret = DADSS_SetPhase(channel, phase)) < 0)
		return ret;
	return 0;
}

/// HIFN Reads waveform parameters from the source in cartesian form 
/// HIFN (real, in phase, part and imaginary, quadrature, part)
/// HIPAR channel/Channel number
/// HIPAR real/
/// HIPAR imag/
/// HIRET The return value is 0 on success or a negative value on failure
int DADSS_GetWaveformParametersCartesian(int channel, double *real, double *imag)
{
	int ret;
	double amplitude, phase;
	
	if ((ret = DADSS_GetAmplitude(channel, &amplitude)) < 0 ||
			(ret = DADSS_GetPhase(channel, &phase)) < 0)
		return ret;
	PolarToCartesian(amplitude, phase, real, imag);
	return 0;
}

/// HIFN Sets waveform parameters from the source in cartesian form 
/// HIFN (real and imaginary parts)
/// HIPAR channel/Channel number
/// HIPAR real/Real (in-phase) part
/// HIPAR imag/Imaginary (quadrature) part
/// HIRET The return value is 0 on success or a negative value on failure
int DADSS_SetWaveformParametersCartesian(int channel, double real, double imag)
{
	int ret;
	double amplitude, phase;
	
	CartesianToPolar(real, imag, &amplitude, &phase);
	if ((ret = DADSS_SetAmplitude(channel, amplitude)) < 0 ||
			(ret = DADSS_SetPhase(channel, phase)) < 0)
		return ret;
	return 0;
}
