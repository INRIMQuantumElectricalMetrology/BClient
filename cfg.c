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

#include <inifile.h>
#include <analysis.h>

#include "main.h"
#include "msg.h"
#include "cfg.h"
#include "DADSS_utility.h"

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

SourceSettings sourceSettings = {.dataPathName = "", .dataFileHandle = NULL };
LockinSettings lockinSettings = {.lockinDesc = 0};
ModeSettings modeSettings[MAX_MODES]; 

const char defaultSettingsFileName[] = "bclient.ini";
char defaultSettingsFile[MAX_PATHNAME_LEN]; 
char defaultSettingsFileDir[MAX_PATHNAME_LEN];

//==============================================================================
// Global functions

void SetDefaultSettings(void) 
{
	sourceSettings.nvServer = 1;
	sourceSettings.frequency = 1000;
	sourceSettings.clockFrequency = 20;
	sourceSettings.realFrequency = sourceSettings.frequency;
	for (int i = 0; i < DADSS_CHANNELS; ++i) {
		sourceSettings.range[i] = DADSS_RANGE_2V5;
		snprintf(sourceSettings.label[i], LABEL_SZ, "E%d", i+1);
	}
	sourceSettings.nModes = 2; // Dummy mode 0 is always present
	sourceSettings.activeMode = 1;
	sourceSettings.activeChannel = 0;
	
	lockinSettings.gpibAddress = 8;
	strncpy(lockinSettings.initString, "*RST;*CLS;FMOD 0;RSLP 0", GPIB_BUF_SZ);
	
	for (int i = 1; i < MAX_MODES; ++i) 
		SetDefaultModeSettings(i);
	modeSettings[0] = modeSettings[sourceSettings.activeMode];
}

void SetDefaultModeSettings(int mode) 
{
	snprintf(modeSettings[mode].label, LABEL_SZ, "Mode %d", mode);
	for (int i = 0; i < DADSS_CHANNELS; ++i) {
		modeSettings[mode].channelSettings[i].isLocked = 0;
		modeSettings[mode].channelSettings[i].amplitude = 0;
		modeSettings[mode].channelSettings[i].phase = 0;
		modeSettings[mode].channelSettings[i].real = 0;
		modeSettings[mode].channelSettings[i].imag = 0;
		modeSettings[mode].channelSettings[i].mdac2Code = DADSS_MDAC2_CODE_MAX;
		DADSS_Mdac2CodeToValue(modeSettings[mode].channelSettings[i].mdac2Code, &modeSettings[mode].channelSettings[i].mdac2Val);
		modeSettings[mode].channelSettings[i].lockinGainType = LOCKIN_GAIN_MANUAL;
		modeSettings[mode].channelSettings[i].lockinInputSettings.lockinInputType = LOCKIN_INPUT_VOLTAGE_SINGLE_ENDED;
		modeSettings[mode].channelSettings[i].lockinInputSettings.lockinReserveType = LOCKIN_RESERVE_LOW_NOISE;  
		modeSettings[mode].channelSettings[i].lockinInputSettings.lockinFiltersType = LOCKIN_FILTERS_LINE_NOTCH_IN_BOTH ;  
		modeSettings[mode].channelSettings[i].lockinInputSettings.lockinGroundConnection = LOCKIN_INPUT_FLOAT;
		modeSettings[mode].channelSettings[i].lockinInputSettings.lockinCouplingType = LOCKIN_COUPLING_AC;
		modeSettings[mode].channelSettings[i].balanceThreshold = 1e-5;
	}
}

void LoadSettings(char *fileName)
{
	SourceSettings sourceSettingsTmp = {.dataFileHandle = sourceSettings.dataFileHandle};
	strncpy(sourceSettingsTmp.dataPathName, sourceSettings.dataPathName, MAX_PATHNAME_LEN);
	LockinSettings lockinSettingsTmp = {.lockinDesc = lockinSettings.lockinDesc};
	ModeSettings modeSettingsTmp[MAX_MODES];
	
	IniText iniText = Ini_New(TRUE); 
	if (iniText == 0) {
		warn("%s %s.\n%s.", msgStrings[MSG_LOADING_ERROR], fileName, msgStrings[MSG_OUT_OF_MEMORY]);
		return;
	}

	int ret = 0;
	if ((ret = Ini_ReadFromFile(iniText, fileName)) < 0 ||
			(ret = Ini_GetUInt(iniText, "Source", "Network variables server", &sourceSettingsTmp.nvServer)) <= 0 ||
			(ret = Ini_GetInt(iniText, "Source", "Modes", &sourceSettingsTmp.nModes)) <= 0 ||
			(ret = Ini_GetInt(iniText, "Source", "Active mode", &sourceSettingsTmp.activeMode)) <= 0 ||
			(ret = Ini_GetDouble(iniText, "Source", "Clock frequency", &sourceSettingsTmp.clockFrequency)) <= 0 ||
			(ret = Ini_GetDouble(iniText, "Source", "Frequency", &sourceSettingsTmp.frequency)) <= 0 ||
			(ret = Ini_GetInt(iniText, "Source", "Active channel", &sourceSettingsTmp.activeChannel)) <= 0) {
		if (ret == 0)
			warn("%s %s.\n%s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, 
				 msgStrings[MSG_SETTINGS_MISSING_PARAMETER], "Source");
		else
			warn("%s %s.\n%s %s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, GetGeneralErrorString(ret), 
				 msgStrings[MSG_SETTINGS_SECTION], "Source");
		goto cleanup;
	}
	
	for (int i = 0; i < DADSS_CHANNELS; ++i) {
		char buf[BUF_SZ];
		snprintf(buf, BUF_SZ, "Range %d", i+1);
		if ((ret = Ini_GetInt(iniText, "Source", buf, &sourceSettingsTmp.range[i])) <= 0) {
			if (ret == 0)
				warn("%s %s.\n%s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, 
					 msgStrings[MSG_SETTINGS_MISSING_PARAMETER], "Source");
			else
				warn("%s %s.\n%s %s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, GetGeneralErrorString(ret), 
					 msgStrings[MSG_SETTINGS_SECTION], "Source");
			goto cleanup;
		}
		if (sourceSettingsTmp.range[i] < DADSS_RANGE_1V || 
				sourceSettingsTmp.range[i] > DADSS_RANGE_10V) {
			warn("%s %s.\n%s [%s].", msgStrings[MSG_LOADING_ERROR], fileName,
				 msgStrings[MSG_SETTINGS_PARAMETER_OUT_OF_RANGE], buf);
			goto cleanup;
		}
		snprintf(buf,BUF_SZ, "Label %d",i+1);
		if ((ret = Ini_GetStringIntoBuffer(iniText, "Source", buf, sourceSettingsTmp.label[i], LABEL_SZ)) <= 0) {
			if (ret == 0)
				warn("%s %s.\n%s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, 
					 msgStrings[MSG_SETTINGS_MISSING_PARAMETER], "Source");
			else
				warn("%s %s.\n%s %s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, GetGeneralErrorString(ret), 
					 msgStrings[MSG_SETTINGS_SECTION], "Source");
			goto cleanup;
		}
	}
	
	if (sourceSettingsTmp.nvServer > 1 ||
			sourceSettingsTmp.nModes < 2 || sourceSettingsTmp.nModes > MAX_MODES || 
			sourceSettingsTmp.activeMode < 1 || sourceSettingsTmp.activeMode > sourceSettingsTmp.nModes-1 ||
			sourceSettingsTmp.clockFrequency < DADSS_CLOCKFREQUENCY_MIN || sourceSettingsTmp.clockFrequency > DADSS_CLOCKFREQUENCY_MAX ||
			sourceSettingsTmp.frequency < DADSS_FREQUENCY_MIN || sourceSettingsTmp.frequency > DADSS_FREQUENCY_MAX ||
			sourceSettingsTmp.activeChannel < 0 || sourceSettingsTmp.activeChannel > DADSS_CHANNELS-1) {
		warn("%s %s.\n%s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, 
			 msgStrings[MSG_SETTINGS_PARAMETER_OUT_OF_RANGE], "Source");
		goto cleanup;
	}
	sourceSettingsTmp.realFrequency = sourceSettingsTmp.frequency;
		
	if ((ret = Ini_GetInt(iniText, "Lock-in", "GPIB address", &lockinSettingsTmp.gpibAddress)) <= 0 ||
			(ret = Ini_GetStringIntoBuffer(iniText, "Lock-in", "Init string", 
										   lockinSettingsTmp.initString, GPIB_BUF_SZ)) <= 0) {
		if (ret == 0)
			warn("%s %s.\n%s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, 
				 msgStrings[MSG_SETTINGS_MISSING_PARAMETER], "Lock-in");
		else
			warn("%s %s.\n%s %s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, GetGeneralErrorString(ret), 
				 msgStrings[MSG_SETTINGS_SECTION], "Lock-in");
		goto cleanup;
	}
	
	for (int j = 0; j < sourceSettingsTmp.nModes; ++j) {
		char buf[BUF_SZ];
		snprintf(buf, BUF_SZ, "Mode %d", j);
		if ((ret = Ini_GetStringIntoBuffer(iniText, "Mode Labels", buf, modeSettingsTmp[j].label, LABEL_SZ)) <= 0) {
				if (ret == 0)
					warn("%s %s.\n%s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, 
						 msgStrings[MSG_SETTINGS_MISSING_PARAMETER], buf);
				else
					warn("%s %s.\n%s %s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, GetGeneralErrorString(ret), 
						 msgStrings[MSG_SETTINGS_SECTION], buf);
				goto cleanup;
		}
	}
	
	for (int j = 0; j < sourceSettingsTmp.nModes; ++j) {
		for (int i = 0; i < DADSS_CHANNELS; ++i) {
			char buf[BUF_SZ];
			snprintf(buf, BUF_SZ, "Mode %d Channel %d", j, i+1);
		
			unsigned int mdac2Code;
			if ((ret = Ini_GetBoolean(iniText, buf, "Locked", &modeSettingsTmp[j].channelSettings[i].isLocked)) <= 0 ||
					(ret = Ini_GetDouble(iniText, buf, "Amplitude", &modeSettingsTmp[j].channelSettings[i].amplitude)) <= 0 ||
					(ret = Ini_GetDouble(iniText, buf, "Phase", &modeSettingsTmp[j].channelSettings[i].phase)) <= 0 ||
					(ret = Ini_GetUInt(iniText, buf, "MDAC2 code", &mdac2Code)) <= 0 ||
					(ret = Ini_GetInt(iniText, buf, "Lock-in gain type", 
										  &modeSettingsTmp[j].channelSettings[i].lockinGainType)) <= 0 ||
					(ret = Ini_GetInt(iniText, buf, "Lock-in input type", 
										  &modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinInputType)) <= 0 ||
					(ret = Ini_GetInt(iniText, buf, "Lock-in reserve type", 
										  &modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinReserveType)) <= 0 ||
					(ret = Ini_GetInt(iniText, buf, "Lock-in filters type", 
										  &modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinFiltersType)) <= 0 ||
					(ret = Ini_GetInt(iniText, buf, "Lock-in ground connection", 
										  &modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinGroundConnection)) <= 0 ||
					(ret = Ini_GetInt(iniText, buf, "Lock-in coupling type", 
										  &modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinCouplingType)) <= 0 ||
					(ret = Ini_GetDouble(iniText, buf, "Balance threshold", 
										 &modeSettingsTmp[j].channelSettings[i].balanceThreshold)) <= 0) {
				if (ret == 0)
					warn("%s %s.\n%s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, 
						 msgStrings[MSG_SETTINGS_MISSING_PARAMETER], buf);
				else
					warn("%s %s.\n%s %s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, GetGeneralErrorString(ret), 
						 msgStrings[MSG_SETTINGS_SECTION], buf);
				goto cleanup;
			}
		
			if (modeSettingsTmp[j].channelSettings[i].amplitude < DADSS_AMPLITUDE_MIN ||
					modeSettingsTmp[j].channelSettings[i].amplitude > DADSS_AMPLITUDE_MAX ||
					modeSettingsTmp[j].channelSettings[i].phase < DADSS_PHASE_MIN ||
					modeSettingsTmp[j].channelSettings[i].phase > DADSS_PHASE_MAX ||
					modeSettingsTmp[j].channelSettings[i].balanceThreshold < 0 ||
					mdac2Code > DADSS_MDAC2_CODE_MAX ||
			        modeSettingsTmp[j].channelSettings[i].lockinGainType < LOCKIN_GAIN_MANUAL ||
			   		modeSettingsTmp[j].channelSettings[i].lockinGainType > LOCKIN_GAIN_AUTO_PROGRAM ||
			   		modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinInputType < LOCKIN_INPUT_VOLTAGE_SINGLE_ENDED ||
			   		modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinInputType > LOCKIN_INPUT_CURRENT_1_MOHM ||
					modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinReserveType < LOCKIN_RESERVE_HIGH ||  
					modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinReserveType > LOCKIN_RESERVE_LOW_NOISE ||  
					modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinFiltersType < LOCKIN_FILTERS_NO_OUT ||   
					modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinReserveType > LOCKIN_FILTERS_LINE_NOTCH_IN_BOTH ||   
					modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinCouplingType < LOCKIN_COUPLING_AC ||
					modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinCouplingType > LOCKIN_COUPLING_DC || 
			   		modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinGroundConnection < LOCKIN_INPUT_FLOAT ||
			   		modeSettingsTmp[j].channelSettings[i].lockinInputSettings.lockinGroundConnection > LOCKIN_INPUT_GROUND) {
				warn("%s %s.\n%s [%s].", msgStrings[MSG_LOADING_ERROR], fileName, 
					 msgStrings[MSG_SETTINGS_PARAMETER_OUT_OF_RANGE], buf);
				goto cleanup;
			}
		
			ToRect(modeSettingsTmp[j].channelSettings[i].amplitude, modeSettingsTmp[j].channelSettings[i].phase, 
				   &modeSettingsTmp[j].channelSettings[i].real, &modeSettingsTmp[j].channelSettings[i].imag);
			modeSettingsTmp[j].channelSettings[i].mdac2Code = mdac2Code;
			DADSS_Mdac2CodeToValue(modeSettingsTmp[j].channelSettings[i].mdac2Code, &modeSettingsTmp[j].channelSettings[i].mdac2Val);
		}
	}
	
	sourceSettings = sourceSettingsTmp;
	lockinSettings = lockinSettingsTmp;
	for (int j = 0; j < sourceSettingsTmp.nModes; ++j)
			modeSettings[j] = modeSettingsTmp[j];

cleanup:
	Ini_Dispose(iniText);
}



void SaveSettings(char *fileName)
{
	int ret;
	
	IniText iniText = Ini_New(TRUE); 
	if (!iniText) {
		warn("%s %s.\n%s", msgStrings[MSG_SAVING_ERROR], fileName, msgStrings[MSG_OUT_OF_MEMORY]);
		return;
	}
	
	if ((ret = Ini_PutUInt(iniText, "Source", "Network variables server", sourceSettings.nvServer)) < 0 ||
			(ret = Ini_PutInt(iniText, "Source", "Modes", sourceSettings.nModes)) < 0 ||
			(ret = Ini_PutInt(iniText, "Source", "Active mode", sourceSettings.activeMode)) < 0 ||
			(ret = Ini_PutDouble(iniText, "Source", "Clock frequency", sourceSettings.clockFrequency)) < 0 ||
			(ret = Ini_PutDouble(iniText, "Source", "Frequency", sourceSettings.frequency)) < 0 ||
			(ret = Ini_PutInt(iniText, "Source", "Active channel", sourceSettings.activeChannel)) < 0 ||
			(ret = Ini_PutInt(iniText, "Lock-in", "GPIB address", lockinSettings.gpibAddress)) < 0 ||
			(ret = Ini_PutString(iniText, "Lock-in", "Init string", lockinSettings.initString)) < 0) 
		goto error;
	
	for (int i = 0; i < DADSS_CHANNELS; ++i) {
		char buf[BUF_SZ];
		snprintf(buf, BUF_SZ, "Range %d", i+1);
		if ((ret = Ini_PutInt(iniText, "Source", buf, sourceSettings.range[i])) < 0)
		    goto error;
		snprintf(buf, BUF_SZ, "Label %d", i+1);
		if ((ret = Ini_PutString(iniText, "Source", buf, sourceSettings.label[i])) < 0)  
			goto error;
	}

	for (int i = 0; i < sourceSettings.nModes; ++i) {
		char buf[BUF_SZ];
		snprintf(buf, BUF_SZ, "Mode %d", i);
		if ((ret = Ini_PutString(iniText, "Mode Labels", buf, modeSettings[i].label)) < 0)
			goto error;
	}
	
	for (int j = 0; j < sourceSettings.nModes; ++j) {
		for (int i = 0; i < DADSS_CHANNELS; ++i) {
			char buf[BUF_SZ];
			snprintf(buf, BUF_SZ, "Mode %d Channel %d", j, i+1);
		
			if ((ret = Ini_PutBoolean(iniText, buf, "Locked", modeSettings[j].channelSettings[i].isLocked)) < 0 ||
					(ret = Ini_PutDouble(iniText, buf, "Amplitude", modeSettings[j].channelSettings[i].amplitude)) < 0 ||
					(ret = Ini_PutDouble(iniText, buf, "Phase", modeSettings[j].channelSettings[i].phase)) < 0 ||
					(ret = Ini_PutUInt(iniText, buf, "MDAC2 code", modeSettings[j].channelSettings[i].mdac2Code)) < 0 ||
					(ret = Ini_PutInt(iniText, buf, "Lock-in gain type", 
										  modeSettings[j].channelSettings[i].lockinGainType)) < 0 ||
					(ret = Ini_PutInt(iniText, buf, "Lock-in input type", 
										  modeSettings[j].channelSettings[i].lockinInputSettings.lockinInputType)) < 0 ||
					(ret = Ini_PutInt(iniText, buf, "Lock-in reserve type", 
									      modeSettings[j].channelSettings[i].lockinInputSettings.lockinReserveType)) < 0 || 
					(ret = Ini_PutInt(iniText, buf, "Lock-in filters type",
									      modeSettings[j].channelSettings[i].lockinInputSettings.lockinFiltersType)) < 0 ||
					(ret = Ini_PutInt(iniText, buf, "Lock-in ground connection", 
										  modeSettings[j].channelSettings[i].lockinInputSettings.lockinGroundConnection)) < 0 ||
					(ret = Ini_PutInt(iniText, buf, "Lock-in coupling type", 
										  modeSettings[j].channelSettings[i].lockinInputSettings.lockinCouplingType)) < 0 ||
					(ret = Ini_PutDouble(iniText, buf, "Balance threshold", 
										 modeSettings[j].channelSettings[i].balanceThreshold)) < 0)
				goto error;
		}
	}
		
	if ((ret = Ini_WriteToFile(iniText, fileName)) < 0)
		goto error;
	
	Ini_Dispose(iniText);
	return;
error:
	warn("%s %s.\n%s.", msgStrings[MSG_SAVING_ERROR], fileName, GetGeneralErrorString(ret));
	Ini_Dispose(iniText);
	return;
}
