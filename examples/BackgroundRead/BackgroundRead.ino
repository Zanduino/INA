/*******************************************************************************************************************
** Program to demonstrate using the interrupt pin of any INA2xx which supports that functionality. It uses a      **
** pin-change interrupt handler and programs any INA2xx found to to read voltage and current information in the   **
** background while allowing the main Arduino code to continue processing normally until it is ready to consume   **
** the readings.                                                                                                  **
**                                                                                                                **
** Detailed documentation can be found on the GitHub Wiki pages at https://github.com/SV-Zanshin/INA/wiki         **
**                                                                                                                **
** Since the INA library allows multiple devices of different types and this program demonstrates interrupts and  **
** background processing, it will limit itself to using the first INA226 detected. This is easily changed in the  **
** if another device type or device number to test is required.                                                   **
**                                                                                                                **
** This example is for a INA226 set up to measure a 5-Volt load with a 0.1Ω resistor in place, this is the same   **
** setup that can be found in the Adafruit INA226 breakout board.  The complex calibration options are done at    **
** runtime using the 2 parameters specified in the "begin()" call and the library has gone to great lengths to    **
** avoid the use of floating point to conserve space and minimize runtime.  This demo program uses floating point **
** only to convert and display the data conveniently. The INA226 uses 15 bits of precision, and even though the   **
** current and watt information is returned using 32-bit integers the precision remains the same.                 **
**                                                                                                                **
** The INA226 is set up to measure using the maximum conversion length (and maximum accuracy) and then average    **
** those readings 64 times. This results in readings taking 8.244ms x 64 = 527.616ms or just less than 2 times    **
** a second. The pin-change interrupt handler is called when a reading is finished and the INA226 pulls the pin   **
** down to ground, it resets the pin status and adds the readings to the global variables. The main program will  **
** do whatever processing it has to and every 5 seconds it will display the current averaged readings and reset   **
** them.                                                                                                          **
**                                                                                                                **
** The datasheet for the INA226 can be found at http://www.ti.com/lit/ds/symlink/INA226.pdf and it contains the   **
** information required in order to hook up the device. Unfortunately it comes as a VSSOP package but it can be   **
** soldered onto a breakout board for breadboard use. The INA226 is quite similar to the INA219 mentioned above,  **
** but it can take bus voltages of up to 36V (which I needed in order to monitor a 24V battery system which goes  **
** above 28V while charging and which is above the absolute limits of the INA219). It is also significantly more  **
** accurate than the INA219, plus has an alert pin.                                                               **
**                                                                                                                **
** The interrupt is set to pin 8. The tests were done on an Arduino Micro, and the Atmel 82U4 chip only allows    **
** pin change interrupt on selected pins (SS,SCK,MISO,MOSI,8) so pin 8 was chosen.                                **
**                                                                                                                **
** GNU General Public License 3                                                                                   **
** ============================                                                                                   **
** This program is free software: you can redistribute it and/or modify it under the terms of the GNU General     **
** Public License as published by the Free Software Foundation, either version 3 of the License, or (at your      **
** option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY     **
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   **
** GNU General Public License for more details. You should have received a copy of the GNU General Public License **
** along with this program (see https://github.com/SV-Zanshin/INA/blob/master/LICENSE).  If not, see              **
** <http://www.gnu.org/licenses/>.                                                                                **                                                                                                                **
**                                                                                                                **
**                                                                                                                **
** Vers.  Date       Developer                     Comments                                                       **
** ====== ========== ============================= ============================================================== **
** 1.0.0   2018-06-23 https://github.com/SV-Zanshin Cloned and adapted example from old deprecated INA226 library **
**                                                                                                                **
*******************************************************************************************************************/
#include <INA.h>                                                              // INA Library                      //
/*******************************************************************************************************************
** Declare program Constants                                                                                      **
*******************************************************************************************************************/
const uint8_t  INA_ALERT_PIN   =      8;                                      // Pin 8. Micro only allows SS,SCK, //
const uint8_t  GREEN_LED_PIN   =     13;                                      // Green LED (nonstandard location) //
const uint32_t SERIAL_SPEED    = 115200;                                      // Use fast serial speed            //
/*******************************************************************************************************************
** Declare global variables and instantiate classes                                                               **
*******************************************************************************************************************/
INA_Class INA;                                                                // INA class instantiation          //
volatile uint8_t  deviceNumber    = UINT8_MAX;                                // Device Number to use in example  //
volatile uint64_t sumBusMillVolts =         0;                                // Sum of bus voltage readings      //
volatile int64_t  sumBusMicroAmps =         0;                                // Sum of bus amperage readings     //
volatile uint8_t  readings        =         0;                                // Number of measurements taken     //
/*******************************************************************************************************************
** Declare interrupt service routine for the pin-change interrupt on pin 8 which is set in the setup() method     **
*******************************************************************************************************************/
ISR (PCINT0_vect) {                                                           // handle pin change interrupt D8   //
  *digitalPinToPCMSK(INA_ALERT_PIN)&=~bit(digitalPinToPCMSKbit(INA_ALERT_PIN));// Disable PCMSK pin               //
  PCICR  &= ~bit(digitalPinToPCICRbit(INA_ALERT_PIN));                        // disable interrupt for the group  //
  sei();                                                                      // Enable interrupts for I2C calls  //
  digitalWrite(GREEN_LED_PIN,!digitalRead(GREEN_LED_PIN));                    // Toggle LED to show we are working//
  sumBusMillVolts += INA.getBusMilliVolts(deviceNumber);                      // Read the current value           //
  sumBusMicroAmps += INA.getBusMicroAmps(deviceNumber);                       // Read the current value           //
  readings++;                                                                 // Increment the number of readings //
  INA.waitForConversion(deviceNumber);                                        // Resets INA interrupt flag        //
  cli();                                                                      // Disable interrupts               //
  *digitalPinToPCMSK(INA_ALERT_PIN)|=bit(digitalPinToPCMSKbit(INA_ALERT_PIN));// Enable PCMSK pin                 //
  PCIFR  |= bit (digitalPinToPCICRbit(INA_ALERT_PIN));                        // clear any outstanding interrupt  //
  PCICR  |= bit (digitalPinToPCICRbit(INA_ALERT_PIN));                        // enable interrupt for the group   //
} // of ISR handler for INT0 group of pins                                    //                                  //
/*******************************************************************************************************************
** Method Setup(). This is an Arduino IDE method which is called first upon initial boot or restart. It is only   **
** called one time and all of the variables and other initialization calls are done here prior to entering the    **
** main loop for data measurement.                                                                                **
*******************************************************************************************************************/
void setup() {                                                                //                                  //
  pinMode(GREEN_LED_PIN, OUTPUT);                                             // Define the green LED as an output//
  digitalWrite(GREEN_LED_PIN,true);                                           // Turn on the LED                  //
  pinMode(INA_ALERT_PIN,INPUT_PULLUP);                                        // Declare pin with pull-up resistor//
  *digitalPinToPCMSK(INA_ALERT_PIN)|=bit(digitalPinToPCMSKbit(INA_ALERT_PIN));// Enable PCMSK pin                 //
  PCIFR |= bit (digitalPinToPCICRbit(INA_ALERT_PIN));                         // clear any outstanding interrupt  //
  PCICR |= bit (digitalPinToPCICRbit(INA_ALERT_PIN));                         // enable interrupt for the group   //
  Serial.begin(SERIAL_SPEED);                                                 // Start serial communications      //
  #ifdef  __AVR_ATmega32U4__                                                  // If this is a 32U4 processor,     //
    delay(2000);                                                              // wait 3 seconds for serial port   //
  #endif                                                                      // interface to initialize          //
  Serial.print(F("\n\nBackground INA Read V1.0.0\n"));                        // Display program information      //
  uint8_t devicesFound = 0;                                                   // Number of INA2xx found on I2C    //
  while (deviceNumber==UINT8_MAX) {                                           // Loop until we find devices       //
    devicesFound = INA.begin(1,100000);                                       // ±1Amps maximum for 0.1Ω resistor //
    for (uint8_t i=0;i<devicesFound;i++) {                                    // the first INA226 device found    //
                                                                              // Change "INA226" to "INA260" or   //
                                                                              // whichever INA2xx to measure      //
      if (strcmp(INA.getDeviceName(i),"INA226")==0) {                         // Set deviceNumber appropriately   //
        deviceNumber = i;                                                     //                                  //
        INA.reset(deviceNumber);                                              // Reset device to default settings //
        break;                                                                //                                  //
      } // of if-then we have found an INA226                                 //                                  //
    } // of for-next loop through all devices found                           //                                  //
    if (deviceNumber==UINT8_MAX) {                                            // Try again if no device found     //
      Serial.print(F("No INA found. Waiting 5s.\n"));                         //                                  //
      delay(5000);                                                            //                                  //
    } // of if-then no INA226 found                                           //                                  //
  } // of if-then no device found                                             //                                  //
  Serial.print(F("Found INA at device number "));                             //                                  //
  Serial.println(deviceNumber);                                               //                                  //
  Serial.println();                                                           //                                  //
  INA.setAveraging(64,deviceNumber);                                          // Average each reading 64 times    //
  INA.setBusConversion(82440,deviceNumber);                                   // Maximum conversion time 8.244ms  //
  INA.setShuntConversion(82440,deviceNumber);                                 // Maximum conversion time 8.244ms  //
  INA.setMode(INA_MODE_CONTINUOUS_BOTH,deviceNumber);                         // Bus/shunt measured continuously  //
  INA.AlertOnConversion(true,deviceNumber);                                   // Make alert pin go low on finish  //
} // of method setup()                                                        //                                  //
/*******************************************************************************************************************
** This is the main program for the Arduino IDE, it is called in an infinite loop. The INA226 measurements are    **
** triggered by the interrupt handler each time a conversion is ready and stored in variables. The main infinite  **
** doesn't call any INA library functions, that is done in the interrupt handler. Each time 10 readings have been **
** collected the program will output the averaged values and measurements resume from that point onwards          **
*******************************************************************************************************************/
void loop() {                                                                 // Main program loop                //
  static long lastMillis = millis();                                          // Store the last time we printed   //
  if (readings>=10) {                                                         // If it is time to display results //
    Serial.print(F("Averaging readings taken over "));                        //                                  //
    Serial.print((float)(millis()-lastMillis)/1000,2);                        //                                  //
    Serial.print(F(" seconds.\nBus voltage:   "));                            //                                  //
    Serial.print((float)sumBusMillVolts/readings/1000.0,4);                   //                                  //
    Serial.print(F("V\nBus amperage:  "));                                    //                                  //
    Serial.print((float)sumBusMicroAmps/readings/1000.0,4);                   //                                  //
    Serial.print(F("mA\n\n"));                                                //                                  //
    lastMillis = millis();                                                    //                                  //
    cli();                                                                    // Disable interrupts               //
    readings        = 0;                                                      // Reset values                     //
    sumBusMillVolts = 0;                                                      // Reset values                     //
    sumBusMicroAmps = 0;                                                      // Reset values                     //
    sei();                                                                    // Enable interrupts                //
  } // of if-then we've reached the required amount of readings               //                                  //
} // of method loop                                                           //----------------------------------//
