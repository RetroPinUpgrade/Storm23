#include "Arduino.h"
#define SMALLEST_CODESIZE
#include <ky-040.h>
#include "LEDAccessoryBoard.h"
#include "ALB-Communication.h"
#include <Adafruit_NeoPixel.h>
#include <math.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <EEPROM.h>

#define PIXELS_PER_ROW              13


// Fix tilt code

/***************************************************************
 *  Set up these definitions based on install
 */
#define STRIP_1_NUMBER_OF_PIXELS    52
#define STRIP_2_NUMBER_OF_PIXELS    0
#define STRIP_3_NUMBER_OF_PIXELS    0
#define STRIP_4_NUMBER_OF_PIXELS    0
#define STRIP_5_NUMBER_OF_PIXELS    0

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel StripOfLEDs1 = Adafruit_NeoPixel(STRIP_1_NUMBER_OF_PIXELS, STRIP_1_CONTROL, NEO_GRB + NEO_KHZ800);

// Connector 1 (pins from top to bottom)
// 1:  A12 (D7)  - 
// 2:  A11 (D3)  - 
// 3:  A10 (D8)  - 
// 4:  A9  (D4)  - 
// 5:  A8  (D9)  - 
// 6:  A7  (D5)  - 
// 7:  KEY
// 8:  A6  (D10) - 
// 9:  A5  (D6)  - 
//
// Connector 2  (pin from top to bottom)
// 1:  A4  (D11)  - 
// 2:  A3  (D14)  - 
// 3:  Key
// 4:  A2  (D12)  - 
// 5:  A1  (D15)  - 
// 6:  A0  (D13)  - 


// The LightingNeedsToWatchI2C() function should return either:
//    false = No, we're not expecting i2c communication
//    true  = Yes, watch i2c port for commands
boolean LightingNeedsToWatchI2C() {
  return true;
}

byte IncomingI2CDeviceAddress() {
  return 8;
}


// The LightingNeedsToCheckMachineFor5V() function should return either:
//    false = No, nothing is hooked to the 5V sensor
//    true  = Yes, check the 5V sensor to see if the machine is on
boolean LightingNeedsToCheckMachineFor5V() {
  return false;
}

// The LightingNeedsToCheckMachineForFlipperActivity() function should return either:
//    false = No, the flipper sensing lines shouldn't dictate the machine state
//    true  = Yes, flipper activity should tell us if the machine is in attract or game play
boolean LightingNeedsToCheckMachineForFlipperActivity() {
  return false;
}


void InitializeAllStrips() {
  StripOfLEDs1.begin();
  StripOfLEDs1.show(); // Initialize all pixels to 'off'
}

void InitializeRGBValues() {
  // not used in this game
  analogWrite(NON_ADDRESSABLE_RGB_RED_PIN, 0);
  analogWrite(NON_ADDRESSABLE_RGB_GREEN_PIN, 0);
  analogWrite(NON_ADDRESSABLE_RGB_BLUE_PIN, 0);
}


int AttractModeBrightness = 255;
int GameModeBrightness = 64;
int AnimationBrightness[16] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

#define EEPROM_CONFIRMATION_UL                0x4C454430
#define EEPROM_INIT_CONFIRMATION_ADDRESS      200
#define EEPROM_ATTRACT_BRIGHTNESS_ADDRESS     205
#define EEPROM_GAME_BRIGHTNESS_ADDRESS        206
#define EEPROM_INPUT_0_BRIGHTNESS_ADDRESS     210
#define EEPROM_INPUT_1_BRIGHTNESS_ADDRESS     211
#define EEPROM_INPUT_2_BRIGHTNESS_ADDRESS     212
#define EEPROM_INPUT_3_BRIGHTNESS_ADDRESS     213
#define EEPROM_INPUT_4_BRIGHTNESS_ADDRESS     214
#define EEPROM_INPUT_5_BRIGHTNESS_ADDRESS     215
#define EEPROM_INPUT_6_BRIGHTNESS_ADDRESS     216
#define EEPROM_INPUT_7_BRIGHTNESS_ADDRESS     217
#define EEPROM_INPUT_8_BRIGHTNESS_ADDRESS     218
#define EEPROM_INPUT_9_BRIGHTNESS_ADDRESS     219
#define EEPROM_INPUT_10_BRIGHTNESS_ADDRESS    220
#define EEPROM_INPUT_11_BRIGHTNESS_ADDRESS    221
#define EEPROM_INPUT_12_BRIGHTNESS_ADDRESS    222
#define EEPROM_INPUT_13_BRIGHTNESS_ADDRESS    223
#define EEPROM_INPUT_14_BRIGHTNESS_ADDRESS    224
#define EEPROM_INPUT_15_BRIGHTNESS_ADDRESS    225

boolean ReadSettings() {

  unsigned long testVal =   (((unsigned long)EEPROM.read(EEPROM_INIT_CONFIRMATION_ADDRESS+3))<<24) | 
                            ((unsigned long)(EEPROM.read(EEPROM_INIT_CONFIRMATION_ADDRESS+2))<<16) | 
                            ((unsigned long)(EEPROM.read(EEPROM_INIT_CONFIRMATION_ADDRESS+1))<<8) | 
                            ((unsigned long)(EEPROM.read(EEPROM_INIT_CONFIRMATION_ADDRESS)));
  if (testVal!=EEPROM_CONFIRMATION_UL) {
    // Write the defaults compiled above
    WriteSettings();
    EEPROM.write(EEPROM_INIT_CONFIRMATION_ADDRESS+3, EEPROM_CONFIRMATION_UL>>24);
    EEPROM.write(EEPROM_INIT_CONFIRMATION_ADDRESS+2, 0xFF & (EEPROM_CONFIRMATION_UL>>16));
    EEPROM.write(EEPROM_INIT_CONFIRMATION_ADDRESS+1, 0xFF & (EEPROM_CONFIRMATION_UL>>8));
    EEPROM.write(EEPROM_INIT_CONFIRMATION_ADDRESS+0, 0xFF & (EEPROM_CONFIRMATION_UL));
    //Serial.write("No Confirmation bytes: Inserting default values\n");
  } else {
    //Serial.write("Confirmation bytes okay\n");
  }               

  AttractModeBrightness = EEPROM.read(EEPROM_ATTRACT_BRIGHTNESS_ADDRESS);
  GameModeBrightness = EEPROM.read(EEPROM_GAME_BRIGHTNESS_ADDRESS);

  for (byte count=0; count<16; count++) {
    AnimationBrightness[count] = EEPROM.read(EEPROM_INPUT_0_BRIGHTNESS_ADDRESS+count);
  }

  return true;
}

boolean WriteSettings() {
  EEPROM.write(EEPROM_ATTRACT_BRIGHTNESS_ADDRESS, AttractModeBrightness);
  EEPROM.write(EEPROM_GAME_BRIGHTNESS_ADDRESS, GameModeBrightness);
  for (byte count=0; count<16; count++) {
    EEPROM.write(EEPROM_INPUT_0_BRIGHTNESS_ADDRESS+count, AnimationBrightness[count]);
  }

  return true;
}



/***************************************************************
 *  This function shows the general illumination version of the
 *  backlight (with no animation)
 */

#define MACHINE_STATE_TRANSITION_OFF_TO_ATTRACT      250
#define MACHINE_STATE_TRANSITION_ATTRACT_TO_GAME     251
#define MACHINE_STATE_TRANSITION_GAME_TO_ATTRACT     252

byte CurrentLightingState = MACHINE_STATE_OFF;
int CurrentBlendValue = 255;
int StartingBlendValue = 0;
unsigned long EffectStartTime = 0;
unsigned long TimeToReachNewState = 0;
unsigned long FlashyAttractTime = 0;
unsigned long AttractModeStabilizationTime = 0;





boolean RGBHasBeenShown = true;

boolean UpdateRGBBasedOnInputs(unsigned long  *lastInputSeenTime, unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime) {

  // This accessory doesn't use RGB lamps or inputs
  (void)lastInputSeenTime;
  (void)currentTime;
  (void)currentAnimationFrame;
  (void)machineState;
  (void)machineStateChangedTime;
  return false;
}


#define S23_ANIMATION_YELLOW_PULSE        0
#define S23_ANIMATION_CYAN_PULSE          1
#define S23_ANIMATION_GREEN_FLASH         2
#define S23_ANIMATION_PURPLE_LOOP         3
#define S23_ANIMATION_LTOR_BLUE           4
#define S23_ANIMATION_RTOL_BLUE           5
#define S23_ANIMATION_LEFT_FLASH          6
#define S23_ANIMATION_RIGHT_FLASH         7
#define S23_ANIMATION_SPARKLE             8
#define S23_ANIMATION_FIRE                9
#define S23_ANIMATION_LIGHTNING_1         10
#define S23_ANIMATION_LIGHTNING_2         11
#define S23_ANIMATION_LIGHTNING_3         12
#define S23_ANIMATION_LIGHTNING_4         13
#define S23_ANIMATION_LIGHTNING_5         14
#define S23_ANIMATION_LIGHTNING_6         15
#define S23_ANIMATION_LIGHTNING_7         16
#define S23_ANIMATION_LIGHTNING_8         17
#define S23_ANIMATION_LIGHTNING_9         18
#define S23_ANIMATION_LIGHTNING_10        19
#define S23_ANIMATION_LIGHTNING_11        20
#define S23_ANIMATION_LIGHTNING_12        21
#define S23_ANIMATION_CANDY_PULSE         22
#define S23_ANIMATION_BIG_LIGHTNING_1     23
#define S23_ANIMATION_BIG_LIGHTNING_2     24
#define S23_ANIMATION_DIM_LIGHTNING_1     25



boolean PulseColor(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>30) return false;

  int brightness = 255;
  if (animationFrame<=15) {
    brightness = 17 * animationFrame;
  } else {
    brightness = 255 - (17*animationFrame);
  }

  for (byte count=0; count<STRIP_1_NUMBER_OF_PIXELS; count++) {
    int adjRed = (brightness * red) / 255;
    int adjGreen = (brightness * green) / 255;
    int adjBlue = (brightness * blue) / 255;
    StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( adjRed, adjGreen, adjBlue ));
  }

  if (animationFrame==30) return false;

  return true;
}

byte LastPulsePolarity = 0;
boolean Pulse2Colors(int animationFrame, byte r1, byte g1, byte b1, byte r2, byte g2, byte b2) {
  if (animationFrame>30) {
    return false;
  }

  int brightness = 255;
  if (animationFrame<=15) {
    brightness = 17 * animationFrame;
  } else {
    brightness = 255 - (17*animationFrame);
  }

  if (LastPulsePolarity) {
    byte tempr = r1;
    byte tempg = g1;
    byte tempb = b1;
    r1 = r2;
    g1 = g2;
    b1 = b2;
    r2 = tempr;
    g2 = tempg;
    b2 = tempb;
  }

  for (byte count=0; count<STRIP_1_NUMBER_OF_PIXELS; count++) {
    if (count<(STRIP_1_NUMBER_OF_PIXELS/2)) {
      int adjRed = (brightness * r1) / 255;
      int adjGreen = (brightness * g1) / 255;
      int adjBlue = (brightness * b1) / 255;
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( adjRed, adjGreen, adjBlue ));
    } else {
      int adjRed = ((255-brightness) * r2) / 255;
      int adjGreen = ((255-brightness) * g2) / 255;
      int adjBlue = ((255-brightness) * b2) / 255;
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( adjRed, adjGreen, adjBlue ));
    }
  }

  if (animationFrame==30) {
    LastPulsePolarity ^= 1;
    return false;
  }
  return true;
}

int RandomFlashSeed = 0;
boolean RandomFlashColor(int animationFrame, byte red, byte green, byte blue, boolean flashSide) {

  if (animationFrame>30) return false;
  
  if (RandomFlashSeed==0) {
    RandomFlashSeed = (millis()%15);
  }

  boolean flash1On = false;
  boolean flash2On = false;
  if (animationFrame==RandomFlashSeed || animationFrame==(RandomFlashSeed + 6) || animationFrame==(RandomFlashSeed * 2)) {
    flash1On = true;
  }  
  if (animationFrame==(RandomFlashSeed+4) || animationFrame==(RandomFlashSeed + 9) || animationFrame==((RandomFlashSeed * 2)/3)) {
    flash2On = true;
  }

  for (byte count=0; count<STRIP_1_NUMBER_OF_PIXELS; count++) {
    if (count<(STRIP_1_NUMBER_OF_PIXELS/2)) {
      if ((flash1On && flashSide==0) || (flash2On && flashSide==1)) StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( red, green, blue ));
    } else {
      if ((flash2On && flashSide==0) || (flash1On && flashSide==1)) StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( red, green, blue ));
    }
  }

  if (animationFrame==30) {
    RandomFlashSeed = 0;
    return false;
  }

  return true;
}


boolean BigFlash(int animationFrame, byte red, byte green, byte blue, boolean longerFlash) {

  if (!longerFlash && animationFrame>45) return false;
  if (animationFrame>75) return false;
  
  if (RandomFlashSeed==0) {
    RandomFlashSeed = (millis()%10);
  }

  boolean flashBack = false;
  boolean flashFront = false;
  boolean flashLeft = false;
  boolean flashRight = false;

  if ( (animationFrame%12)==0 ) flashBack = true;
  if ( (animationFrame%17)==0 ) flashFront = true;
  if ( (animationFrame%13)==0 ) flashLeft = true;
  if ( (animationFrame%RandomFlashSeed)==0 ) flashRight = true;

  if (flashBack) {
    for (byte count=0; count<(STRIP_1_NUMBER_OF_PIXELS/2); count++) {
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( red, green, blue ));
    }
  } else if (flashFront) {
    for (byte count=0; count<(STRIP_1_NUMBER_OF_PIXELS/2); count++) {
      StripOfLEDs1.setPixelColor(count + (STRIP_1_NUMBER_OF_PIXELS/2), StripOfLEDs1.Color( red, green, blue ));
    }
  } else if (flashLeft) {
    for (byte count=0; count<(STRIP_1_NUMBER_OF_PIXELS/8); count++) {
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor((STRIP_1_NUMBER_OF_PIXELS/2 - 1) - count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(count + (STRIP_1_NUMBER_OF_PIXELS/2), StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor((STRIP_1_NUMBER_OF_PIXELS - 1) - count, StripOfLEDs1.Color( red, green, blue ));
    }
  } else if (flashRight) {
    for (byte count=0; count<(STRIP_1_NUMBER_OF_PIXELS/8); count++) {
      StripOfLEDs1.setPixelColor(STRIP_1_NUMBER_OF_PIXELS/8 + count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(STRIP_1_NUMBER_OF_PIXELS/4 + count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(STRIP_1_NUMBER_OF_PIXELS/2 + STRIP_1_NUMBER_OF_PIXELS/8 + count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(STRIP_1_NUMBER_OF_PIXELS/2 + STRIP_1_NUMBER_OF_PIXELS/4 + count, StripOfLEDs1.Color( red, green, blue ));
    }
  }


  if (!longerFlash && animationFrame==45) {
    RandomFlashSeed = 0;
    return false;
  } else if (longerFlash && animationFrame==75) {
    RandomFlashSeed = 0;
    return false;    
  }

  return true;
}

boolean LoopColor(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>19) return false;

  if (animationFrame<10) {
    for (byte count=0; count<10; count++) {
      if (animationFrame==count) {
        StripOfLEDs1.setPixelColor(count*2, StripOfLEDs1.Color( red, green, blue ));
        StripOfLEDs1.setPixelColor((count*2)+1, StripOfLEDs1.Color( red, green, blue ));
      }
    }
  } else {
    for (byte count=0; count<10; count++) {
      if ((animationFrame-10)==count) {
        StripOfLEDs1.setPixelColor(60 + count*2, StripOfLEDs1.Color( red, green, blue ));
        StripOfLEDs1.setPixelColor(61 + count*2, StripOfLEDs1.Color( red, green, blue ));
      }
    }
  }

  if (animationFrame==19) {
    return false;
  }
  return true;
}

boolean LeftToRightSweep(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>19) return false;

  for (byte count=0; count<20; count++) {
    if (animationFrame==count) {
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(39 - count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(count + 40, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(79 - count, StripOfLEDs1.Color( red, green, blue ));
    }
  }

  if (animationFrame==19) {
    return false;
  }
  return true;
}

boolean RightToLeftSweep(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>19) return false;

  for (byte count=0; count<20; count++) {
    if (animationFrame==count) {
      StripOfLEDs1.setPixelColor(19 - count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(20 + count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(59 - count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(60 + count, StripOfLEDs1.Color( red, green, blue ));
    }
  }
  if (animationFrame==19) {
    return false;
  }
  return true;
}

boolean LeftFlash(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>5) return false;

  if (animationFrame==0 || animationFrame==2) {
    for (byte count=0; count<8; count++) {
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(39 - count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(count + 40, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(79 - count, StripOfLEDs1.Color( red, green, blue ));
    }
  }

  if (animationFrame==5) {
    return false;
  }
  return true;
}

boolean RightFlash(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>5) return false;

  if (animationFrame==0 || animationFrame==2) {
    for (byte count=0; count<8; count++) {
      StripOfLEDs1.setPixelColor(19 - count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(20 + count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(59 - count, StripOfLEDs1.Color( red, green, blue ));
      StripOfLEDs1.setPixelColor(60 + count, StripOfLEDs1.Color( red, green, blue ));
    }
  }

  if (animationFrame==5) {
    return false;
  }
  return true;
}

boolean Sparkle(int animationFrame) {
  if (animationFrame>10) return false;

  for (byte count=0; count<80; count++) {
    byte brightness = micros()%255;
    if ((brightness%4)==0) StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( brightness, brightness, 0 ));
    else if ((brightness%4)==1) StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( 0, brightness/2, brightness ));
  }

  if (animationFrame==10) {
    return false;
  }
  return true;
}

boolean Fire(int animationFrame, byte red, byte green, byte blue, int redFactor, int greenFactor, int blueFactor) {
  if (animationFrame>60) return false;
  if (redFactor<1) redFactor = 1;
  if (greenFactor<1) greenFactor = 1;
  if (blueFactor<1) blueFactor = 1;

  for (byte count=0; count<80; count++) {
    int brightness = micros()%255;
    int newRed = ((brightness/redFactor) * red)/254;
    int newGreen = ((brightness/greenFactor) * green)/254;
    int newBlue = ((brightness/blueFactor) * blue)/254;
    StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( newRed, newGreen, newBlue ));
  }

  if (animationFrame==60) {
    return false;
  }
  return true;
}



unsigned long LoopStartTime = 0;
boolean PlayAnimation(byte animationNum, unsigned long animationStartTime, unsigned long currentTime) {

  int animationFrame = (currentTime-animationStartTime)/30;

  switch (animationNum) {
    case S23_ANIMATION_YELLOW_PULSE:
      return PulseColor(animationFrame, 0xFF, 0xFF, 0x00);
      break;
    case S23_ANIMATION_CYAN_PULSE:
      return PulseColor(animationFrame, 0x00, 0xFF, 0xFF);
      break;
    case S23_ANIMATION_GREEN_FLASH:
      return RandomFlashColor(animationFrame, 0x00, 0xFF, 0x00, animationNum%2);
      break;
    case S23_ANIMATION_PURPLE_LOOP:
      return LoopColor(animationFrame, 0xFF, 0x00, 0xFF);
      break;
    case S23_ANIMATION_LTOR_BLUE:
      return LeftToRightSweep(animationFrame, 0x00, 0x40, 0xFF);
      break;
    case S23_ANIMATION_RTOL_BLUE:
      return RightToLeftSweep(animationFrame, 0x40, 0x00, 0xFF);
      break;
    case S23_ANIMATION_LEFT_FLASH:
      return LeftFlash(animationFrame, 0x20, 0x40, 0x00);
      break;
    case S23_ANIMATION_RIGHT_FLASH:
      return RightFlash(animationFrame, 0x20, 0x40, 0x00);
      break;
    case S23_ANIMATION_SPARKLE:
      return Sparkle(animationFrame);
      break;
    case S23_ANIMATION_FIRE:
      return Fire(animationFrame, 0xFF, 0x80, 0x00, 1, 2, 1);
      break;
    case S23_ANIMATION_LIGHTNING_1:
    case S23_ANIMATION_LIGHTNING_2:
      return RandomFlashColor(animationFrame, 0x4F, 0x4F, 0x4F, animationNum%2);
      break;
    case S23_ANIMATION_LIGHTNING_3:
    case S23_ANIMATION_LIGHTNING_4:
    case S23_ANIMATION_LIGHTNING_5:
    case S23_ANIMATION_LIGHTNING_6:
    case S23_ANIMATION_LIGHTNING_7:
    case S23_ANIMATION_LIGHTNING_8:
      return RandomFlashColor(animationFrame, 0xFF, 0xFF, 0xFF, animationNum%2);
      break;
    case S23_ANIMATION_LIGHTNING_9:
    case S23_ANIMATION_LIGHTNING_10:
    case S23_ANIMATION_LIGHTNING_11:
    case S23_ANIMATION_LIGHTNING_12:
      return RandomFlashColor(animationFrame, 0x7F, 0x7F, 0x7F, animationNum%2);
      break;
    case S23_ANIMATION_BIG_LIGHTNING_1:
      return BigFlash(animationFrame, 0xAF, 0xAF, 0xFF, false);
      break;
    case S23_ANIMATION_BIG_LIGHTNING_2:
      return BigFlash(animationFrame, 0xDF, 0xDF, 0xFF, true);
      break;
    case S23_ANIMATION_DIM_LIGHTNING_1:
      return BigFlash(animationFrame, 0x60, 0x60, 0x60, false);
      break;
    case S23_ANIMATION_CANDY_PULSE:
      return Pulse2Colors(animationFrame, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF);
      break;
  }

  return false;
}



boolean StripHasBeenShown = true;

boolean UpdateStripsBasedOnInputs(unsigned long  *lastInputSeenTime, unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime) {
  // This accessory doesn't use inputs
  (void)lastInputSeenTime;
  (void)currentTime;
  (void)currentAnimationFrame;
  (void)machineState;
  (void)machineStateChangedTime;
  return false;
}


boolean UpdateRGBBasedOnI2C(unsigned long lastMessageSeenTime, byte lastMessage, byte lastParameter, unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime) {

  // This accessory doesn't use RGB lamps
  (void)lastMessageSeenTime;
  (void)lastMessage;
  (void)lastParameter;
  (void)currentTime;
  (void)currentAnimationFrame;
  (void)machineState;
  (void)machineStateChangedTime;
  return false;
}

byte LoopRunning = 0xFF;

boolean UpdateStripsBasedOnI2C(unsigned long lastMessageSeenTime, byte lastMessage, byte lastParameter, unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime) {

  (void)currentAnimationFrame;

  if (machineState==MACHINE_STATE_OFF) {
    CurrentLightingState = MACHINE_STATE_OFF;
    LoopRunning = 0xFF;
    if (StripHasBeenShown) {
      StripOfLEDs1.clear();
      StripOfLEDs1.show();
      StripHasBeenShown = false;
    }
    return false;
  }
  CurrentLightingState = machineState;

  if (lastMessage==ALB_COMMAND_STOP_ALL_ANIMATIONS || lastMessage==ALB_COMMAND_ALL_LAMPS_OFF) {
    LoopRunning = 0xFF;

    if (StripHasBeenShown) {
      StripOfLEDs1.clear();
      StripOfLEDs1.show();
      StripHasBeenShown = false;
    }
    return false;
  }

  if (currentTime>(lastMessageSeenTime+60000)) {
    if (StripHasBeenShown) {
      StripOfLEDs1.clear();
      StripOfLEDs1.show();
      StripHasBeenShown = false;
    }
    return false;    
  }

  StripOfLEDs1.clear();
  boolean animationRendered = false;

  if (lastMessage==ALB_COMMAND_PLAY_ANIMATION) {
    if (PlayAnimation(lastParameter, lastMessageSeenTime, currentTime)) {
      animationRendered = true;
    }
  } else if (lastMessage==ALB_COMMAND_LOOP_ANIMATION) {
    if (LoopStartTime==0) LoopStartTime = currentTime;
    LoopRunning = lastParameter;
    
    if (!PlayAnimation(lastParameter, LoopStartTime, currentTime)) {
      LoopStartTime = 0;
    }
    animationRendered = true;   
  } else if (lastMessage==ALB_COMMAND_STOP_ANIMATION) {
    if (lastParameter==LoopRunning) LoopRunning = 0xFF;
  }
  
  if (!animationRendered && LoopRunning!=0xFF) {
    if (LoopStartTime==0) LoopStartTime = currentTime;
    
    if (!PlayAnimation(LoopRunning, LoopStartTime, currentTime)) {
      LoopStartTime = 0;
    }
    animationRendered = true;
  }
  

  StripOfLEDs1.show();
  if (animationRendered) StripHasBeenShown = true;    

/*
  if (currentTime<(lastMessageSeenTime+500)) {
    StripOfLEDs1.clear();
  
    for (byte count=0; count<STRIP_1_NUMBER_OF_PIXELS; count++) {
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( (lastMessage&0x04)?250:0, (lastMessage&0x02)?250:0, (lastMessage&0x01)?250:0 ));
    }
  
    StripOfLEDs1.show();
    StripHasBeenShown = true;    
  } else {
    StripOfLEDs1.clear();
  
    byte colorPhase = (currentTime/300)%3;
    for (byte count=0; count<STRIP_1_NUMBER_OF_PIXELS; count++) {
      byte pixelColor = (colorPhase+count)%3;
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( (pixelColor==0)?250:0, (pixelColor==1)?250:0, (pixelColor==2)?250:0 ));
    }
  
    StripOfLEDs1.show();
    StripHasBeenShown = true;    
  }
*/
  //(void)machineState;
  (void)machineStateChangedTime;
  return true;
}


byte AdvanceSettingsMode(byte oldSettingsMode) {
  oldSettingsMode += 1;
  if (oldSettingsMode>0) oldSettingsMode = SETTINGS_MODE_OFF;
  return oldSettingsMode;
}

unsigned long LastTimeAnimationTriggered = 0;
boolean ShowSettingsMode(byte settingsMode, unsigned long currentAnimationFrame) {
  if (settingsMode==SETTINGS_MODE_OFF) return false;

  (void)currentAnimationFrame;

  StripOfLEDs1.clear();
  StripOfLEDs1.show();

  return true;
}


boolean IncreaseBrightness(byte settingsMode) {

  (void)settingsMode;

  return true;
}

boolean DecreaseBrightness(byte settingsMode) {

  (void)settingsMode;

  return true;
}
