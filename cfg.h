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

#ifndef CFG_H
#define CFG_H

#ifdef __cplusplus
	extern "C" {
#endif

//==============================================================================
// Include files
		
#include "main.h"
		
//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// External variables

extern SourceSettings sourceSettings;
extern LockinSettings lockinSettings;
extern ModeSettings modeSettings[];

extern const char defaultSettingsFileName[];
extern char defaultSettingsFile[]; 
extern char defaultSettingsFileDir[];

//==============================================================================
// Global functions

void SetDefaultSettings(void);
void SetDefaultModeSettings(int);  
void LoadSettings(char *);
void SaveSettings(char *);

#ifdef __cplusplus
	}
#endif

#endif /* CFG_H */
