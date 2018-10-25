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

#ifndef LOCKIN_H
#define LOCKIN_H

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

//==============================================================================
// Global functions

unsigned long ReadLockinRaw(int, LockinReading *);
unsigned long SetLockinInputRaw(int, LockinInputSettings);

#ifdef __cplusplus
    }
#endif

#endif /* LOCKIN_H */
