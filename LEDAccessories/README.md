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


## Parts List  
### PCB
### Arduino
This board requires an Arduino MEGA 2560 Pro (at the time of this writing, between $10 ~ $20 each):  
https://www.amazon.com/HiLetgo-MEGA2560-CH340G-ATMEGA2560-16AU-Headers/dp/B0832RQBR1/  

### Headers and Ports  
#### Arduino Socket  
Qty 1, 2x5 for Arduino 3M Female Header (2x5) Part# 929852-01-05-RA  
https://www.mouser.com/ProductDetail/3M-Electronic-Solutions-Division/929852-01-05-RA?qs=neFkstNq%252B6Ex1fEfkze0Fg%3D%3D&countryCode=US&currencyCode=USD  
Qty 2, 2x16 for Arduino 3M Female Header (2x16) Part# 929852-01-16-RA    
https://www.mouser.com/ProductDetail/3M-Electronic-Solutions-Division/929852-01-16-RA?qs=neFkstNq%252B6FJU7HdNnS4aQ%3D%3D&countryCode=US&currencyCode=USD  
  
#### Power (use either barrel connector or 2-pin 0.156" Male)  
Qty 1, DC Barrel Connector Part# PRT-00119  
https://www.mouser.com/ProductDetail/SparkFun/PRT-00119?qs=WyAARYrbSnbnQpS9xf%252Bx5A%3D%3D&countryCode=US&currencyCode=USD  
(OR)  
Qty 1, Molex 2pin Male Header (0.156") Part# 26-60-4020  
https://www.mouser.com/ProductDetail/Molex/26-60-4020?qs=ZwgtpdmWYYSCkM5sV%252BTTVQ%3D%3D&countryCode=US&currencyCode=USD  
  
#### 5050 Drivers (optional)  
Qty 3, TIP102   
https://www.mouser.com/ProductDetail/STMicroelectronics/TIP102?qs=ljbEvF4DwONmWw0zLDYvVw%3D%3D  
Qty 3, 2N4401  
https://www.mouser.com/ProductDetail/637-2N4401  
Qty 3, 510 Ohm 1/4W resistor  
https://www.mouser.com/ProductDetail/KOA-Speer/MF1-4LCT52R511J?qs=91WPSIiQh9K9EQzPBl%2F5Xg%3D%3D  
Qty 3, 2.2 kOhm 1/4 resistor  
https://www.mouser.com/ProductDetail/Vishay-Dale/CCF072K20GKE36?qs=QKEOZdL6EQocqyVzx7lQ8Q%3D%3D  
Qty 3, 1x5 0.156" Molex Header (26-60-4050)   
https://www.mouser.com/ProductDetail/Molex/26-60-4050?qs=tRPrwvvr%2FujZmUu0JhEmww%3D%3D&countryCode=US&currencyCode=USD  
  

  
#### KK Header Key Pins  
https://www.mouser.com/ProductDetail/Molex/15-04-0297?qs=jcz9%252BeLWjlgekF8j6YPhGw%3D%3D&countryCode=US&currencyCode=USD  
  
#### Connector Bodies  
2-pin 0.156" Molex (26-03-3021)   
https://www.mouser.com/ProductDetail/Molex/26-03-3021?qs=Nzhos1w%252BoCe1ZbBMgYGpDQ%3D%3D&countryCode=US&currencyCode=USD  

#### Long Header Pins (0.1") - row of 40 to cut down for various connectors    
https://www.mouser.com/ProductDetail/TE-Connectivity/9-146274-0?qs=g1eCW3XhYQKtxd9G6KjUhA%3D%3D&countryCode=US&currencyCode=USD  
  



  



  
