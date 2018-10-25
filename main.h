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

#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
	extern "C" {
#endif

//==============================================================================
// Include files

#include <stdio.h>

#include "DADSS_utility.h"

		
//==============================================================================
// Constants
		
#define TITLE_BUF_SZ 120
		
#define PI 3.1415926535897932

#define BUF_SZ 1024
#define CLIPBOARD_BUF_SZ 393216
#define GPIB_BUF_SZ 1024
#define GPIB_READ_LEN 50
#define LABEL_SZ 32
		
#define STARTSTOP_STEPS 25
#define STARTSTOP_STEP_DELAY 0.05
		
#define MAX_AUTOZERO_STEPS 25
#define AUTOZERO_ADJ_DELAY_BASE 1.0 
#define AUTOZERO_ADJ_DELAY_FACTOR 10.0
		
#define MAX_MODES 21

//==============================================================================
// Types
		
typedef enum {
	STATE_IDLE,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_RUNNING_UP,
	STATE_RUNNING,
	STATE_RUNNING_DOWN,
	STATE_AUTOZEROING,
	STATE_SWITCHING_MODE
} ProgramState;

typedef enum {
	LOCKIN_INPUT_VOLTAGE_SINGLE_ENDED,
	LOCKIN_INPUT_VOLTAGE_DIFFERENTIAL,
	LOCKIN_INPUT_CURRENT_1_MOHM,
	//LOCKIN_INPUT_CURRENT_100_MOHM
} LockinInputType;

typedef enum {
	LOCKIN_GAIN_MANUAL, 
	LOCKIN_GAIN_AUTO_INTERNAL,
	LOCKIN_GAIN_AUTO_PROGRAM
} LockinGainType;

typedef enum {
	LOCKIN_INPUT_FLOAT, 
	LOCKIN_INPUT_GROUND
} LockinGroundConnection;

typedef enum {
	LOCKIN_COUPLING_AC,
    LOCKIN_COUPLING_DC
} LockinCouplingType;

typedef enum {
	LOCKIN_RESERVE_HIGH,
	LOCKIN_RESERVE_NORMAL,
	LOCKIN_RESERVE_LOW_NOISE,
} LockinReserveType;

typedef enum {
	LOCKIN_FILTERS_NO_OUT,
	LOCKIN_FILTERS_LINE_NOTCH_IN,
	LOCKIN_FILTERS_LINE_NOTCH_IN_2X,
	LOCKIN_FILTERS_LINE_NOTCH_IN_BOTH
} LockinFiltersType;

typedef struct {
	unsigned int nvServer;
	double clockFrequency;
	double frequency;
	double realFrequency;
	DADSS_RangeList range[DADSS_CHANNELS];
	int nModes;
	int activeMode;
	int activeChannel;
	char label[DADSS_CHANNELS][LABEL_SZ]; 
	char dataPathName[MAX_PATHNAME_LEN];
	FILE *dataFileHandle;
} SourceSettings;

typedef struct {
	int gpibAddress;
	char initString[GPIB_BUF_SZ];
	int lockinDesc;
} LockinSettings;

typedef struct {
	LockinInputType lockinInputType;
	LockinGroundConnection lockinGroundConnection;
	LockinCouplingType lockinCouplingType;
	LockinFiltersType lockinFiltersType;
	LockinReserveType lockinReserveType;
} LockinInputSettings;

typedef struct {
	double real;
	double imag;
	int timeConstantCode;
	double timeConstant;
	double adjDelay;
} LockinReading;

typedef struct {
	int isLocked;
	double amplitude;
	double phase;
	double real;
	double imag;
	unsigned int mdac2Code;
	double mdac2Val;
	LockinInputSettings lockinInputSettings;
	LockinGainType lockinGainType;
	double balanceThreshold;
} ChannelSettings;

typedef struct {
	char label[LABEL_SZ];
	ChannelSettings channelSettings[DADSS_CHANNELS];
} ModeSettings;

//==============================================================================
// External variables

extern const char panelsFile[];
extern ProgramState programState;

//==============================================================================
// Global functions

void InitPanelAttributes(int);
void UpdatePanel(int);
void UpdatePanelModes(int);
void UpdatePanelActiveChannel(int);
void UpdatePanelWaveformParameters(int);
void UpdatePanelLockinReading(int, LockinReading); 
void UpdatePanelLockinInputSettings(int);
void UpdatePanelTitle(int);

#ifdef __cplusplus
	}
#endif

#endif /* MAIN_H */
