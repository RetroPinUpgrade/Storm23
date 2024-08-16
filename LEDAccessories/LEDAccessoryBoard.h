#define LED_UPDATE_FRAME_PERIOD_IN_MS   30

#define MACHINE_STATE_OFF             0
#define MACHINE_STATE_ATTRACT_MODE    1
#define MACHINE_STATE_GAME_MODE       2


#define SETTINGS_MODE_OFF          0

#ifdef LED_BACKLIGHT_INO
#endif


/***************************************************************
 *  These definitions are for the Accessory Lamp Board V1.0
 */
#define STRIP_1_CONTROL       30
#define STRIP_2_CONTROL       31
#define STRIP_3_CONTROL       29
#define STRIP_4_CONTROL       28
#define STRIP_5_CONTROL       27

#define NON_ADDRESSABLE_RGB_RED_PIN     8
#define NON_ADDRESSABLE_RGB_GREEN_PIN   9
#define NON_ADDRESSABLE_RGB_BLUE_PIN    10



/***************************************************************
 * These functions are supplied by the game file
 */
//int GetNumberOfLEDsInStrip();
boolean LightingNeedsToWatchI2C();
boolean LightingNeedsToCheckMachineFor5V();
boolean LightingNeedsToCheckMachineForFlipperActivity();
boolean ALBSupportsPlayfieldLamps();

byte IncomingI2CDeviceAddress();

void InitializeAllStrips();
void InitializeRGBValues();
boolean UpdateStripsBasedOnInputs(unsigned long  *lastInputSeenTime, unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime);
boolean UpdateRGBBasedOnInputs(unsigned long  *lastInputSeenTime, unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime);
boolean UpdateStripsBasedOnI2C(unsigned long lastMessageSeenTime, byte lastMessage, byte lastParameter, unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime);
boolean UpdateRGBBasedOnI2C(unsigned long lastMessageSeenTime, byte lastMessage, byte lastParameter, unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime);
boolean UpdateStripsBasedOnLampBus(unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime);

boolean ReadSettings();
boolean WriteSettings();
byte AdvanceSettingsMode(byte oldSettingsMode);
boolean ShowSettingsMode(byte settingsMode, unsigned long currentAnimationFrame);
boolean IncreaseBrightness(byte settingsMode);
boolean DecreaseBrightness(byte settingsMode);
