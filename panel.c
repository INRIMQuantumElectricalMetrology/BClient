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
#include "progressbar.h"

#include "panels.h"
#include "main.h"
#include "msg.h" 
#include "cfg.h"
#include "lockin.h"
#include "DA_DSS_cvi_driver.h"
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

//==============================================================================
// Global functions

int CVICALLBACK AutoZero (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double maxAmplitude, amplitude, phase;
	LockinReading lockinReading;
	char buf[GPIB_BUF_SZ];
	struct {
		double real;
		double imag;
	} stimulus[MAX_AUTOZERO_STEPS] = {{0}}, response[MAX_AUTOZERO_STEPS] = {{0}},
	deltaStimulus[MAX_AUTOZERO_STEPS-2] = {{0}}, deltaResponse[MAX_AUTOZERO_STEPS-2] = {{0}},
	sensitivity[MAX_AUTOZERO_STEPS-2] = {{0}}, stimulusCorrection[MAX_AUTOZERO_STEPS-2]= {{0}};

	switch (event)
	{
		case EVENT_COMMIT:
			programState = STATE_AUTOZEROING;
			UpdatePanel(panel);

			DSSERRCHK(DADSS_GetAmplitudeMax(sourceSettings.activeChannel+1, &maxAmplitude));

			// First data point of the equilibrium strategy
			stimulus[0].real = modeSettings[0].channelSettings[sourceSettings.activeChannel].real; 
			stimulus[0].imag = modeSettings[0].channelSettings[sourceSettings.activeChannel].imag;
			if (modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinGainType == LOCKIN_GAIN_AUTO_INTERNAL) {
				snprintf(buf, GPIB_BUF_SZ, "AGAN");	
				GPIBERRCHK(ibwrt(lockinSettings.lockinDesc, buf, strlen(buf)));
			}
			GPIBERRCHK(ReadLockinRaw(lockinSettings.lockinDesc, &lockinReading));
			UpdatePanelLockinReading(panel, lockinReading);
			response[0].real = lockinReading.real;
			response[0].imag = lockinReading.imag;

			// Randomly update the stimulus for the second point
			stimulus[1].real = stimulus[0].real+maxAmplitude*Random(-0.01,0.01);
			stimulus[1].imag = stimulus[0].imag+maxAmplitude*Random(-0.01,0.01);
	
			ToPolar(stimulus[1].real, stimulus[1].imag, &amplitude, &phase);
			if (amplitude > maxAmplitude) {
				SetCtrlVal(panel, PANEL_OUT_OF_RANGE_LED, 1);
				programState = STATE_RUNNING;
				UpdatePanel(panel);
				break; 
			}
			
			DSSERRCHK(DADSS_SetWaveformParametersPolar(sourceSettings.activeChannel+1, amplitude, phase));
			DSSERRCHK(DADSS_UpdateWaveform());
			
			/* Read the outcome */
			Delay(lockinReading.adjDelay);
			DSSERRCHK(DADSS_GetWaveformParametersPolar(sourceSettings.activeChannel+1,
							&modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude, 
							&modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
			ToRect(modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude, 
				   modeSettings[0].channelSettings[sourceSettings.activeChannel].phase, 
				   &modeSettings[0].channelSettings[sourceSettings.activeChannel].real, 
				   &modeSettings[0].channelSettings[sourceSettings.activeChannel].imag);
			UpdatePanelWaveformParameters(panel);
			stimulus[1].real = modeSettings[0].channelSettings[sourceSettings.activeChannel].real;
			stimulus[1].imag = modeSettings[0].channelSettings[sourceSettings.activeChannel].imag;
			if (modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinGainType == LOCKIN_GAIN_AUTO_INTERNAL) {
				snprintf(buf, GPIB_BUF_SZ, "AGAN");	
				GPIBERRCHK(ibwrt(lockinSettings.lockinDesc, buf, strlen(buf)));
			}
			GPIBERRCHK(ReadLockinRaw(lockinSettings.lockinDesc, &lockinReading));
			UpdatePanelLockinReading(panel, lockinReading);
			response[1].real = lockinReading.real;
			response[1].imag = lockinReading.imag;

			// Start the equilibrium procedure
			int k = 2;
			for ( ; k < MAX_AUTOZERO_STEPS && 
					sqrt(response[k-1].real*response[k-1].real +
						 response[k-1].imag*response[k-1].imag) > modeSettings[0].channelSettings[sourceSettings.activeChannel].balanceThreshold;
					++k) {
				// Update the source
				CxSub(stimulus[k-1].real, stimulus[k-1].imag, 
					  stimulus[k-2].real, stimulus[k-2].imag, 
					  &deltaStimulus[k-2].real, &deltaStimulus[k-2].imag);
				CxSub(response[k-1].real, response[k-1].imag, 
					  response[k-2].real, response[k-2].imag, 
					  &deltaResponse[k-2].real, &deltaResponse[k-2].imag); 	
				CxDiv(deltaStimulus[k-2].real, deltaStimulus[k-2].imag, 
					  deltaResponse[k-2].real, deltaResponse[k-2].imag,
					  &sensitivity[k-2].real, &sensitivity[k-2].imag);
				CxMul(sensitivity[k-2].real, sensitivity[k-2].imag,
					  response[k-1].real, response[k-1].imag, 
					  &stimulusCorrection[k-2].real, &stimulusCorrection[k-2].imag);
				CxSub(stimulus[k-1].real, stimulus[k-1].imag, 
					  stimulusCorrection[k-2].real, stimulusCorrection[k-2].imag, 
					  &stimulus[k].real, &stimulus[k].imag); 
			
				ToPolar(stimulus[k].real, stimulus[k].imag, &amplitude, &phase);
				if (amplitude > maxAmplitude) {
					SetCtrlVal(panel, PANEL_OUT_OF_RANGE_LED, 1);
					break; 
				}
			
				DSSERRCHK(DADSS_SetWaveformParametersPolar(sourceSettings.activeChannel+1, amplitude, phase));
				DSSERRCHK(DADSS_UpdateWaveform());
			
				/* Read the outcome */
				DelayWithEventProcessing(lockinReading.adjDelay);
				DSSERRCHK(DADSS_GetWaveformParametersPolar(sourceSettings.activeChannel+1, 
						  &modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude, 
						  &modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
				ToRect(modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude, 
					   modeSettings[0].channelSettings[sourceSettings.activeChannel].phase, 
					   &modeSettings[0].channelSettings[sourceSettings.activeChannel].real, 
					   &modeSettings[0].channelSettings[sourceSettings.activeChannel].imag);
				UpdatePanelWaveformParameters(panel);
				stimulus[k].real = modeSettings[0].channelSettings[sourceSettings.activeChannel].real;
				stimulus[k].imag = modeSettings[0].channelSettings[sourceSettings.activeChannel].imag;
				if (modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinGainType == LOCKIN_GAIN_AUTO_INTERNAL) {
					snprintf(buf, GPIB_BUF_SZ, "AGAN");	
					GPIBERRCHK(ibwrt(lockinSettings.lockinDesc, buf, strlen(buf)));
				}
				GPIBERRCHK(ReadLockinRaw(lockinSettings.lockinDesc, &lockinReading));
				UpdatePanelLockinReading(panel, lockinReading);
				response[k].real = lockinReading.real;
				response[k].imag = lockinReading.imag;
			}
			if (k == MAX_AUTOZERO_STEPS)
				warn("%s.", msgStrings[MSG_MAX_AUTOZERO_STEPS]);
			programState = STATE_RUNNING;
			UpdatePanel(panel);
	}
	return 0;

Error:
	programState = STATE_RUNNING;
	UpdatePanel(panel);
	return 0;
}

int CVICALLBACK Connect (int panel, int control, int event,
						 void *callbackData, int eventData1, int eventData2)
{
	int pbPanel, ret;
	
	switch (event) {
		case EVENT_COMMIT:
			if (programState == STATE_IDLE) { 
				programState = STATE_CONNECTING;
				UpdatePanel(panel);
				
				pbPanel = LoadPanel(panel, panelsFile, PANEL_CON1);
				UIERRCHK(pbPanel);
				UIERRCHK(SetPanelAttribute(pbPanel, ATTR_TITLE, msgStrings[MSG_CONNECTING_TITLE]));
				UIERRCHK(ProgressBar_ConvertFromSlide(pbPanel, PANEL_CON1_PROGRESSBAR));
				UIERRCHK(ProgressBar_SetMilestones(pbPanel, PANEL_CON1_PROGRESSBAR, 
												   12.5, 25.0, 37.5, 50.0, 62.5, 75.0, 87.5, 0.0));
				UIERRCHK(DisplayPanel(pbPanel));
				
				lockinSettings.lockinDesc = ibdev(0, lockinSettings.gpibAddress, 0, T100s, 1, 0);
				if (ibsta & ERR) {
					UIERRCHK(DiscardPanel(pbPanel));
					warn("%s lock-in: %d", msgStrings[MSG_DEVICE_OPEN_ERROR], iberr);
					programState = STATE_IDLE;
					UpdatePanel(panel);
					return 0;
				}			
				UIERRCHK(ProgressBar_AdvanceMilestone(pbPanel, PANEL_CON1_PROGRESSBAR, 0)); // 1
				
				ibwrt(lockinSettings.lockinDesc, lockinSettings.initString, strlen(lockinSettings.initString));
				if (ibsta & ERR) {
					UIERRCHK(DiscardPanel(pbPanel));
					warn("%s lock-in: %d", msgStrings[MSG_DEVICE_INIT_ERROR], iberr);
					ibonl(lockinSettings.lockinDesc, 0);
					programState = STATE_IDLE;
					UpdatePanel(panel);
					return 0;
				}
				// SetLockinInputRaw per active channel
				
				SetLockinInputRaw(lockinSettings.lockinDesc, modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings);
				if (ibsta & ERR) {
					UIERRCHK(DiscardPanel(pbPanel));
					warn("%s lock-in: %d", msgStrings[MSG_GPIB_ERROR], iberr);
					ibonl(lockinSettings.lockinDesc, 0);
					programState = STATE_IDLE;
					UpdatePanel(panel);
					return 0;
				}
				
				UIERRCHK(ProgressBar_AdvanceMilestone(pbPanel, PANEL_CON1_PROGRESSBAR, 0)); // 2
				
				if ((ret = DADSS_StartStop(0)) < 0) {
					UIERRCHK(DiscardPanel(pbPanel));
					warn("%s DSS: %d", msgStrings[MSG_DEVICE_INIT_ERROR], ret);
					ibonl(lockinSettings.lockinDesc, 0);
					programState = STATE_IDLE;
					UpdatePanel(panel);
					return 0;
				}
				UIERRCHK(ProgressBar_AdvanceMilestone(pbPanel, PANEL_CON1_PROGRESSBAR, 0)); // 3
				
				// Update the source to current settings
				if ((ret = DADSS_SetCLKFrequency(sourceSettings.clockFrequency)) < 0 ||
							(Delay(DADSS_ADJ_DELAY), (ret = DADSS_SetFrequency(sourceSettings.frequency))) < 0 ||
							(Delay(DADSS_ADJ_DELAY), (ret = DADSS_GetRealFrequency(&sourceSettings.realFrequency))) < 0) {
					UIERRCHK(DiscardPanel(pbPanel));
					warn("%s DSS: %d", msgStrings[MSG_DEVICE_INIT_ERROR], ret);
					ibonl(lockinSettings.lockinDesc, 0);
					programState = STATE_IDLE;
					UpdatePanel(panel);
					return 0;
				}
				UIERRCHK(ProgressBar_AdvanceMilestone(pbPanel, PANEL_CON1_PROGRESSBAR, 0)); // 4
				
				for (int i = 0; i < DADSS_CHANNELS; ++i) {
					if ((ret = DADSS_SetRange(i+1, sourceSettings.range[i])) < 0)  {
						UIERRCHK(DiscardPanel(pbPanel));
						warn("%s DSS: %d", msgStrings[MSG_DEVICE_INIT_ERROR], ret);
						ibonl(lockinSettings.lockinDesc, 0);
						programState = STATE_IDLE;
						UpdatePanel(panel);
						return 0;
					} 
				}
				
				if ((ret = DADSS_UpdateConfiguration()) < 0) {
					UIERRCHK(DiscardPanel(pbPanel));
					warn("%s DSS: %d", msgStrings[MSG_DEVICE_INIT_ERROR], ret);
					ibonl(lockinSettings.lockinDesc, 0);
					programState = STATE_IDLE;
					UpdatePanel(panel);
					return 0;
				}
				UIERRCHK(ProgressBar_AdvanceMilestone(pbPanel, PANEL_CON1_PROGRESSBAR, 0)); // 5
				
				for (int i = 0; i < DADSS_CHANNELS; ++i) {
					if ((ret = DADSS_SetMDAC2(i+1, modeSettings[0].channelSettings[i].mdac2Code)) < 0)  {
						UIERRCHK(DiscardPanel(pbPanel));
						warn("%s DSS: %d", msgStrings[MSG_DEVICE_INIT_ERROR], ret);
						ibonl(lockinSettings.lockinDesc, 0);
						programState = STATE_IDLE;
						UpdatePanel(panel);
						return 0;
					}
				}
				if ((ret = DADSS_UpdateMDAC2()) < 0) {
					UIERRCHK(DiscardPanel(pbPanel));
					warn("%s DSS: %d", msgStrings[MSG_DEVICE_INIT_ERROR], ret);
					ibonl(lockinSettings.lockinDesc, 0);
					programState = STATE_IDLE;
					UpdatePanel(panel);
					return 0;
				}
				UIERRCHK(ProgressBar_AdvanceMilestone(pbPanel, PANEL_CON1_PROGRESSBAR, 0)); // 6
				
				for (int i = 0; i < DADSS_CHANNELS; ++i) {
					if ((ret = DADSS_SetAmplitude(i+1, modeSettings[0].channelSettings[i].amplitude)) < 0 ||
					   	(ret = DADSS_SetPhase(i+1, modeSettings[0].channelSettings[i].phase)) < 0)  {
						UIERRCHK(DiscardPanel(pbPanel));
						warn("%s DSS: %d", msgStrings[MSG_DEVICE_INIT_ERROR], ret);
						ibonl(lockinSettings.lockinDesc, 0);
						programState = STATE_IDLE;
						UpdatePanel(panel);
						return 0;
					} 							 
				}
				if ((ret = DADSS_UpdateWaveform()) < 0) {
					UIERRCHK(DiscardPanel(pbPanel));
					warn("%s DSS: %d", msgStrings[MSG_DEVICE_INIT_ERROR], ret);
					ibonl(lockinSettings.lockinDesc, 0);
					programState = STATE_IDLE;
					UpdatePanel(panel);
					return 0;
				}
				UIERRCHK(ProgressBar_AdvanceMilestone(pbPanel, PANEL_CON1_PROGRESSBAR, 0)); // 7
				
				
				Delay(DADSS_ADJ_DELAY);
				for (int i = 0; i < DADSS_CHANNELS; ++i) {
					if ((ret = DADSS_GetAmplitude(i+1, &modeSettings[0].channelSettings[i].amplitude)) < 0 ||
					   	(ret = DADSS_GetPhase(i+1, &modeSettings[0].channelSettings[i].phase)) < 0)  {
						UIERRCHK(DiscardPanel(pbPanel));
						warn("%s DSS: %d", msgStrings[MSG_DEVICE_INIT_ERROR], ret);
						ibonl(lockinSettings.lockinDesc, 0);
						programState = STATE_IDLE;
						UpdatePanel(panel);
						return 0;
					} 
					ToRect(modeSettings[0].channelSettings[i].amplitude,
						   modeSettings[0].channelSettings[i].phase, 
						   &modeSettings[0].channelSettings[i].real, 
						   &modeSettings[0].channelSettings[i].imag);
				}
				UIERRCHK(ProgressBar_AdvanceMilestone (pbPanel, PANEL_CON1_PROGRESSBAR, 0)); // 8
				
				UIERRCHK(DiscardPanel(pbPanel));
				programState = STATE_CONNECTED;				
				UpdatePanel(panel);
			} else if (programState == STATE_CONNECTED) { // Disconnect
				ibonl(lockinSettings.lockinDesc, 0);
				if (ibsta & ERR)														 
					warn("%s lock-in, iberr = %d", msgStrings[MSG_DEVICE_CLOSE_ERROR], iberr);

				programState = STATE_IDLE;
				UpdatePanel(panel);
			} else
				die(msgStrings[MSG_INTERNAL_ERROR]);
			break;
	}
	return 0;
}

int CVICALLBACK CopyWaveformParameters (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	char buf[CLIPBOARD_BUF_SZ];
	int nSamples;
	int samples[DADSS_SAMPLES_MAX] = {0};
	double xReal[DADSS_SAMPLES_MAX] = {0.0};
	double xImag[DADSS_SAMPLES_MAX] = {0.0};
	double dacScale;
	

	switch (event)
	{
		case EVENT_COMMIT:
			switch (control) {
				case PANEL_COPY_REAL_FREQUENCY:
					snprintf(buf, sizeof buf, "f = %.11g;", sourceSettings.realFrequency);
					break;
				case PANEL_COPY_PHASOR:
					snprintf(buf, sizeof buf, "%s = %.10g%+.10gi;", sourceSettings.label[sourceSettings.activeChannel],
							 modeSettings[0].channelSettings[sourceSettings.activeChannel].real, 
							 modeSettings[0].channelSettings[sourceSettings.activeChannel].imag);
					break;
				case PANEL_COPY_SAMPLES:
					DSSERRCHK(DADSS_GetNumberSamples(&nSamples));
					DSSERRCHK(DADSS_GetWaveform(sourceSettings.activeChannel+1,samples,nSamples));
					char *cur = buf;
					char *end = buf + sizeof buf; 
					for (int i = 0; i < nSamples && end > cur; ++i) {	
						cur += snprintf(cur, end-cur, "%d\n", samples[i]);
					}
					break;
				case PANEL_COPY_FFT:
					DSSERRCHK(DADSS_GetNumberSamples(&nSamples));
					DSSERRCHK(DADSS_GetWaveform(sourceSettings.activeChannel+1,samples,nSamples));
					for (int i = 0; i < DADSS_SAMPLES_MAX; ++i)
						xReal[i] = samples[i];
					ALERRCHK(FFT(xReal, xImag, nSamples)); 
					dacScale = (modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Val) * \ 
							   (DADSS_RangeMultipliers[sourceSettings.range[sourceSettings.activeChannel]]/DADSS_MDAC1_CODE_RANGE) * \
							   DADSS_REFERENCE_VOLTAGE/nSamples;
					snprintf(buf, sizeof buf, "%s = %.10g%+.10gi;", \
							 sourceSettings.label[sourceSettings.activeChannel], \
							 (xImag[nSamples-1] - xImag[1]) * dacScale, \
							 (xReal[1] + xReal[nSamples-1]) * dacScale);
			}
			UIERRCHK(ClipboardPutText(buf));
			break;
		default:
			break;
	}

Error: 
	return 0;
}

int CVICALLBACK ManagePanel (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_CLOSE: 
			int ret = 0;
			UIERRCHK(ret = ConfirmPopup(msgStrings[MSG_POPUP_CONFIRM_TITLE], msgStrings[MSG_POPUP_QUIT]));
			if (ret == 0)
				return 0;
			if (sourceSettings.dataFileHandle != NULL)
				fclose(sourceSettings.dataFileHandle);
			switch (programState) {
				case STATE_RUNNING:
					StartStop(panel, PANEL_START_STOP, EVENT_COMMIT, NULL, 0, 0);
				case STATE_CONNECTED:
					Connect(panel, PANEL_CONNECT, EVENT_COMMIT, NULL, 0, 0);
					break;
				case STATE_IDLE:
					break;
				default:
					die(msgStrings[MSG_INTERNAL_ERROR]);
			}
			UIERRCHK(QuitUserInterface(0));
			break;
	}
	return 0;
}

int CVICALLBACK ReadLockin (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	char buf[GPIB_BUF_SZ];
	LockinReading lockinReading;

	switch (event)
	{
		case EVENT_COMMIT:
			if (modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinGainType == LOCKIN_GAIN_AUTO_INTERNAL) {
				snprintf(buf, GPIB_BUF_SZ, "AGAN");	
				GPIBERRCHK(ibwrt(lockinSettings.lockinDesc, buf, strlen(buf)));
			}
			GPIBERRCHK(ReadLockinRaw(lockinSettings.lockinDesc, &lockinReading));
			UpdatePanelLockinReading(panel, lockinReading); 
			break;
	}

Error:
	return 0;
}

int CVICALLBACK SetActiveChannel (int panel, int control, int event,
								  void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			UIERRCHK(GetCtrlVal(panel, control, &sourceSettings.activeChannel));
			GPIBERRCHK(SetLockinInputRaw(lockinSettings.lockinDesc, modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings)); 
			UpdatePanelActiveChannel(panel);
			break;
	}
	
Error:
	return 0;
}

int CVICALLBACK SetBalanceThreshold (int panel, int control, int event,
									 void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			UIERRCHK(GetCtrlVal(panel, control, &modeSettings[0].channelSettings[sourceSettings.activeChannel].balanceThreshold));
			break;
	}
	return 0;
}

int CVICALLBACK SetConnection (int panel, int control, int event,
								 void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			switch (control) {
				case PANEL_CON2_OK:
					UIERRCHK(GetCtrlVal(panel, PANEL_CON2_NV_SERVER, &sourceSettings.nvServer));  // 0 - IME-PXI8101  1 - Localhost
					DADSS_SetNameNVServer(sourceSettings.nvServer);
					UIERRCHK(GetCtrlVal(panel, PANEL_CON2_LOCKIN_GPIB_ADDRESS, &lockinSettings.gpibAddress));
					UIERRCHK(RemovePopup(0));
					break;
				case PANEL_CON2_CANCEL:
					UIERRCHK(RemovePopup(0));
					break;
			}
			break;
	}
	return 0;
}

int CVICALLBACK SetFrequency (int panel, int control, int event,
							  void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			switch (control) {
				case PANEL_CLOCKFREQUENCY:
					UIERRCHK(GetCtrlVal(panel, control, &sourceSettings.clockFrequency));
					DSSERRCHK(DADSS_SetCLKFrequency(sourceSettings.clockFrequency));
					Delay(DADSS_ADJ_DELAY);
					UIERRCHK(SetCtrlVal(panel, PANEL_CLOCKFREQUENCY, sourceSettings.clockFrequency));
				case PANEL_FREQUENCY:
					UIERRCHK(GetCtrlVal(panel, PANEL_FREQUENCY, &sourceSettings.frequency));
					DSSERRCHK(DADSS_SetFrequency(sourceSettings.frequency));
					Delay(DADSS_ADJ_DELAY);
					DSSERRCHK(DADSS_GetRealFrequency(&sourceSettings.realFrequency));
					UIERRCHK(SetCtrlVal(panel, PANEL_REAL_FREQUENCY, sourceSettings.realFrequency));
					break;
			}
			break;
	}

Error:
	return 0;
}

int CVICALLBACK SetLabel (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			UIERRCHK(GetCtrlVal(panel, control, sourceSettings.label[sourceSettings.activeChannel]));
			break;	  
	}
	return 0;
}

int CVICALLBACK SetLockinInput (int panel, int control, int event,
								void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			switch (control) {
				case PANEL_LOCKIN_GAIN_TYPE:
					 UIERRCHK(GetCtrlVal(panel, control, 
								(int *)&modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinGainType));
					break;
				case PANEL_LOCKIN_INPUT_TYPE:
					 UIERRCHK(GetCtrlVal(panel, control, 
								(int *)&modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinInputType));
					break;
				case PANEL_LOCKIN_RESERVE_TYPE:
					 UIERRCHK(GetCtrlVal(panel, control, 
								(int *)&modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinReserveType));
					break;
				case PANEL_LOCKIN_FILTERS_TYPE:
					 UIERRCHK(GetCtrlVal(panel, control, 
								(int *)&modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinFiltersType));
					break;
				case PANEL_LOCKIN_COUPLING_AC:
					 modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinCouplingType = 
							!modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinCouplingType;
					break;
				case PANEL_LOCKIN_FLOATING:
					 modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinGroundConnection = 
							!modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings.lockinGroundConnection;
					break;
			}
			SetLockinInputRaw(lockinSettings.lockinDesc, modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings);
			UpdatePanelLockinInputSettings(panel); 
			break;
	}
	
	return 0;
}

int CVICALLBACK SetRange (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			UIERRCHK(GetCtrlVal(panel, control, (int *)&sourceSettings.range[sourceSettings.activeChannel]));
			DSSERRCHK(DADSS_SetRange(sourceSettings.activeChannel+1, 
									 sourceSettings.range[sourceSettings.activeChannel]));
			DSSERRCHK(DADSS_UpdateConfiguration());
			DSSERRCHK(DADSS_UpdateWaveform());
			Delay(DADSS_ADJ_DELAY);
			DSSERRCHK(DADSS_GetWaveformParametersPolar(sourceSettings.activeChannel+1, 
							&modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude,
							&modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
			ToRect(modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude,
					modeSettings[0].channelSettings[sourceSettings.activeChannel].phase, 
				   &modeSettings[0].channelSettings[sourceSettings.activeChannel].real, 
				   &modeSettings[0].channelSettings[sourceSettings.activeChannel].imag);
			UpdatePanelActiveChannel(panel);
			break;
	}

Error:
	return 0;
}

int CVICALLBACK SetWaveformParameters (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			switch (control) {
				case PANEL_AMPLITUDE:
					UIERRCHK(GetCtrlVal(panel, control, &modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude));
					DSSERRCHK(DADSS_SetAmplitude(sourceSettings.activeChannel+1,
													   modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude));
					DSSERRCHK(DADSS_UpdateWaveform());
					break;
				case PANEL_PHASE:
					UIERRCHK(GetCtrlVal(panel, control, &modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
					DSSERRCHK(DADSS_SetPhase(sourceSettings.activeChannel+1,
												   modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
					DSSERRCHK(DADSS_UpdateWaveform());
					break;
				case PANEL_REAL:
					UIERRCHK(GetCtrlVal(panel, control, &modeSettings[0].channelSettings[sourceSettings.activeChannel].real));
					ToPolar(modeSettings[0].channelSettings[sourceSettings.activeChannel].real, 
							modeSettings[0].channelSettings[sourceSettings.activeChannel].imag,
							&modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude, 
							&modeSettings[0].channelSettings[sourceSettings.activeChannel].phase);
					if (modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude > DADSS_AMPLITUDE_MAX)
						modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude = DADSS_AMPLITUDE_MAX;
					DSSERRCHK(DADSS_SetWaveformParametersPolar(sourceSettings.activeChannel+1,
									modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude,
									modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
					DSSERRCHK(DADSS_UpdateWaveform());
					break;
				case PANEL_IMAG:
					UIERRCHK(GetCtrlVal(panel, control, &modeSettings[0].channelSettings[sourceSettings.activeChannel].imag));
					ToPolar(modeSettings[0].channelSettings[sourceSettings.activeChannel].real, 
							modeSettings[0].channelSettings[sourceSettings.activeChannel].imag,
							&modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude, 
							&modeSettings[0].channelSettings[sourceSettings.activeChannel].phase);
					if (modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude > DADSS_AMPLITUDE_MAX)
						modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude = DADSS_AMPLITUDE_MAX;
					DSSERRCHK(DADSS_SetWaveformParametersPolar(sourceSettings.activeChannel+1,
									modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude,
									modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
					DSSERRCHK(DADSS_UpdateWaveform());
					break;
				case PANEL_MDAC2_CODE:
					UIERRCHK(GetCtrlVal(panel, control, &modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Code));
					DSSERRCHK(DADSS_SetMDAC2(sourceSettings.activeChannel+1, 
												 modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Code));
					DSSERRCHK(DADSS_UpdateMDAC2());
					break;
				case PANEL_MDAC2_VAL:
					UIERRCHK(GetCtrlVal(panel, control, &modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Val));
					DSSERRCHK(DADSS_Mdac2ValueToCode(modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Val,
														   &modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Code));
					DSSERRCHK(DADSS_SetMDAC2(sourceSettings.activeChannel+1, 
												   modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Code));
					DSSERRCHK(DADSS_UpdateMDAC2());
					break;
				case PANEL_PHASE_ADD_PIHALF:
					if (modeSettings[0].channelSettings[sourceSettings.activeChannel].phase + PI/2 <= DADSS_PHASE_MAX) {
						modeSettings[0].channelSettings[sourceSettings.activeChannel].phase += PI/2;
						DSSERRCHK(DADSS_SetPhase(sourceSettings.activeChannel+1,
													   modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
						DSSERRCHK(DADSS_UpdateWaveform());
					} else {
						return 0;
					}
					break;
				case PANEL_PHASE_ADD_PI:
					if (modeSettings[0].channelSettings[sourceSettings.activeChannel].phase + PI <= DADSS_PHASE_MAX) {
						modeSettings[0].channelSettings[sourceSettings.activeChannel].phase += PI;
						DSSERRCHK(DADSS_SetPhase(sourceSettings.activeChannel+1,
													   modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
						DSSERRCHK(DADSS_UpdateWaveform());
					} else {
						return 0;
					}
					break;
				case PANEL_PHASE_SUBTRACT_PIHALF:
					if (modeSettings[0].channelSettings[sourceSettings.activeChannel].phase - PI/2 >= DADSS_PHASE_MIN) {
						modeSettings[0].channelSettings[sourceSettings.activeChannel].phase -= PI/2;
						DSSERRCHK(DADSS_SetPhase(sourceSettings.activeChannel+1,
													   modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
						DSSERRCHK(DADSS_UpdateWaveform());
					} else {
						return 0;
					}
					break;
				case PANEL_PHASE_SUBTRACT_PI:
					if (modeSettings[0].channelSettings[sourceSettings.activeChannel].phase - PI >= DADSS_PHASE_MIN) {
						modeSettings[0].channelSettings[sourceSettings.activeChannel].phase -= PI;
						DSSERRCHK(DADSS_SetPhase(sourceSettings.activeChannel+1,
													   modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
						DSSERRCHK(DADSS_UpdateWaveform());
					} else {
						return 0;
					}
					break;
				case PANEL_PASTE_PHASOR:
					char *clipboardText;
					ClipboardGetText(&clipboardText, 0);
					if (clipboardText == 0)
						return 0;
					char clipboardTextTrimmed[CLIPBOARD_BUF_SZ];
					char *p = clipboardTextTrimmed;
					char c;
					for (size_t i = 0; 
							p-clipboardTextTrimmed < CLIPBOARD_BUF_SZ && (c = clipboardText[i]) != '\0'; 
							++i) {
						if (!isspace(c)) {
							*p++ = c;
						}
					}
					double realTmp;
					double imagTmp;
					if (sscanf(clipboardTextTrimmed, "%lf %lf", &realTmp, &imagTmp) == 2) {
						ToPolar(realTmp,imagTmp,
								&modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude, 
								&modeSettings[0].channelSettings[sourceSettings.activeChannel].phase);
					if (modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude > DADSS_AMPLITUDE_MAX)
						modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude = DADSS_AMPLITUDE_MAX;
						DSSERRCHK(DADSS_SetWaveformParametersPolar(sourceSettings.activeChannel+1,
										modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude,
										modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
						DSSERRCHK(DADSS_UpdateWaveform());
					}
					free(clipboardText);
					break;
			}
			Delay(DADSS_ADJ_DELAY);
			DSSERRCHK(DADSS_GetMDAC2(sourceSettings.activeChannel+1,
										   &modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Code));
			DSSERRCHK(DADSS_Mdac2CodeToValue(modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Code,
												   &modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Val));
			DSSERRCHK(DADSS_GetWaveformParametersPolar(sourceSettings.activeChannel+1,
							&modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude,
							&modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
			ToRect(modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude, 
				   modeSettings[0].channelSettings[sourceSettings.activeChannel].phase, 
				   &modeSettings[0].channelSettings[sourceSettings.activeChannel].real, 
				   &modeSettings[0].channelSettings[sourceSettings.activeChannel].imag);
			UpdatePanelWaveformParameters(panel);
			break;
	}

Error:
	return 0;
}

int CVICALLBACK StartStop (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	int pbPanel;
	
	switch (event) {
		case EVENT_COMMIT:
			if (programState == STATE_CONNECTED) { // Start
				programState = STATE_RUNNING_UP;
				UpdatePanel(panel);
	
				UIERRCHK(pbPanel = LoadPanel(panel, panelsFile, PANEL_S));
				UIERRCHK(SetPanelAttribute(pbPanel, ATTR_TITLE, msgStrings[MSG_STARTING_TITLE]));
				UIERRCHK(ProgressBar_ConvertFromSlide(pbPanel, PANEL_S_PROGRESSBAR));
				UIERRCHK(DisplayPanel(pbPanel));
				for (int i = 0; i < DADSS_CHANNELS; ++i)
					DSSERRCHK(DADSS_SetMDAC2(i+1, 0));
				DSSERRCHK(DADSS_UpdateMDAC2());
				DSSERRCHK(DADSS_StartStop(1));
				
				int interruptFlag = 0;
				for (int i = 1; i <= STARTSTOP_STEPS && !interruptFlag; ++i) {
					double t = (double)i/STARTSTOP_STEPS;
					for (int j = 0; j < DADSS_CHANNELS; ++j) {
						unsigned int mdac2Code = (unsigned int)RoundRealToNearestInteger(modeSettings[0].channelSettings[j].mdac2Code*t);
						DSSERRCHK(DADSS_SetMDAC2(j+1, mdac2Code));
					}
					DSSERRCHK(DADSS_UpdateMDAC2());
					Delay(STARTSTOP_STEP_DELAY);
					UIERRCHK(ProgressBar_SetPercentage(pbPanel, PANEL_S_PROGRESSBAR, 100.0*t, 0));
					
					int eventHandle, ctrlHandle;
					UIERRCHK(GetUserEvent(0, &eventHandle, &ctrlHandle));
					switch (ctrlHandle) {
						case PANEL_S_INTERRUPT:
							interruptFlag = 1;
							break;
						default:
							 break;
					}
				}
				
				Delay(DADSS_ADJ_DELAY);
				for (int i = 0; i < DADSS_CHANNELS; ++i) {
						DSSERRCHK(DADSS_GetMDAC2(i+1, &modeSettings[0].channelSettings[i].mdac2Code));
						DSSERRCHK(DADSS_Mdac2CodeToValue(modeSettings[0].channelSettings[i].mdac2Code, 
															   &modeSettings[0].channelSettings[i].mdac2Val));
						DSSERRCHK(DADSS_GetWaveformParametersPolar(i+1, &modeSettings[0].channelSettings[i].amplitude, 
										&modeSettings[0].channelSettings[i].phase));
						ToRect(modeSettings[0].channelSettings[i].amplitude, 
							   modeSettings[0].channelSettings[i].phase, 
							   &modeSettings[0].channelSettings[i].real,
							   &modeSettings[0].channelSettings[i].imag);
				}
					
				UIERRCHK(DiscardPanel(pbPanel));
				programState = STATE_RUNNING;
				UpdatePanel(panel);
			} else if (programState == STATE_RUNNING) { // Stop
				programState = STATE_RUNNING_DOWN;
				UpdatePanel(panel);
	
				UIERRCHK(pbPanel = LoadPanel(panel, panelsFile, PANEL_S));
				UIERRCHK(SetPanelAttribute(pbPanel, ATTR_TITLE, msgStrings[MSG_STOPPING_TITLE]));
				UIERRCHK(ProgressBar_ConvertFromSlide(pbPanel, PANEL_S_PROGRESSBAR));
				UIERRCHK(DisplayPanel(pbPanel));

				int interruptFlag = 0;
				for (int i = STARTSTOP_STEPS; i >= 0 && !interruptFlag; --i) {
					double t = (double)i/STARTSTOP_STEPS;
					for (int j = 0; j < DADSS_CHANNELS; ++j) {
						unsigned int mdac2Code = (unsigned int)RoundRealToNearestInteger(modeSettings[0].channelSettings[j].mdac2Code*t);
						DSSERRCHK(DADSS_SetMDAC2(j+1, mdac2Code));
					}
					DSSERRCHK(DADSS_UpdateMDAC2());
					Delay(STARTSTOP_STEP_DELAY);
					UIERRCHK(ProgressBar_SetPercentage(pbPanel, PANEL_S_PROGRESSBAR, 100.0*t, 0));
					
					int eventHandle, ctrlHandle;
					UIERRCHK(GetUserEvent(0, &eventHandle, &ctrlHandle));
					switch (ctrlHandle) {
						case PANEL_S_INTERRUPT:
							interruptFlag = 1;
							break;
						default:
							 break;
					}				
				}
				Delay(DADSS_ADJ_DELAY);
				
				if (!interruptFlag) {
					DSSERRCHK(DADSS_StartStop(0));

					for (int i = 0; i < DADSS_CHANNELS; ++i)
						DSSERRCHK(DADSS_SetMDAC2(i+1, modeSettings[0].channelSettings[i].mdac2Code));
					DSSERRCHK(DADSS_UpdateMDAC2());

					UIERRCHK(DiscardPanel(pbPanel));
					programState = STATE_CONNECTED;				
					UpdatePanel(panel);
				} else {
					for (int i = 0; i < DADSS_CHANNELS; ++i) {
						DSSERRCHK(DADSS_GetMDAC2(i+1, &modeSettings[0].channelSettings[i].mdac2Code));
						DSSERRCHK(DADSS_Mdac2CodeToValue(modeSettings[0].channelSettings[i].mdac2Code, 
															   &modeSettings[0].channelSettings[i].mdac2Val));
						DSSERRCHK(DADSS_GetWaveformParametersPolar(i+1, &modeSettings[0].channelSettings[i].amplitude,
										&modeSettings[0].channelSettings[i].phase));
						ToRect(modeSettings[0].channelSettings[i].amplitude, 
							   modeSettings[0].channelSettings[i].phase, 
							   &modeSettings[0].channelSettings[i].real,
							   &modeSettings[0].channelSettings[i].imag);
					}
					
					UIERRCHK(DiscardPanel(pbPanel));
					programState = STATE_RUNNING;				
					UpdatePanel(panel);				
				}
			} else
				die(msgStrings[MSG_INTERNAL_ERROR]);
			break;
	}
	return 0;
	
Error:
	UIERRCHK(DiscardPanel(pbPanel));
	ibonl(lockinSettings.lockinDesc, 0);
	programState = STATE_IDLE;
	UpdatePanel(panel);
	return 0;
}

int CVICALLBACK ToggleLock (int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			if (!modeSettings[0].channelSettings[sourceSettings.activeChannel].isLocked) {
				modeSettings[0].channelSettings[sourceSettings.activeChannel].isLocked = 1;
			} else {
				int ans = ConfirmPopup(msgStrings[MSG_POPUP_CONFIRM_TITLE], msgStrings[MSG_UNLOCK_CHANNEL]);
				UIERRCHK(ans);
				if (ans == 0) {
					SetCtrlVal(panel, control, 1);
					return 0;
				}
				modeSettings[0].channelSettings[sourceSettings.activeChannel].isLocked = 0;
				SetCtrlVal(panel, control, 0);
			}
			UpdatePanelActiveChannel(panel);
			break;
	}
	return 0;
}

int CVICALLBACK SetModes (int panel, int control, int event,
							  void *callbackData, int eventData1, int eventData2)
{
	Point activeTableCell;
	
	switch (event)
	{
		case EVENT_COMMIT:
			switch (control) {
				case PANEL_MOD_ADD:
					if (sourceSettings.nModes < MAX_MODES) {
						UIERRCHK(InsertTableRows(panel, PANEL_MOD_LIST, -1, 1, VAL_CELL_STRING));
						SetDefaultModeSettings(sourceSettings.nModes);
						UIERRCHK(SetTableCellVal(panel, PANEL_MOD_LIST, MakePoint(1,sourceSettings.nModes), modeSettings[sourceSettings.nModes].label));
						sourceSettings.nModes++;
					}
					break;
				case PANEL_MOD_DUPLICATE:
					if (sourceSettings.nModes < MAX_MODES) {
						UIERRCHK(GetActiveTableCell (panel, PANEL_MOD_LIST, &activeTableCell));
						UIERRCHK(InsertTableRows(panel, PANEL_MOD_LIST, -1, 1, VAL_CELL_STRING));
						modeSettings[sourceSettings.nModes] = modeSettings[activeTableCell.y];
						UIERRCHK(SetTableCellVal(panel, PANEL_MOD_LIST, MakePoint(1,sourceSettings.nModes), modeSettings[sourceSettings.nModes].label));
						sourceSettings.nModes++;
					}
					break;
				case PANEL_MOD_REMOVE:
					if (sourceSettings.nModes > sourceSettings.activeMode+1) {
						UIERRCHK(DeleteTableRows(panel, PANEL_MOD_LIST, sourceSettings.nModes-1, -1));
						sourceSettings.nModes--;
					} else
						warn("%s.", msgStrings[MSG_CANNOT_REMOVE_ACTIVE_MODE]);
					break;
				case PANEL_MOD_LIST:
					UIERRCHK(GetTableCellVal(panel, PANEL_MOD_LIST, MakePoint(1,eventData1), modeSettings[eventData1].label));
					if (eventData1 == sourceSettings.activeMode)
						strncpy(modeSettings[0].label, modeSettings[eventData1].label, LABEL_SZ);
					break;
				case PANEL_MOD_OK:
					UIERRCHK(RemovePopup(0));
					UIERRCHK(panel = GetActivePanel());
					UpdatePanelModes(panel);
					break;
			}
			break;
	}
	return 0;
}

int CVICALLBACK SetActiveMode (int panel, int control, int event,
								  void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			switch (control) {
				case PANEL_ACTIVE_MODE:
					UIERRCHK(GetCtrlVal(panel, control, &sourceSettings.activeMode));
					modeSettings[0] = modeSettings[sourceSettings.activeMode];
					ProgramState savedProgramState = programState;
					programState = STATE_SWITCHING_MODE;
					UpdatePanel(panel);
					for (int i = 0; i < DADSS_CHANNELS; ++i)
						DSSERRCHK(DADSS_SetMDAC2(i+1, modeSettings[0].channelSettings[i].mdac2Code));
					DSSERRCHK(DADSS_UpdateMDAC2());
					for (int i = 0; i < DADSS_CHANNELS; ++i) {
						DSSERRCHK(DADSS_SetAmplitude(i+1, modeSettings[0].channelSettings[i].amplitude));
						DSSERRCHK(DADSS_SetPhase(i+1, modeSettings[0].channelSettings[i].phase));
					}
					DSSERRCHK(DADSS_UpdateWaveform());
					Delay(DADSS_ADJ_DELAY);
					for (int i = 0; i < DADSS_CHANNELS; ++i) {
						DSSERRCHK(DADSS_GetAmplitude(i+1, &modeSettings[0].channelSettings[i].amplitude));
						DSSERRCHK(DADSS_GetPhase(i+1, &modeSettings[0].channelSettings[i].phase));
					}
					GPIBERRCHK(SetLockinInputRaw(lockinSettings.lockinDesc, modeSettings[0].channelSettings[sourceSettings.activeChannel].lockinInputSettings));     
					programState = savedProgramState; 
					UpdatePanel(panel);
					break;
				case PANEL_SET_MODE:
					modeSettings[sourceSettings.activeMode] = modeSettings[0];
					break;
			}
			break;
	}

Error:
	return 0;
}

int CVICALLBACK SwapActiveChannel (int panel, int control, int event,
							  		void *callbackData, int eventData1, int eventData2)
{
	int swapActiveChannelPanel; 
	switch (event)
	{
		case EVENT_COMMIT:
			UIERRCHK(swapActiveChannelPanel = LoadPanel(0, panelsFile, PANEL_SWAP));
			UIERRCHK(SetTableColumnAttribute (swapActiveChannelPanel, PANEL_SWAP_LIST, 1, ATTR_MAX_ENTRY_LENGTH, LABEL_SZ-1));
			UIERRCHK(InsertTableRows(swapActiveChannelPanel, PANEL_SWAP_LIST, -1, DADSS_CHANNELS, VAL_CELL_STRING));
			for (int i = 0; i < DADSS_CHANNELS; ++i)
				UIERRCHK(SetTableCellVal(swapActiveChannelPanel, PANEL_SWAP_LIST, MakePoint(1,i+1), sourceSettings.label[i]));
			SetActiveTableCell (swapActiveChannelPanel, PANEL_SWAP_LIST, MakePoint(1, sourceSettings.activeChannel+1));
			UIERRCHK(SetCtrlAttribute(swapActiveChannelPanel, PANEL_SWAP_LIST, ATTR_ENABLE_COLUMN_SIZING, 0));
			UIERRCHK(SetCtrlAttribute(swapActiveChannelPanel, PANEL_SWAP_LIST, ATTR_ENABLE_ROW_SIZING, 0));
			UIERRCHK(InstallPopup(swapActiveChannelPanel));
			break;
		default:
			break;
	}
	return 0;
}

int CVICALLBACK SetSwapDestinationChannel (int panel, int control, int event,
							  				void *callbackData, int eventData1, int eventData2)
{
	Point activeTableCell;
	int swapDestinationChannel;
	ChannelSettings channelSettingsTmp;
	
	switch (event)
	{
		case EVENT_COMMIT:
			switch (control) {
				case PANEL_SWAP_OK:
					UIERRCHK(GetActiveTableCell (panel, PANEL_SWAP_LIST, &activeTableCell));
					swapDestinationChannel = activeTableCell.y-1;
					UIERRCHK(RemovePopup(0));
					if (swapDestinationChannel != sourceSettings.activeChannel) {
						channelSettingsTmp = modeSettings[0].channelSettings[sourceSettings.activeChannel];
						modeSettings[0].channelSettings[sourceSettings.activeChannel] = modeSettings[0].channelSettings[swapDestinationChannel];
						modeSettings[0].channelSettings[swapDestinationChannel] = channelSettingsTmp;
						DSSERRCHK(DADSS_SetMDAC2(sourceSettings.activeChannel+1, modeSettings[0].channelSettings[sourceSettings.activeChannel].mdac2Code));
						DSSERRCHK(DADSS_SetMDAC2(swapDestinationChannel+1, modeSettings[0].channelSettings[swapDestinationChannel].mdac2Code));
						DSSERRCHK(DADSS_UpdateMDAC2());
						DSSERRCHK(DADSS_SetAmplitude(sourceSettings.activeChannel+1, modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude));
						DSSERRCHK(DADSS_SetPhase(sourceSettings.activeChannel+1, modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
						DSSERRCHK(DADSS_SetAmplitude(swapDestinationChannel+1, modeSettings[0].channelSettings[swapDestinationChannel].amplitude));
						DSSERRCHK(DADSS_SetPhase(swapDestinationChannel+1, modeSettings[0].channelSettings[swapDestinationChannel].phase));
						DSSERRCHK(DADSS_UpdateWaveform());
						Delay(DADSS_ADJ_DELAY);
						DSSERRCHK(DADSS_GetAmplitude(sourceSettings.activeChannel+1, &modeSettings[0].channelSettings[sourceSettings.activeChannel].amplitude));
						DSSERRCHK(DADSS_GetPhase(sourceSettings.activeChannel+1, &modeSettings[0].channelSettings[sourceSettings.activeChannel].phase));
						DSSERRCHK(DADSS_GetAmplitude(swapDestinationChannel+1, &modeSettings[0].channelSettings[swapDestinationChannel].amplitude));
						DSSERRCHK(DADSS_GetPhase(swapDestinationChannel+1, &modeSettings[0].channelSettings[swapDestinationChannel].phase));
					}
					UIERRCHK(panel = GetActivePanel());
					UpdatePanelActiveChannel(panel);
					break;
				case PANEL_SWAP_CANCEL:
					UIERRCHK(RemovePopup(0));
					break;

			}
			break;
	}

Error:
	return 0;
}

int CVICALLBACK SetBridge (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	BridgeSettings bridgeSettingsTmp;
	
	switch (event) {
		case EVENT_COMMIT:
			switch (control) {
				case PANEL_BRI_OK:
					UIERRCHK(GetCtrlVal(panel, PANEL_BRI_VOLTAGE_CHANNEL_A, &bridgeSettingsTmp.channelAssignment[VOLTAGE_CHANNEL_A]));
					--bridgeSettingsTmp.channelAssignment[VOLTAGE_CHANNEL_A];
					UIERRCHK(GetCtrlVal(panel, PANEL_BRI_CURRENT_CHANNEL_A, &bridgeSettingsTmp.channelAssignment[CURRENT_CHANNEL_A]));
					--bridgeSettingsTmp.channelAssignment[CURRENT_CHANNEL_A];
					UIERRCHK(GetCtrlVal(panel, PANEL_BRI_VOLTAGE_CHANNEL_B, &bridgeSettingsTmp.channelAssignment[VOLTAGE_CHANNEL_B]));
					--bridgeSettingsTmp.channelAssignment[VOLTAGE_CHANNEL_B];
					UIERRCHK(GetCtrlVal(panel, PANEL_BRI_CURRENT_CHANNEL_B, &bridgeSettingsTmp.channelAssignment[CURRENT_CHANNEL_B]));
					--bridgeSettingsTmp.channelAssignment[CURRENT_CHANNEL_B];
					UIERRCHK(GetCtrlVal(panel, PANEL_BRI_VOLTAGE_RESISTANCE_A, &bridgeSettingsTmp.seriesResistance[VOLTAGE_CHANNEL_A]));
					UIERRCHK(GetCtrlVal(panel, PANEL_BRI_CURRENT_RESISTANCE_A, &bridgeSettingsTmp.seriesResistance[CURRENT_CHANNEL_A]));
					UIERRCHK(GetCtrlVal(panel, PANEL_BRI_VOLTAGE_RESISTANCE_B, &bridgeSettingsTmp.seriesResistance[VOLTAGE_CHANNEL_B]));
					UIERRCHK(GetCtrlVal(panel, PANEL_BRI_CURRENT_RESISTANCE_B, &bridgeSettingsTmp.seriesResistance[CURRENT_CHANNEL_B]));
					if (bridgeSettingsTmp.channelAssignment[VOLTAGE_CHANNEL_A] == bridgeSettingsTmp.channelAssignment[CURRENT_CHANNEL_A] ||
							bridgeSettingsTmp.channelAssignment[VOLTAGE_CHANNEL_A] == bridgeSettingsTmp.channelAssignment[VOLTAGE_CHANNEL_B] ||
							bridgeSettingsTmp.channelAssignment[VOLTAGE_CHANNEL_A] == bridgeSettingsTmp.channelAssignment[CURRENT_CHANNEL_B] ||
							bridgeSettingsTmp.channelAssignment[CURRENT_CHANNEL_A] == bridgeSettingsTmp.channelAssignment[VOLTAGE_CHANNEL_B] ||
							bridgeSettingsTmp.channelAssignment[CURRENT_CHANNEL_A] == bridgeSettingsTmp.channelAssignment[CURRENT_CHANNEL_B] ||
							bridgeSettingsTmp.channelAssignment[VOLTAGE_CHANNEL_B] == bridgeSettingsTmp.channelAssignment[CURRENT_CHANNEL_B]) {
								warn("%s.", msgStrings[MSG_EQUAL_CHANNELS]);
					} else {
						bridgeSettings = bridgeSettingsTmp;
						UIERRCHK(RemovePopup(0));
					}
					break;
				case PANEL_BRI_CANCEL:
					UIERRCHK(RemovePopup(0));
					break;
			}
			break;
	}
	return 0;
}

int CVICALLBACK PresetBridgeParameters (int panel, int control, int event,
										void *callbackData, int eventData1, int eventData2)
{
	ImpedanceType impedanceTypeA, impedanceTypeB;
	double RA = 0, tauA = 0, RB = 0, tauB = 0;
	double CA = 0, DA = 0, CB = 0, DB = 0;
	double LA = 0, XA = 0, LB = 0, XB = 0; 
	double GA = 0, BA = 0, GB = 0, BB = 0;
	double impedanceAMagnitude, impedanceAPhase, impedanceBMagnitude, impedanceBPhase;
	double impedanceALoadedMagnitude, impedanceALoadedPhase, impedanceBLoadedMagnitude, impedanceBLoadedPhase;
	double rmsCurrent, currentAmplitude, currentPhase;
	SourceSettings sourceSettingsTmp = sourceSettings;
	ModeSettings modeSettingsTmp = modeSettings[0];
	
	switch (event) {
		case EVENT_COMMIT:
			switch (control) {
				case PANEL_PRE_IMPEDANCE_A:
					UIERRCHK(GetCtrlVal(panel, control, (int *) &impedanceTypeA));
					switch (impedanceTypeA)
					{
						case RESISTANCE:
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_PRIMARY_A, ATTR_LABEL_TEXT, "R ="));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_A, ATTR_LABEL_TEXT, "tau ="));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_PRIMARY_A, "ohm"));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_SECONDARY_A, "second"));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_A, ATTR_MIN_VALUE, -INFINITY));
							break;
						case CAPACITANCE:
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_PRIMARY_A, ATTR_LABEL_TEXT, "C ="));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_A, ATTR_LABEL_TEXT, "D ="));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_PRIMARY_A, "farad"));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_SECONDARY_A, ""));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_A, ATTR_MIN_VALUE, 0.0));
							break;
						case INDUCTANCE:
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_PRIMARY_A, ATTR_LABEL_TEXT, "L ="));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_A, ATTR_LABEL_TEXT, "R ="));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_PRIMARY_A, "henry"));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_SECONDARY_A, "ohm"));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_A, ATTR_MIN_VALUE, 0.0));
							break;
						case IMPEDANCE:
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_PRIMARY_A, ATTR_LABEL_TEXT, "R ="));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_A, ATTR_LABEL_TEXT, "X ="));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_PRIMARY_A, "ohm"));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_SECONDARY_A, "ohm"));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_A, ATTR_MIN_VALUE, -INFINITY));
							break;
						case ADMITTANCE:
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_PRIMARY_A, ATTR_LABEL_TEXT, "G ="));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_A, ATTR_LABEL_TEXT, "B ="));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_PRIMARY_A, "siemens"));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_SECONDARY_A, "siemens"));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_A, ATTR_MIN_VALUE, -INFINITY));
							break;
					}
					break;
				case PANEL_PRE_IMPEDANCE_B:
					UIERRCHK(GetCtrlVal(panel, control, (int *) &impedanceTypeB));
					switch (impedanceTypeB)
					{
						case RESISTANCE:
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_PRIMARY_B, ATTR_LABEL_TEXT, "R ="));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_B, ATTR_LABEL_TEXT, "tau ="));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_PRIMARY_B, "ohm"));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_SECONDARY_B, "second"));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_B, ATTR_MIN_VALUE, -INFINITY));
							break;
						case CAPACITANCE:
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_PRIMARY_B, ATTR_LABEL_TEXT, "C ="));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_B, ATTR_LABEL_TEXT, "D ="));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_PRIMARY_B, "farad"));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_SECONDARY_B, ""));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_B, ATTR_MIN_VALUE, 0.0));
							break;
						case INDUCTANCE:
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_PRIMARY_B, ATTR_LABEL_TEXT, "L ="));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_B, ATTR_LABEL_TEXT, "R ="));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_PRIMARY_B, "henry"));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_SECONDARY_B, "ohm"));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_B, ATTR_MIN_VALUE, 0.0));
							break;
						case IMPEDANCE:
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_PRIMARY_B, ATTR_LABEL_TEXT, "R ="));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_B, ATTR_LABEL_TEXT, "X ="));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_PRIMARY_B, "ohm"));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_SECONDARY_B, "ohm"));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_B, ATTR_MIN_VALUE, -INFINITY));
							break;
						case ADMITTANCE:
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_PRIMARY_B, ATTR_LABEL_TEXT, "G ="));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_B, ATTR_LABEL_TEXT, "B ="));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_PRIMARY_B, "siemens"));
							UIERRCHK(SetCtrlVal(panel, PANEL_PRE_UNIT_SECONDARY_B, "siemens"));
							UIERRCHK(SetCtrlAttribute(panel, PANEL_PRE_SECONDARY_B, ATTR_MIN_VALUE, -INFINITY));
							break;
					}
					break;
				case PANEL_PRE_SET:
					UIERRCHK(GetCtrlVal(panel, PANEL_PRE_IMPEDANCE_A, (int *) &impedanceTypeA));
					switch (impedanceTypeA)
					{
						case RESISTANCE:
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_PRIMARY_A, &RA));
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_SECONDARY_A, &tauA));
							XA = 2.0*PI*sourceSettings.realFrequency*tauA*RA;
							break;
						case CAPACITANCE:
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_PRIMARY_A, &CA));
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_SECONDARY_A, &DA));
							BA = 2.0*PI*sourceSettings.realFrequency*CA;
							GA = BA*DA;
							CxDiv(1.0, 0.0, GA, BA, &RA, &XA);
							break;
						case INDUCTANCE:
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_PRIMARY_A, &LA));
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_SECONDARY_A, &RA));
							XA = 2.0*PI*sourceSettings.realFrequency*LA;
							break;
						case IMPEDANCE:
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_PRIMARY_A, &RA));
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_SECONDARY_A, &XA));
							break;
						case ADMITTANCE:
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_PRIMARY_A, &GA));
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_SECONDARY_A, &BA));
							CxDiv(1.0, 0.0, GA, BA, &RA, &XA);
							break;
					}
					UIERRCHK(GetCtrlVal(panel, PANEL_PRE_IMPEDANCE_B, (int *) &impedanceTypeB));
					switch (impedanceTypeB)
					{
						case RESISTANCE:
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_PRIMARY_B, &RB));
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_SECONDARY_B, &tauB));
							XB = 2.0*PI*sourceSettings.realFrequency*tauB*RB;
							break;
						case CAPACITANCE:
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_PRIMARY_B, &CB));
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_SECONDARY_B, &DB));
							BB = 2.0*PI*sourceSettings.realFrequency*CB;
							GB = BB*DB;
							CxDiv(1.0, 0.0, GB, BB, &RB, &XB);
							break;
						case INDUCTANCE:
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_PRIMARY_B, &LB));
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_SECONDARY_B, &RB));
							XB = 2.0*PI*sourceSettings.realFrequency*LB;
							break;
						case IMPEDANCE:
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_PRIMARY_B, &RB));
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_SECONDARY_B, &XB));
							break;
						case ADMITTANCE:
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_PRIMARY_B, &GB));
							UIERRCHK(GetCtrlVal(panel, PANEL_PRE_SECONDARY_B, &BB));
							CxDiv(1.0, 0.0, GB, BB, &RB, &XB);
							break;
					}
					ToPolar(RA, XA, &impedanceAMagnitude, &impedanceAPhase);
					ToPolar(RB, XB, &impedanceBMagnitude, &impedanceBPhase);
					ToPolar(RA+bridgeSettings.seriesResistance[CURRENT_CHANNEL_A], XA, &impedanceALoadedMagnitude, &impedanceALoadedPhase);
					ToPolar(RB+bridgeSettings.seriesResistance[CURRENT_CHANNEL_B], XB, &impedanceBLoadedMagnitude, &impedanceBLoadedPhase);
					
					UIERRCHK(GetCtrlVal(panel, PANEL_PRE_RMS_CURRENT, &rmsCurrent));
					currentAmplitude = rmsCurrent*sqrt(2.0);
					currentPhase = -(impedanceAPhase+impedanceBPhase+PI)/2;
					
					modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[VOLTAGE_CHANNEL_A]].amplitude = \
							impedanceAMagnitude*currentAmplitude;
					modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[VOLTAGE_CHANNEL_A]].phase = \
							impedanceAPhase+currentPhase;
					modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[VOLTAGE_CHANNEL_B]].amplitude = \
							impedanceBMagnitude*currentAmplitude;
					modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[VOLTAGE_CHANNEL_B]].phase = \
							impedanceBPhase+currentPhase+PI;
					
					modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[CURRENT_CHANNEL_A]].amplitude = \
							impedanceALoadedMagnitude*currentAmplitude;
					modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[CURRENT_CHANNEL_A]].phase = \
							impedanceALoadedPhase+currentPhase;
					modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[CURRENT_CHANNEL_B]].amplitude = \
							impedanceBLoadedMagnitude*currentAmplitude;
					modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[CURRENT_CHANNEL_B]].phase = \
							impedanceBLoadedPhase+currentPhase+PI;
					
					DADSS_RangeList tmpRangeA = DADSS_GetMinimumRange(modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[VOLTAGE_CHANNEL_A]].amplitude);
					DADSS_RangeList tmpRangeB = DADSS_GetMinimumRange(modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[VOLTAGE_CHANNEL_B]].amplitude);
					sourceSettingsTmp.range[bridgeSettings.channelAssignment[VOLTAGE_CHANNEL_A]] = (tmpRangeA > tmpRangeB) ? tmpRangeA : tmpRangeB;
					sourceSettingsTmp.range[bridgeSettings.channelAssignment[VOLTAGE_CHANNEL_B]] = sourceSettingsTmp.range[bridgeSettings.channelAssignment[VOLTAGE_CHANNEL_A]];
					
					tmpRangeA = DADSS_GetMinimumRange(modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[CURRENT_CHANNEL_A]].amplitude);
					tmpRangeB = DADSS_GetMinimumRange(modeSettingsTmp.channelSettings[bridgeSettings.channelAssignment[CURRENT_CHANNEL_B]].amplitude);
					sourceSettingsTmp.range[bridgeSettings.channelAssignment[CURRENT_CHANNEL_A]] = (tmpRangeA > tmpRangeB) ? tmpRangeA : tmpRangeB;
					sourceSettingsTmp.range[bridgeSettings.channelAssignment[CURRENT_CHANNEL_B]] = sourceSettingsTmp.range[bridgeSettings.channelAssignment[CURRENT_CHANNEL_A]];

					for (int i = 0; i < MAIN_CHANNEL_COUNT; ++i) {
						if (sourceSettingsTmp.range[bridgeSettings.channelAssignment[i]] == DADSS_OVERRANGE) {
							warn("%s.", msgStrings[MSG_PRESET_OVERRANGE]);
							return 0;
						}
					}
					
					sourceSettings = sourceSettingsTmp;
					modeSettings[0] = modeSettingsTmp;
					
					for (int i = 0; i < DADSS_CHANNELS; ++i)
						DSSERRCHK(DADSS_SetRange(i+1, sourceSettings.range[i]));
					DSSERRCHK(DADSS_UpdateConfiguration());
					
					for (int i = 0; i < DADSS_CHANNELS; ++i) {
						DSSERRCHK(DADSS_SetRange(i+1, sourceSettings.range[i]));
						DSSERRCHK(DADSS_SetAmplitude(i+1, modeSettings[0].channelSettings[i].amplitude));
						DSSERRCHK(DADSS_SetPhase(i+1, modeSettings[0].channelSettings[i].phase));
					}
					DSSERRCHK(DADSS_UpdateWaveform());
				
					Delay(DADSS_ADJ_DELAY);
					for (int i = 0; i < DADSS_CHANNELS; ++i) {
						DSSERRCHK(DADSS_GetAmplitude(i+1, &modeSettings[0].channelSettings[i].amplitude));
					   	DSSERRCHK(DADSS_GetPhase(i+1, &modeSettings[0].channelSettings[i].phase));
						ToRect(modeSettings[0].channelSettings[i].amplitude, 
							   modeSettings[0].channelSettings[i].phase, 
							   &modeSettings[0].channelSettings[i].real, 
							   &modeSettings[0].channelSettings[i].imag);
					} 
					
					int mainPanel = (int)callbackData;
					UpdatePanelActiveChannel(mainPanel);
					UIERRCHK(RemovePopup(0)); 
					break;
				case PANEL_PRE_CANCEL:
					UIERRCHK(RemovePopup(0));
					break;
			}
			break;
	}
	
Error:
	return 0;
}
