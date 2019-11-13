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

//==============================================================================
// Include files

#include <ansi_c.h>
#include <userint.h>
#include <utility.h> 
#include <analysis.h>

#include "toolbox.h" 

#include "DA_DSS_cvi_driver.h"
#include "panels.h"
#include "main.h"
#include "cfg.h"
#include "msg.h"

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

void CVICALLBACK FileNew (int menuBar, int menuItem, void *callbackData,
						  int panel)
{
	int ret = FileSelectPopup("", 
							  "",
							  "*.*",
							  msgStrings[MSG_POPUP_NEW_FILE_TITLE],
							  VAL_SAVE_BUTTON,
							  0,
							  0,
							  1,
							  1,
							  sourceSettings.dataPathName);
	switch (ret) {
		case VAL_NO_FILE_SELECTED:
			break;
		case VAL_EXISTING_FILE_SELECTED:
		case VAL_NEW_FILE_SELECTED:
			sourceSettings.dataFileHandle = fopen(sourceSettings.dataPathName, "w");
    		if(sourceSettings.dataFileHandle == NULL) {
        		warn("%s %s.", msgStrings[MSG_SAVING_ERROR], sourceSettings.dataPathName);
				strcpy(sourceSettings.dataPathName, "");
				UpdatePanel(panel);
        		return;
    		}
			if (fprintf(sourceSettings.dataFileHandle,"Timestamp\tMode\tFrequency\tActive") < 0 || fflush(sourceSettings.dataFileHandle)) {
				warn("%s %s.", msgStrings[MSG_SAVING_ERROR], sourceSettings.dataPathName);
				goto Error;	
			}
			for (int i = 0; i < DADSS_CHANNELS; ++i) {
				if (fprintf(sourceSettings.dataFileHandle, "\tRe(%s)\tIm(%s)\tThreshold(%s)",\
							sourceSettings.label[i], sourceSettings.label[i], sourceSettings.label[i]) < 0) {
					warn("%s %s.", msgStrings[MSG_SAVING_ERROR], sourceSettings.dataPathName);
					goto Error;
    			}
			}
			if (fflush(sourceSettings.dataFileHandle)) {
				warn("%s %s.", msgStrings[MSG_SAVING_ERROR], sourceSettings.dataPathName);
				goto Error;	
			}
			UpdatePanel(panel);
			break;		
		default:
			die(GetUILErrorString(ret));
	}
	return;
	
Error: 
	FileClose(menuBar, menuItem, callbackData, panel);
	return;
}

void CVICALLBACK FileSave (int menuBar, int menuItem, void *callbackData,
						   int panel)
{
	int nSamples;
	int samples[DADSS_SAMPLES_MAX] = {0};
	double xReal[DADSS_SAMPLES_MAX] = {0.0};
	double xImag[DADSS_SAMPLES_MAX] = {0.0};
	double dacScale;
	double timeStamp;
	char timeStampBuf[16]; // YYYYMMDDTHHMMSS

	if (sourceSettings.dataFileHandle == NULL) 
		FileNew(menuBar, menuItem, callbackData, panel);
	if (sourceSettings.dataFileHandle == NULL)
		return;
	
	GetCurrentDateTime(&timeStamp);
	FormatDateTimeString(timeStamp, "%Y%m%dT%H%M%S", timeStampBuf, sizeof timeStampBuf);
	if (fprintf(sourceSettings.dataFileHandle,"\n%s\t%s\t%.11g\t%d", timeStampBuf, modeSettings[0].label, sourceSettings.realFrequency, sourceSettings.activeChannel+1) < 0) {
		warn("%s %s.", msgStrings[MSG_SAVING_ERROR], sourceSettings.dataPathName);
		goto Error;
	}

	DSSERRCHK(DADSS_GetNumberSamples(&nSamples));
	for (int i = 0; i < DADSS_CHANNELS; ++i) {
		DSSERRCHK(DADSS_GetWaveform(i+1,samples,nSamples));
		for (int j = 0; j < DADSS_SAMPLES_MAX; ++j)
			xReal[j] = samples[j];
		ALERRCHK(FFT(xReal, xImag, nSamples)); 
		dacScale = (modeSettings[0].channelSettings[i].mdac2Val) * \ 
				   (DADSS_RangeMultipliers[sourceSettings.range[i]]/DADSS_MDAC1_CODE_RANGE) * \
				   DADSS_REFERENCE_VOLTAGE/nSamples;
		if (fprintf(sourceSettings.dataFileHandle,"\t% 16.10e\t% 16.10e\t% 16.10e",
					(xImag[nSamples-1]-xImag[1])*dacScale,
					(xReal[1]+xReal[nSamples-1])*dacScale,
					modeSettings[0].channelSettings[i].balanceThreshold) < 0 || fflush(sourceSettings.dataFileHandle)) {
			warn("%s %s.", msgStrings[MSG_SAVING_ERROR], sourceSettings.dataPathName);
			goto Error;
		}
	}
	return;

Error: 
	FileClose(menuBar, menuItem, callbackData, panel);
	return;
}


void CVICALLBACK FileClose (int menuBar, int menuItem, void *callbackData,
							int panel)
{
	if (sourceSettings.dataFileHandle != NULL) {
		fclose(sourceSettings.dataFileHandle);
		sourceSettings.dataFileHandle = NULL;
		strcpy(sourceSettings.dataPathName, "");
		UpdatePanel(panel);
	}
}


void CVICALLBACK FileExit (int menuBar, int menuItem, void *callbackData,
						   int panel)
{
	ManagePanel (panel, EVENT_CLOSE, callbackData, 0, 0);
}

void CVICALLBACK SettingsMenu (int menuBar, int menuItem, void *callbackData,
							   int panel)
{
	char settingsFile[MAX_PATHNAME_LEN];
	int ret = 0; 
	int settingsPanel;
	
	switch (menuItem) {
		case MENUBAR_SETTINGS_CONNECTION:
			UIERRCHK(settingsPanel = LoadPanel(0, panelsFile, PANEL_CON2));
			UIERRCHK(SetCtrlVal(settingsPanel, PANEL_CON2_NV_SERVER, sourceSettings.nvServer));
			UIERRCHK(SetCtrlVal(settingsPanel, PANEL_CON2_LOCKIN_GPIB_ADDRESS, lockinSettings.gpibAddress));
			UIERRCHK(InstallPopup(settingsPanel));
			return;	
		case MENUBAR_SETTINGS_MODES:
			UIERRCHK(settingsPanel = LoadPanel(0, panelsFile, PANEL_MOD));
			UIERRCHK(SetTableColumnAttribute (settingsPanel, PANEL_MOD_LIST, 1, ATTR_MAX_ENTRY_LENGTH, LABEL_SZ-1));
			UIERRCHK(InsertTableRows(settingsPanel, PANEL_MOD_LIST, -1, sourceSettings.nModes-1, VAL_CELL_STRING));
			for (int i = 1; i < sourceSettings.nModes; ++i)
				UIERRCHK(SetTableCellVal(settingsPanel, PANEL_MOD_LIST, MakePoint(1,i), modeSettings[i].label));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_MOD_LIST, ATTR_ENABLE_COLUMN_SIZING, 0));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_MOD_LIST, ATTR_ENABLE_ROW_SIZING, 0));
			UIERRCHK(SetPanelAttribute(settingsPanel, ATTR_CLOSE_ITEM_VISIBLE, 0));
			UIERRCHK(InstallPopup(settingsPanel));
			return;	
		case MENUBAR_SETTINGS_BRIDGE:
			UIERRCHK(settingsPanel = LoadPanel(0, panelsFile, PANEL_BRI));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_BRI_VOLTAGE_CHANNEL_A, ATTR_MIN_VALUE, 1));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_BRI_VOLTAGE_CHANNEL_A, ATTR_MAX_VALUE, DADSS_CHANNELS));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_BRI_CURRENT_CHANNEL_A, ATTR_MIN_VALUE, 1));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_BRI_CURRENT_CHANNEL_A, ATTR_MAX_VALUE, DADSS_CHANNELS));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_BRI_VOLTAGE_CHANNEL_B, ATTR_MIN_VALUE, 1));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_BRI_VOLTAGE_CHANNEL_B, ATTR_MAX_VALUE, DADSS_CHANNELS));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_BRI_CURRENT_CHANNEL_B, ATTR_MIN_VALUE, 1));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_BRI_CURRENT_CHANNEL_B, ATTR_MAX_VALUE, DADSS_CHANNELS));
			UIERRCHK(SetCtrlVal(settingsPanel, PANEL_BRI_VOLTAGE_CHANNEL_A, bridgeSettings.channelAssignment[VOLTAGE_CHANNEL_A]+1));
			UIERRCHK(SetCtrlVal(settingsPanel, PANEL_BRI_CURRENT_CHANNEL_A, bridgeSettings.channelAssignment[CURRENT_CHANNEL_A]+1));
			UIERRCHK(SetCtrlVal(settingsPanel, PANEL_BRI_VOLTAGE_CHANNEL_B, bridgeSettings.channelAssignment[VOLTAGE_CHANNEL_B]+1));
			UIERRCHK(SetCtrlVal(settingsPanel, PANEL_BRI_CURRENT_CHANNEL_B, bridgeSettings.channelAssignment[CURRENT_CHANNEL_B]+1));
			UIERRCHK(SetCtrlVal(settingsPanel, PANEL_BRI_VOLTAGE_RESISTANCE_A, bridgeSettings.seriesResistance[VOLTAGE_CHANNEL_A]));
			UIERRCHK(SetCtrlVal(settingsPanel, PANEL_BRI_CURRENT_RESISTANCE_A, bridgeSettings.seriesResistance[CURRENT_CHANNEL_A]));
			UIERRCHK(SetCtrlVal(settingsPanel, PANEL_BRI_VOLTAGE_RESISTANCE_B, bridgeSettings.seriesResistance[VOLTAGE_CHANNEL_B]));
			UIERRCHK(SetCtrlVal(settingsPanel, PANEL_BRI_CURRENT_RESISTANCE_B, bridgeSettings.seriesResistance[CURRENT_CHANNEL_B]));
			UIERRCHK(InstallPopup(settingsPanel));
			return;	
		case MENUBAR_SETTINGS_PRESET:
			UIERRCHK(settingsPanel = LoadPanel(0, panelsFile, PANEL_PRE));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_PRE_PRIMARY_A, ATTR_MIN_VALUE, 0.0));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_PRE_PRIMARY_A, ATTR_MAX_VALUE, INFINITY));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_PRE_PRIMARY_B, ATTR_MIN_VALUE, 0.0));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_PRE_PRIMARY_B, ATTR_MAX_VALUE, INFINITY));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_A, ATTR_MAX_VALUE, INFINITY));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_B, ATTR_MAX_VALUE, INFINITY));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_PRE_RMS_CURRENT, ATTR_MIN_VALUE, 0.0));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_PRE_RMS_CURRENT, ATTR_MAX_VALUE, DADSS_MAX_RMS_OUTPUT_CURRENT));
			UIERRCHK(SetCtrlAttribute(settingsPanel, PANEL_PRE_SET, ATTR_CALLBACK_DATA, (void *)panel));
			UIERRCHK(InstallPopup(settingsPanel));
			return;
		case MENUBAR_SETTINGS_LOAD: 
			ret = FileSelectPopup("", 
									"*.ini", 
									"*.*", 
									msgStrings[MSG_POPUP_LOAD_SETTINGS_TITLE], 
									VAL_LOAD_BUTTON , 
									0, 
									0, 
									1, 
									0, 
									settingsFile);
			switch (ret) {
				case VAL_NO_FILE_SELECTED:
					break;
				case VAL_EXISTING_FILE_SELECTED:
					LoadSettings(settingsFile);
					UpdatePanelModes(panel);
					UpdatePanel(panel);
					break;
				default:
					die(GetUILErrorString(ret));
			}
			return;
		case MENUBAR_SETTINGS_SAVE: 
			ret = FileSelectPopup("",
									"*.ini", 
									"*.*", 
									msgStrings[MSG_POPUP_SAVE_SETTINGS_TITLE], 
									VAL_SAVE_BUTTON , 
									0, 
									0, 
									1, 
									1, 
									settingsFile);
			switch (ret) {
				case VAL_NO_FILE_SELECTED:
					break;
				case VAL_EXISTING_FILE_SELECTED:
				case VAL_NEW_FILE_SELECTED:
					SaveSettings(settingsFile);
					break;
				default:
					die(GetUILErrorString(ret));
			}
			return;
		case MENUBAR_SETTINGS_RESET:
			UIERRCHK(ret = ConfirmPopup(msgStrings[MSG_POPUP_CONFIRM_TITLE], 
										msgStrings[MSG_POPUP_RESET_SETTINGS]));
			if (ret == 0)
				break;
			SetDefaultSettings();
			UpdatePanelModes(panel);
			UpdatePanel(panel);
		default:
			break;
	}
}
