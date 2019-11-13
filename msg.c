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

const char *msgStrings[] = {
	[MSG_TITLE] = "INRIM Impedance Bridge Client",
	[MSG_VERSION] = "R2019b",
	[MSG_FATAL_ERROR] = "Fatal error",
	[MSG_INTERNAL_ERROR] = "Internal error",
	[MSG_WARNING] = "Warning", 
	[MSG_OUT_OF_MEMORY] = "Out of memory",
	[MSG_MAIN_PANEL_ERROR] = "Error loading main panel", 
	[MSG_LOADING_ERROR] = "Error loading",
	[MSG_SAVING_ERROR] = "Error saving",
	[MSG_SETTINGS_MISSING_PARAMETER] = "Missing parameter in section",
	[MSG_SETTINGS_PARAMETER_OUT_OF_RANGE] = "Parameter out of range in section",
	[MSG_SETTINGS_SECTION] = "in section",
	[MSG_POPUP_CONFIRM_TITLE] = "Confirm",
	[MSG_POPUP_LOAD_SETTINGS_TITLE] = "Load settings file",
	[MSG_POPUP_SAVE_SETTINGS_TITLE] = "Save settings file",
	[MSG_POPUP_RESET_SETTINGS] = "Reset settings to default values?",
	[MSG_POPUP_QUIT] = "Do you really want to quit?",
	[MSG_DEVICE_OPEN_ERROR] = "Error opening",
	[MSG_DEVICE_CLOSE_ERROR] = "Error closing",
	[MSG_DEVICE_INIT_ERROR] = "Error initializing",
	[MSG_DSS_ERROR] = "DSS error",
	[MSG_GPIB_ERROR] = "GPIB error",
	[MSG_ANALYSIS_ERROR] = "Analysis library error",
	[MSG_UNLOCK_CHANNEL] = "Unlock?",
	[MSG_CONNECTING_TITLE] = "Connecting...",
	[MSG_STARTING_TITLE] = "Starting...",
	[MSG_STOPPING_TITLE] = "Stopping...",
	[MSG_MAX_AUTOZERO_STEPS] = "Maximum number of\niterations reached",
	[MSG_RANGE_TOOLTIP] = "Channel range is common to all modes",
	[MSG_CANNOT_REMOVE_ACTIVE_MODE] = "Cannot remove active mode",
	[MSG_POPUP_NEW_FILE_TITLE] = "New",
	[MSG_POPUP_SAVEAS_FILE_TITLE] = "Save As",
	[MSG_EQUAL_CHANNELS] = "Channel numbers cannot be equal",
	[MSG_PRESET_OVERRANGE] = "Voltage values over supported ranges",
};

//==============================================================================
// Global functions

void die(const char *fmt, ...)
{
	char buf[MSG_BUF_SZ];
	va_list ap;

	va_start(ap, fmt);
	
	vsnprintf(buf, MSG_BUF_SZ, fmt, ap);
	MessagePopup(msgStrings[MSG_FATAL_ERROR], buf);
	
	va_end(ap);

	exit(EXIT_FAILURE);
}

void warn(const char *fmt, ...)
{
	char buf[MSG_BUF_SZ];
	va_list ap;

	va_start(ap, fmt);
	
	vsnprintf(buf, MSG_BUF_SZ, fmt, ap);
	MessagePopup("Warning", buf);
	
	va_end(ap);
}
