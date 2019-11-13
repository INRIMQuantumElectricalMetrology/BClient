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
#include <cvirte.h>     
#include <userint.h>
#include <analysis.h>
#include <utility.h>
#include <gpib.h> 

#include "toolbox.h" 

#include "panels.h"
#include "main.h"
#include "msg.h" 
#include "cfg.h"
#include "DA_DSS_cvi_driver.h" 

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

const char panelsFile[] = "panels.uir";
ProgramState programState = STATE_IDLE;

//==============================================================================
// Global functions

int main (int argc, char *argv[])
{
	// Initialize
	if (InitCVIRTE(0, argv, 0) == 0)
		return -1; // Out of memory
	
	// Check for duplicate instances
	int isDuplicate;
	if (CheckForDuplicateAppInstance(ACTIVATE_OTHER_INSTANCE, &isDuplicate) < 0)
		return -1; // Out of memory
	if (isDuplicate)
		return 0; // Prevent duplicate instance

	SetDefaultSettings();

	// Load the configuration file (.ini), if existing
	int settingsPathFound = 0;
	if (GetProjectDir(defaultSettingsFileDir) == 0 &&
			MakePathname(defaultSettingsFileDir, defaultSettingsFileName, defaultSettingsFile) == 0)
		settingsPathFound = 1;
	if (settingsPathFound && FileExists(defaultSettingsFile, 0)) 
		LoadSettings(defaultSettingsFile);
	
	DADSS_SetNameNVServer(sourceSettings.nvServer);

	UIERRCHK(SetSystemAttribute(ATTR_REPORT_LOAD_FAILURE, 0)); 
	int panel = LoadPanel(0, panelsFile, PANEL); 
	if (panel < 0)
		die("%s\n%s: %s", msgStrings[MSG_MAIN_PANEL_ERROR], GetUILErrorString(panel), panelsFile);

	// Initialize the panel
	InitPanelAttributes(panel);
	UpdatePanelModes(panel);
	UpdatePanel(panel);

	// Display the panel and run the user interface
	UIERRCHK(DisplayPanel(panel));
	UIERRCHK(SetSleepPolicy(VAL_SLEEP_NONE));
	int status = RunUserInterface();
	UIERRCHK(DiscardPanel(panel));
	
	// Save the configuration file and exit
	if (settingsPathFound)
		SaveSettings(defaultSettingsFile);
	CloseCVIRTE();
	exit(status);
}

void InitPanelAttributes(int panel)    
{
	UIERRCHK(SetCtrlAttribute(panel, PANEL_LABEL, ATTR_MAX_ENTRY_LENGTH, LABEL_SZ-1));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_CLOCKFREQUENCY, ATTR_MIN_VALUE, DADSS_CLOCKFREQUENCY_MIN));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_CLOCKFREQUENCY, ATTR_MAX_VALUE, DADSS_CLOCKFREQUENCY_MAX));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_FREQUENCY, ATTR_MIN_VALUE, DADSS_FREQUENCY_MIN));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_FREQUENCY, ATTR_MAX_VALUE, DADSS_FREQUENCY_MAX));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_CODE, ATTR_MIN_VALUE, DADSS_MDAC2_CODE_MIN));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_CODE, ATTR_MAX_VALUE, DADSS_MDAC2_CODE_MAX));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_VAL, ATTR_MIN_VALUE, DADSS_MDAC2_VALUE_MIN));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_VAL, ATTR_MAX_VALUE, DADSS_MDAC2_VALUE_MAX+DADSS_MDAC2_VALUE_LSB/8));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_AMPLITUDE, ATTR_MIN_VALUE, DADSS_AMPLITUDE_MIN));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_AMPLITUDE, ATTR_MAX_VALUE, DADSS_AMPLITUDE_MAX));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE, ATTR_MIN_VALUE, DADSS_PHASE_MIN));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE, ATTR_MAX_VALUE, DADSS_PHASE_MAX));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL, ATTR_MIN_VALUE, -DADSS_AMPLITUDE_MAX));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL, ATTR_MAX_VALUE, DADSS_AMPLITUDE_MAX));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_IMAG, ATTR_MIN_VALUE, -DADSS_AMPLITUDE_MAX));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_IMAG, ATTR_MAX_VALUE, DADSS_AMPLITUDE_MAX));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_BALANCE_THRESHOLD, ATTR_MIN_VALUE, 0.0));
	UIERRCHK(SetCtrlAttribute(panel, PANEL_RANGE, ATTR_TOOLTIP_TEXT, msgStrings[MSG_RANGE_TOOLTIP]));
	for (int i = 1; i < sourceSettings.nModes; ++i)
		UIERRCHK(InsertListItem(panel, PANEL_ACTIVE_MODE, -1, modeSettings[i].label, i));
	UIERRCHK(SetCtrlVal(panel, PANEL_ACTIVE_MODE, sourceSettings.activeMode));
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_GAIN_TYPE, -1, "Auto internal", LOCKIN_GAIN_AUTO_INTERNAL));
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_GAIN_TYPE, -1, "Auto program", LOCKIN_GAIN_AUTO_PROGRAM));
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_GAIN_TYPE, -1, "Manual", LOCKIN_GAIN_MANUAL));
	//UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_INPUT_TYPE, -1, "Current 100 Mohm", LOCKIN_INPUT_CURRENT_100_MOHM));
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_INPUT_TYPE, -1, "Current 1 Mohm", LOCKIN_INPUT_CURRENT_1_MOHM));
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_INPUT_TYPE, -1, "Voltage differential", LOCKIN_INPUT_VOLTAGE_DIFFERENTIAL));
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_INPUT_TYPE, -1, "Voltage single-ended", LOCKIN_INPUT_VOLTAGE_SINGLE_ENDED));
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_RESERVE_TYPE, -1, "Low noise", LOCKIN_RESERVE_LOW_NOISE));
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_RESERVE_TYPE, -1, "Normal", LOCKIN_RESERVE_NORMAL)); 
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_RESERVE_TYPE, -1, "High", LOCKIN_RESERVE_HIGH)); 
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_FILTERS_TYPE, -1, "Both", LOCKIN_FILTERS_LINE_NOTCH_IN_BOTH)); 
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_FILTERS_TYPE, -1, "2x line", LOCKIN_FILTERS_LINE_NOTCH_IN_2X));
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_FILTERS_TYPE, -1, "Line", LOCKIN_FILTERS_LINE_NOTCH_IN));  
	UIERRCHK(InsertListItem(panel, PANEL_LOCKIN_FILTERS_TYPE, -1, "No filters", LOCKIN_FILTERS_NO_OUT));  	   
}

void UpdatePanel(int panel)
{		
	UIERRCHK(SetCtrlVal(panel, PANEL_CON2_NV_SERVER, sourceSettings.nvServer));
	UIERRCHK(SetCtrlVal(panel, PANEL_CON2_LOCKIN_GPIB_ADDRESS, lockinSettings.gpibAddress));
	UIERRCHK(SetCtrlVal(panel, PANEL_CLOCKFREQUENCY, sourceSettings.clockFrequency));
	UIERRCHK(SetCtrlVal(panel, PANEL_FREQUENCY, sourceSettings.frequency));
	UIERRCHK(SetCtrlVal(panel, PANEL_REAL_FREQUENCY, sourceSettings.realFrequency)); 
	
	int menuBar = GetPanelMenuBar(panel);
	
	UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_SAVE, ATTR_DIMMED, 0)); 
	switch (programState) {
		case STATE_IDLE:
			UIERRCHK(SetPanelAttribute(panel, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_CONNECT, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlVal(panel, PANEL_CONNECT_LED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_ACTIVE_MODE, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_SET_MODE, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_CLOCKFREQUENCY, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_FREQUENCY, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL_FREQUENCY, ATTR_TEXT_STRIKEOUT, 1));
			UIERRCHK(SetCtrlVal(panel, PANEL_START_STOP_LED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_START_STOP, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE, ATTR_DIMMED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE_NEW, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE_SAVE, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE_CLOSE, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS, ATTR_DIMMED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_CONNECTION, ATTR_DIMMED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_LOAD, ATTR_DIMMED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_RESET, ATTR_DIMMED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_PRESET, ATTR_DIMMED, 1));
			break;
		case STATE_CONNECTING:
			UIERRCHK(SetPanelAttribute(panel,ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlVal(panel, PANEL_CONNECT_LED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL_FREQUENCY, ATTR_TEXT_STRIKEOUT, 1));
			UIERRCHK(SetCtrlVal(panel, PANEL_START_STOP_LED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS, ATTR_DIMMED, 1));
			break;
	  	case STATE_CONNECTED:
			UIERRCHK(SetPanelAttribute(panel, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_CONNECT, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlVal(panel, PANEL_CONNECT_LED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_ACTIVE_MODE, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_SET_MODE, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_CLOCKFREQUENCY, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_FREQUENCY, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL_FREQUENCY, ATTR_TEXT_STRIKEOUT, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_START_STOP, ATTR_DIMMED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE, ATTR_DIMMED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE_NEW, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE_SAVE, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE_CLOSE, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS, ATTR_DIMMED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_CONNECTION, ATTR_DIMMED, 1)); 
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_LOAD, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_RESET, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_PRESET, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlVal(panel, PANEL_START_STOP_LED, 0));
			break;
		case STATE_RUNNING_UP:
		case STATE_RUNNING_DOWN:
		case STATE_AUTOZEROING:
		case STATE_SWITCHING_MODE:
			UIERRCHK(SetPanelAttribute(panel, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL_FREQUENCY, ATTR_TEXT_STRIKEOUT, 0));
			UIERRCHK(SetCtrlVal(panel, PANEL_CONNECT_LED, 1));
			UIERRCHK(SetCtrlVal(panel, PANEL_START_STOP_LED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS, ATTR_DIMMED, 1));
			break;
		case STATE_RUNNING:
			UIERRCHK(SetPanelAttribute(panel, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_CONNECT, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlVal(panel, PANEL_CONNECT_LED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_ACTIVE_MODE, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_SET_MODE, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_CLOCKFREQUENCY, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_FREQUENCY, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL_FREQUENCY, ATTR_TEXT_STRIKEOUT, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_START_STOP, ATTR_DIMMED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE, ATTR_DIMMED, 0));
			if (sourceSettings.dataFileHandle == NULL) {
				UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE_NEW, ATTR_DIMMED, 0));
				UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE_CLOSE, ATTR_DIMMED, 1));
			} else {
				UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE_NEW, ATTR_DIMMED, 1));
				UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE_CLOSE, ATTR_DIMMED, 0));
			}
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_FILE_SAVE, ATTR_DIMMED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS, ATTR_DIMMED, 0));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_CONNECTION, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_LOAD, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_RESET, ATTR_DIMMED, 1));
			UIERRCHK(SetMenuBarAttribute(menuBar, MENUBAR_SETTINGS_PRESET, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlVal(panel, PANEL_START_STOP_LED, 1));
			break;
	}
	
	UIERRCHK(ProcessDrawEvents());
	
	UpdatePanelActiveChannel(panel);
	UpdatePanelTitle(panel);	
}

void UpdatePanelModes(int panel)
{
	UIERRCHK(ClearListCtrl(panel, PANEL_ACTIVE_MODE));
	for (int i = 1; i < sourceSettings.nModes; ++i)
		UIERRCHK(InsertListItem(panel, PANEL_ACTIVE_MODE, -1, modeSettings[i].label, i));
	UIERRCHK(SetCtrlVal(panel, PANEL_ACTIVE_MODE, sourceSettings.activeMode));
	UIERRCHK(ProcessDrawEvents());
}

void UpdatePanelActiveChannel(int panel)
{
	UIERRCHK(SetCtrlVal(panel, PANEL_ACTIVE_CHANNEL, sourceSettings.activeChannel));
	UIERRCHK(SetCtrlVal(panel, PANEL_LABEL, sourceSettings.label[sourceSettings.activeChannel]));
	UIERRCHK(SetCtrlVal(panel, PANEL_IS_LOCKED, modeSettings[0].channelSettings[sourceSettings.activeChannel].isLocked));
	UIERRCHK(SetCtrlVal(panel, PANEL_RANGE, sourceSettings.range[sourceSettings.activeChannel]));
	
	UpdatePanelWaveformParameters(panel);
	
	UIERRCHK(SetCtrlVal(panel, PANEL_BALANCE_THRESHOLD, 
						modeSettings[0].channelSettings[sourceSettings.activeChannel].balanceThreshold));
	
	if (programState == STATE_IDLE) {
		UIERRCHK(SetCtrlAttribute(panel, PANEL_ACTIVE_CHANNEL, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_SWAP_ACTIVE_CHANNEL, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LABEL, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_IS_LOCKED, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_RANGE, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_AMPLITUDE, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_ADD_PIHALF, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_ADD_PI, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_SUBTRACT_PIHALF, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_SUBTRACT_PI, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_IMAG, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_CODE, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_VAL, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_COPY_PHASOR, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_COPY_FFT, ATTR_DIMMED, 1));     
		UIERRCHK(SetCtrlAttribute(panel, PANEL_PASTE_PHASOR, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_COPY_SAMPLES, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_READ, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_BALANCE_THRESHOLD, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_AUTOZERO, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlVal(panel, PANEL_AUTOZERO_LED, 0));
		UIERRCHK(SetCtrlVal(panel, PANEL_OUT_OF_RANGE_LED, 0));
	} else if (programState == STATE_CONNECTED) {
		UIERRCHK(SetCtrlAttribute(panel, PANEL_IS_LOCKED, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_ACTIVE_CHANNEL, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_SWAP_ACTIVE_CHANNEL, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_COPY_PHASOR, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_COPY_FFT, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_COPY_SAMPLES, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_READ, ATTR_DIMMED, 0)); 
		UIERRCHK(SetCtrlVal(panel, PANEL_AUTOZERO_LED, 0));
		UIERRCHK(SetCtrlVal(panel, PANEL_OUT_OF_RANGE_LED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_AUTOZERO, ATTR_DIMMED, 1));
		if (!modeSettings[0].channelSettings[sourceSettings.activeChannel].isLocked) {
			UIERRCHK(SetCtrlAttribute(panel, PANEL_LABEL, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_RANGE, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_AMPLITUDE, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_ADD_PIHALF, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_ADD_PI, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_SUBTRACT_PIHALF, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_SUBTRACT_PI, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_IMAG, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_CODE, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_VAL, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PASTE_PHASOR, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_BALANCE_THRESHOLD, ATTR_DIMMED, 0));
		} else {
			UIERRCHK(SetCtrlAttribute(panel, PANEL_LABEL, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_RANGE, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_AMPLITUDE, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_ADD_PIHALF, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_ADD_PI, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_SUBTRACT_PIHALF, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_SUBTRACT_PI, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_IMAG, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_CODE, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_VAL, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_BALANCE_THRESHOLD, ATTR_DIMMED, 1));
		}
	} else if (programState == STATE_CONNECTING || 
		 programState == STATE_RUNNING_UP || 
		 programState == STATE_RUNNING_DOWN || 
		 programState == STATE_SWITCHING_MODE) {
		UIERRCHK(SetCtrlVal(panel, PANEL_AUTOZERO_LED, 0));
		UIERRCHK(SetCtrlVal(panel, PANEL_OUT_OF_RANGE_LED, 0));
	} else if (programState == STATE_RUNNING) {
		UIERRCHK(SetCtrlAttribute(panel, PANEL_IS_LOCKED, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_ACTIVE_CHANNEL, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_SWAP_ACTIVE_CHANNEL, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_COPY_PHASOR, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_COPY_FFT, ATTR_DIMMED, 0));  
		UIERRCHK(SetCtrlAttribute(panel, PANEL_COPY_SAMPLES, ATTR_DIMMED, 0)); 
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_READ, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlVal(panel, PANEL_AUTOZERO_LED, 0)); 		
		UIERRCHK(SetCtrlAttribute(panel, PANEL_RANGE, ATTR_DIMMED, 1)); 
		if (!modeSettings[0].channelSettings[sourceSettings.activeChannel].isLocked) {
			UIERRCHK(SetCtrlAttribute(panel, PANEL_LABEL, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_AMPLITUDE, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_ADD_PIHALF, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_ADD_PI, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_SUBTRACT_PIHALF, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_SUBTRACT_PI, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_IMAG, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_CODE, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_VAL, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PASTE_PHASOR, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_BALANCE_THRESHOLD, ATTR_DIMMED, 0));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_AUTOZERO, ATTR_DIMMED, 0));
		} else {
			UIERRCHK(SetCtrlAttribute(panel, PANEL_LABEL, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_AMPLITUDE, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_ADD_PIHALF, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_ADD_PI, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_SUBTRACT_PIHALF, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PHASE_SUBTRACT_PI, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_REAL, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_IMAG, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_CODE, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_MDAC2_VAL, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_PASTE_PHASOR, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_BALANCE_THRESHOLD, ATTR_DIMMED, 1));
			UIERRCHK(SetCtrlAttribute(panel, PANEL_AUTOZERO, ATTR_DIMMED, 1));
		}
	} else if (programState == STATE_AUTOZEROING) {
		UIERRCHK(SetCtrlVal(panel, PANEL_AUTOZERO_LED, 1));
		UIERRCHK(SetCtrlVal(panel, PANEL_OUT_OF_RANGE_LED, 0));
	} else
		die(msgStrings[MSG_INTERNAL_ERROR]);
	
	UIERRCHK(ProcessDrawEvents());  
			
	UpdatePanelLockinInputSettings(panel);
}

void UpdatePanelWaveformParameters(int panel)
{
	UIERRCHK(SetCtrlVal(panel, PANEL_AMPLITUDE, modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude));
	UIERRCHK(SetCtrlVal(panel, PANEL_PHASE, modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
	UIERRCHK(SetCtrlVal(panel, PANEL_REAL, modeSettings[0].channelSettings[sourceSettings.activeChannel].real));
	UIERRCHK(SetCtrlVal(panel, PANEL_IMAG, modeSettings[0].channelSettings[sourceSettings.activeChannel].imag));
	UIERRCHK(SetCtrlVal(panel, PANEL_MDAC2_CODE, modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Code));
	UIERRCHK(SetCtrlVal(panel, PANEL_MDAC2_VAL, modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Val));
	UIERRCHK(ProcessDrawEvents());
}

void UpdatePanelLockinReading(int panel, LockinReading lockinReading) 
{
	UIERRCHK(SetCtrlVal(panel, PANEL_LOCKIN_X, lockinReading.real));
	UIERRCHK(SetCtrlVal(panel, PANEL_LOCKIN_Y, lockinReading.imag));
	UIERRCHK(SetCtrlVal(panel, PANEL_LOCKIN_TIME_CONSTANT, lockinReading.timeConstant));
	UIERRCHK(SetCtrlVal(panel, PANEL_LOCKIN_ADJ_DELAY, lockinReading.adjDelay));
	UIERRCHK(ProcessDrawEvents());
}

void UpdatePanelLockinInputSettings(int panel)
{
	UIERRCHK(SetCtrlVal(panel, PANEL_LOCKIN_GAIN_TYPE, 
						modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinGainType));
	UIERRCHK(SetCtrlVal(panel, PANEL_LOCKIN_INPUT_TYPE, 
						modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinInputType));
	UIERRCHK(SetCtrlVal(panel, PANEL_LOCKIN_RESERVE_TYPE, 
						modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinReserveType));
	UIERRCHK(SetCtrlVal(panel, PANEL_LOCKIN_FILTERS_TYPE, 
						modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinFiltersType));
	UIERRCHK(SetCtrlVal(panel, PANEL_LOCKIN_FLOATING, 
						!modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinGroundConnection));
	UIERRCHK(SetCtrlVal(panel, PANEL_LOCKIN_COUPLING_AC, 
						!modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinCouplingType));
	
	if (programState == STATE_IDLE || modeSettings[0].channelSettings[sourceSettings.activeChannel].isLocked || 
		programState == STATE_CONNECTING || programState == STATE_RUNNING_UP || programState == STATE_RUNNING_DOWN ||
	    programState == STATE_AUTOZEROING || programState == STATE_SWITCHING_MODE) {
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_GAIN_TYPE, ATTR_DIMMED, 1));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_INPUT_TYPE, ATTR_DIMMED, 1)); 
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_RESERVE_TYPE, ATTR_DIMMED, 1)); 
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_FILTERS_TYPE, ATTR_DIMMED, 1)); 
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_FLOATING, ATTR_DIMMED, 1)); 
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_COUPLING_AC, ATTR_DIMMED, 1));   
	} else if (programState == STATE_CONNECTED || programState == STATE_RUNNING) {
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_GAIN_TYPE, ATTR_DIMMED, 0));
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_INPUT_TYPE, ATTR_DIMMED, 0)); 
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_RESERVE_TYPE, ATTR_DIMMED, 0)); 
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_FILTERS_TYPE, ATTR_DIMMED, 0)); 
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_FLOATING, ATTR_DIMMED, 0)); 
		UIERRCHK(SetCtrlAttribute(panel, PANEL_LOCKIN_COUPLING_AC, ATTR_DIMMED, 0));
	} else
		die(msgStrings[MSG_INTERNAL_ERROR]);
	
	UIERRCHK(ProcessDrawEvents());
}

void UpdatePanelTitle(int panel)
{
	char buf[TITLE_BUF_SZ];
	
	if (sourceSettings.dataFileHandle == NULL)
		snprintf(buf, TITLE_BUF_SZ, "%s %s", msgStrings[MSG_TITLE], msgStrings[MSG_VERSION]);
	else {
		char dataDriveName[MAX_DRIVENAME_LEN];
		char dataDirName[MAX_DIRNAME_LEN];
		char dataFileName[MAX_FILENAME_LEN];
		SplitPath(sourceSettings.dataPathName, dataDriveName, dataDirName, dataFileName);
		snprintf(buf, TITLE_BUF_SZ, "%s %s - %s", msgStrings[MSG_TITLE], msgStrings[MSG_VERSION], dataFileName);
	}
	UIERRCHK(SetPanelAttribute(panel, ATTR_TITLE, buf));
	UIERRCHK(ProcessDrawEvents());
}
