/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL                            1       /* callback function: ManagePanel */
#define  PANEL_RANGE                      2       /* control type: slide, callback function: SetRange */
#define  PANEL_ACTIVE_CHANNEL             3       /* control type: ring, callback function: SetActiveChannel */
#define  PANEL_BALANCE_THRESHOLD          4       /* control type: numeric, callback function: SetBalanceThreshold */
#define  PANEL_LOCKIN_Y                   5       /* control type: numeric, callback function: (none) */
#define  PANEL_REAL_FREQUENCY             6       /* control type: numeric, callback function: (none) */
#define  PANEL_LOCKIN_TIME_CONSTANT       7       /* control type: numeric, callback function: (none) */
#define  PANEL_LOCKIN_X                   8       /* control type: numeric, callback function: (none) */
#define  PANEL_LOCKIN_ADJ_DELAY           9       /* control type: numeric, callback function: (none) */
#define  PANEL_CLOCKFREQUENCY             10      /* control type: numeric, callback function: SetFrequency */
#define  PANEL_FREQUENCY                  11      /* control type: numeric, callback function: SetFrequency */
#define  PANEL_PHASE                      12      /* control type: numeric, callback function: SetWaveformParameters */
#define  PANEL_MDAC2_VAL                  13      /* control type: numeric, callback function: SetWaveformParameters */
#define  PANEL_MDAC2_CODE                 14      /* control type: numeric, callback function: SetWaveformParameters */
#define  PANEL_IMAG                       15      /* control type: numeric, callback function: SetWaveformParameters */
#define  PANEL_REAL                       16      /* control type: numeric, callback function: SetWaveformParameters */
#define  PANEL_AMPLITUDE                  17      /* control type: numeric, callback function: SetWaveformParameters */
#define  PANEL_LOCKIN_READ                18      /* control type: command, callback function: ReadLockin */
#define  PANEL_COPY_REAL_FREQUENCY        19      /* control type: command, callback function: CopyWaveformParameters */
#define  PANEL_COPY_SAMPLES               20      /* control type: command, callback function: CopyWaveformParameters */
#define  PANEL_COPY_FFT                   21      /* control type: command, callback function: CopyWaveformParameters */
#define  PANEL_PASTE_PHASOR               22      /* control type: command, callback function: SetWaveformParameters */
#define  PANEL_COPY_PHASOR                23      /* control type: command, callback function: CopyWaveformParameters */
#define  PANEL_AUTOZERO                   24      /* control type: command, callback function: AutoZero */
#define  PANEL_OUT_OF_RANGE_LED           25      /* control type: LED, callback function: (none) */
#define  PANEL_LOCKIN_FLOATING            26      /* control type: radioButton, callback function: SetLockinInput */
#define  PANEL_LOCKIN_COUPLING_AC         27      /* control type: radioButton, callback function: SetLockinInput */
#define  PANEL_AUTOZERO_LED               28      /* control type: LED, callback function: (none) */
#define  PANEL_DECORATION_7               29      /* control type: deco, callback function: (none) */
#define  PANEL_DECORATION_5               30      /* control type: deco, callback function: (none) */
#define  PANEL_DECORATION_6               31      /* control type: deco, callback function: (none) */
#define  PANEL_DECORATION_4               32      /* control type: deco, callback function: (none) */
#define  PANEL_START_STOP                 33      /* control type: command, callback function: StartStop */
#define  PANEL_START_STOP_LED             34      /* control type: LED, callback function: (none) */
#define  PANEL_CONNECT                    35      /* control type: command, callback function: Connect */
#define  PANEL_CONNECT_LED                36      /* control type: LED, callback function: (none) */
#define  PANEL_TEXTMSG_11                 37      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_6                  38      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_10                 39      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_5                  40      /* control type: textMsg, callback function: (none) */
#define  PANEL_IS_LOCKED                  41      /* control type: radioButton, callback function: ToggleLock */
#define  PANEL_TEXTMSG_9                  42      /* control type: textMsg, callback function: (none) */
#define  PANEL_LABEL                      43      /* control type: string, callback function: SetLabel */
#define  PANEL_TEXTMSG                    44      /* control type: textMsg, callback function: (none) */
#define  PANEL_PHASE_ADD_PIHALF           45      /* control type: command, callback function: SetWaveformParameters */
#define  PANEL_PHASE_SUBTRACT_PIHALF      46      /* control type: command, callback function: SetWaveformParameters */
#define  PANEL_PHASE_ADD_PI               47      /* control type: command, callback function: SetWaveformParameters */
#define  PANEL_PHASE_SUBTRACT_PI          48      /* control type: command, callback function: SetWaveformParameters */
#define  PANEL_ACTIVE_MODE                49      /* control type: ring, callback function: SetActiveMode */
#define  PANEL_LOCKIN_FILTERS_TYPE        50      /* control type: slide, callback function: SetLockinInput */
#define  PANEL_LOCKIN_INPUT_TYPE          51      /* control type: slide, callback function: SetLockinInput */
#define  PANEL_LOCKIN_RESERVE_TYPE        52      /* control type: slide, callback function: SetLockinInput */
#define  PANEL_LOCKIN_GAIN_TYPE           53      /* control type: slide, callback function: SetLockinInput */
#define  PANEL_SWAP_ACTIVE_CHANNEL        54      /* control type: command, callback function: SwapActiveChannel */
#define  PANEL_VERSICAL_LOGO              55      /* control type: picture, callback function: (none) */
#define  PANEL_SET_MODE                   56      /* control type: command, callback function: SetActiveMode */
#define  PANEL_TEXTMSG_8                  57      /* control type: textMsg, callback function: (none) */

#define  PANEL_BRI                        2
#define  PANEL_BRI_CURRENT_CHANNEL_B      2       /* control type: numeric, callback function: (none) */
#define  PANEL_BRI_OK                     3       /* control type: command, callback function: SetBridge */
#define  PANEL_BRI_CANCEL                 4       /* control type: command, callback function: SetBridge */
#define  PANEL_BRI_CURRENT_RESISTANCE_B   5       /* control type: numeric, callback function: (none) */
#define  PANEL_BRI_VOLTAGE_CHANNEL_B      6       /* control type: numeric, callback function: (none) */
#define  PANEL_BRI_VOLTAGE_RESISTANCE_B   7       /* control type: numeric, callback function: (none) */
#define  PANEL_BRI_CURRENT_CHANNEL_A      8       /* control type: numeric, callback function: (none) */
#define  PANEL_BRI_CURRENT_RESISTANCE_A   9       /* control type: numeric, callback function: (none) */
#define  PANEL_BRI_VOLTAGE_CHANNEL_A      10      /* control type: numeric, callback function: (none) */
#define  PANEL_BRI_VOLTAGE_RESISTANCE_A   11      /* control type: numeric, callback function: (none) */
#define  PANEL_BRI_TEXTMSG                12      /* control type: textMsg, callback function: (none) */

#define  PANEL_CON1                       3
#define  PANEL_CON1_PROGRESSBAR           2       /* control type: scale, callback function: (none) */

#define  PANEL_CON2                       4
#define  PANEL_CON2_NV_SERVER             2       /* control type: ring, callback function: (none) */
#define  PANEL_CON2_LOCKIN_GPIB_ADDRESS   3       /* control type: numeric, callback function: (none) */
#define  PANEL_CON2_CANCEL                4       /* control type: command, callback function: SetConnection */
#define  PANEL_CON2_OK                    5       /* control type: command, callback function: SetConnection */

#define  PANEL_INFO                       5       /* callback function: ClosePanelInfo */
#define  PANEL_INFO_PICTURE               2       /* control type: picture, callback function: (none) */

#define  PANEL_MOD                        6
#define  PANEL_MOD_ADD                    2       /* control type: command, callback function: SetModes */
#define  PANEL_MOD_OK                     3       /* control type: command, callback function: SetModes */
#define  PANEL_MOD_DUPLICATE              4       /* control type: command, callback function: SetModes */
#define  PANEL_MOD_REMOVE                 5       /* control type: command, callback function: SetModes */
#define  PANEL_MOD_LIST                   6       /* control type: table, callback function: SetModes */

#define  PANEL_PRE                        7
#define  PANEL_PRE_IMPEDANCE_B            2       /* control type: ring, callback function: PresetBridgeParameters */
#define  PANEL_PRE_SET                    3       /* control type: command, callback function: PresetBridgeParameters */
#define  PANEL_PRE_CANCEL                 4       /* control type: command, callback function: PresetBridgeParameters */
#define  PANEL_PRE_SECONDARY_B            5       /* control type: numeric, callback function: (none) */
#define  PANEL_PRE_UNIT_SECONDARY_B       6       /* control type: textMsg, callback function: (none) */
#define  PANEL_PRE_IMPEDANCE_A            7       /* control type: ring, callback function: PresetBridgeParameters */
#define  PANEL_PRE_RMS_CURRENT            8       /* control type: numeric, callback function: (none) */
#define  PANEL_PRE_PRIMARY_B              9       /* control type: numeric, callback function: (none) */
#define  PANEL_PRE_UNIT_PRIMARY_B         10      /* control type: textMsg, callback function: (none) */
#define  PANEL_PRE_SECONDARY_A            11      /* control type: numeric, callback function: (none) */
#define  PANEL_PRE_UNIT_SECONDARY_A       12      /* control type: textMsg, callback function: (none) */
#define  PANEL_PRE_TEXTMSG_2              13      /* control type: textMsg, callback function: (none) */
#define  PANEL_PRE_PRIMARY_A              14      /* control type: numeric, callback function: (none) */
#define  PANEL_PRE_UNIT_PRIMARY_A         15      /* control type: textMsg, callback function: (none) */
#define  PANEL_PRE_TEXTMSG                16      /* control type: textMsg, callback function: (none) */
#define  PANEL_PRE_TEXTMSG_3              17      /* control type: textMsg, callback function: (none) */

#define  PANEL_S                          8
#define  PANEL_S_PROGRESSBAR              2       /* control type: scale, callback function: (none) */
#define  PANEL_S_INTERRUPT                3       /* control type: command, callback function: (none) */

#define  PANEL_SWAP                       9
#define  PANEL_SWAP_LIST                  2       /* control type: table, callback function: (none) */
#define  PANEL_SWAP_OK                    3       /* control type: command, callback function: SetSwapDestinationChannel */
#define  PANEL_SWAP_CANCEL                4       /* control type: command, callback function: SetSwapDestinationChannel */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

#define  MENUBAR                          1
#define  MENUBAR_FILE                     2
#define  MENUBAR_FILE_NEW                 3       /* callback function: FileNew */
#define  MENUBAR_FILE_SAVE                4       /* callback function: FileSave */
#define  MENUBAR_FILE_CLOSE               5       /* callback function: FileClose */
#define  MENUBAR_FILE_SEPARATOR_3         6
#define  MENUBAR_FILE_EXIT                7       /* callback function: FileExit */
#define  MENUBAR_SETTINGS                 8
#define  MENUBAR_SETTINGS_CONNECTION      9       /* callback function: SettingsMenu */
#define  MENUBAR_SETTINGS_SEPARATOR_4     10
#define  MENUBAR_SETTINGS_MODES           11      /* callback function: SettingsMenu */
#define  MENUBAR_SETTINGS_BRIDGE          12      /* callback function: SettingsMenu */
#define  MENUBAR_SETTINGS_PRESET          13      /* callback function: SettingsMenu */
#define  MENUBAR_SETTINGS_SEPARATOR_2     14
#define  MENUBAR_SETTINGS_LOAD            15      /* callback function: SettingsMenu */
#define  MENUBAR_SETTINGS_SAVE            16      /* callback function: SettingsMenu */
#define  MENUBAR_SETTINGS_SEPARATOR       17
#define  MENUBAR_SETTINGS_RESET           18      /* callback function: SettingsMenu */


     /* Callback Prototypes: */

int  CVICALLBACK AutoZero(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ClosePanelInfo(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Connect(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CopyWaveformParameters(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK FileClose(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK FileExit(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK FileNew(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK FileSave(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK ManagePanel(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK PresetBridgeParameters(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ReadLockin(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetActiveChannel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetActiveMode(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetBalanceThreshold(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetBridge(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetConnection(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetFrequency(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetLabel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetLockinInput(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetModes(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetRange(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetSwapDestinationChannel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK SettingsMenu(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK SetWaveformParameters(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK StartStop(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SwapActiveChannel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ToggleLock(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif