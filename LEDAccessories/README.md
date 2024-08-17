# Lightning LED Lamps  
This code runs on an ALB (Accessory Lamp Board), and responds to commands from an RPU board running Storm23 rules.  
A strip of LEDs should be placed on a topper or on top of the machine. The strip can be of any number of pixels, but should be divided into four rows in order to achieve the desired effects.  

## Bulding the ALB  
The schematic and Gerber files for the ALB are in this folder.  
Use an Arduino MEGA 2560 Pro.  
Header pins for the i2c and WS2812 are 0.1" (2.54mm) headers. 
The barrel connector is a [SparkFun DC power jack](https://www.mouser.com/ProductDetail/SparkFun/PRT-00119?qs=WyAARYrbSnbnQpS9xf%252Bx5A%3D%3D&countryCode=US&currencyCode=USD).  
The alternative power is 2-position 0.156" (3.96mm) header pins.  
The female headers for the Arduino are 0.1" (2.54mm) Female sockets.  

## Connections
Connect the Arduino on the ALB using the i2c port to the i2c port on the RPU.  
Connect a strip (four rows) of WS2812 LEDs to the H2 port of the ALB.  
Connect either 5V or 12V (depending on LED strip type) to the ALB power section using either the barrel connector or the 2-pin molex power header. Be sure to jumper the ALB for either 5V or 12V to match the power supply.  

## Get the Code and install it  
From the [Storm23](https://github.com/RetroPinUpgrade/Storm23/tree/ALB-Branch) folder, click "Code" then "Download Zip".  
Make sure that the LEDAccessories code is unzipped into a folder called, precisely, LEDAccessories.  
In the Storm23Topper.cpp file, be sure to set the total number of pixles that your LED strip has in the define called "STRIP_1_NUMBER_OF_PIXELS". For example, if your LED strip is 4 rows of 13 pixels, you would set the define to 52:  
````  
#define STRIP_1_NUMBER_OF_PIXELS    52  
````  
Open the LEDAccessories.ino in the Arduino compiler and you should be able to compile the code and upload it to the Arduino MEGA on the ALB board.  



  
