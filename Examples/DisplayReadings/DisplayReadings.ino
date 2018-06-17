/*******************************************************************************************************************
** Program to demonstrate the INA library for the Arduino IDE. All INA2xx devices on the I2C bus are located and  **
** the program does a simple infinite loop to retrieve and display measurements for the bus voltage and current   **
** running through each INA device found.                                                                         **
**                                                                                                                **
** Detailed documentation can be found on the GitHub Wiki pages at https://github.com/SV-Zanshin/INA/wiki         **
**                                                                                                                **
** This example is for a INA set up to measure a 5-Volt load with a 0.1 Ohm resistor in place, this is the same   **
** setup that can be found in the Adafruit INA219 breakout board.  The complex calibration options are done at    **
** runtime using the 2 parameters specified in the "begin()" call and the library has gone to great lengths to    **
** avoid the use of floating point to conserve space and minimize runtime.  This demo program uses floating point **
** only to convert and display the data conveniently. The INA devices have 15 bits of precision, and even though  **
** the current and watt information is returned using 32-bit integers the precision remains the same.             **
**                                                                                                                **
** The library supports multiple INA devices and multiple INA device types. The Atmel's EEPROM is used to store   **
** the 96 bytes of static information per device using https://www.arduino.cc/en/Reference/EEPROM function calls. **
** Although up to 16 devices could theoretically be present on the I2C bus the actual limit is determined by the  **
** available EEPROM - ATmega328 UNO has 1024k so can support up to 10 devices but the ATmega168 only has 512 bytes**
** which limits it to supporting at most 5 INAs.                                                                  **
**                                                                                                                **
** This program is free software: you can redistribute it and/or modify it under the terms of the GNU General     **
** Public License as published by the Free Software Foundation, either version 3 of the License, or (at your      **
** option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY     **
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   **
** GNU General Public License for more details. You should have received a copy of the GNU General Public License **
** along with this program.  If not, see <http://www.gnu.org/licenses/>.                                          **
**                                                                                                                **
** Vers.  Date       Developer                     Comments                                                       **
** ====== ========== ============================= ============================================================== **
** 1.0.0b 2018-06-17 https://github.com/SV-Zanshin INA219 and INA226 completed, including testing                 **
** 1.0.0a 2018-06-10 https://github.com/SV-Zanshin Initial coding                                                 **
**                                                                                                                **
*******************************************************************************************************************/
#include <INA.h>                                                              // INA Library                      //
/*******************************************************************************************************************
** Declare program Constants                                                                                      **
*******************************************************************************************************************/
const uint32_t SERIAL_SPEED          = 115200;                                // Use fast serial speed            //
/*******************************************************************************************************************
** Declare global variables and instantiate classes                                                               **
*******************************************************************************************************************/
INA_Class INA;                                                                // INA class instantiation          //
uint8_t devicesFound = 0;                                                     // Number of INAs found             //
/*******************************************************************************************************************
** Method Setup(). This is an Arduino IDE method which is called first upon initial boot or restart. It is only   **
** called one time and all of the variables and other initialization calls are done here prior to entering the    **
** main loop for data measurement and storage.                                                                    **
*******************************************************************************************************************/
void setup() {                                                                //                                  //
  Serial.begin(SERIAL_SPEED);                                                 // Start serial communications      //
  #ifdef  __AVR_ATmega32U4__                                                  // If we are a 32U4 processor, then //
    delay(2000);                                                              // wait 2 seconds for the serial    //
  #endif                                                                      // interface to initialize          //
  Serial.print(F("\n\nDisplay INA Readings V1.0.0\n"));                       // Display program information      //
  Serial.print(F(" - Searching & Initializing INA devices\n"));               // Display program information      //
  // The begin initializes the calibration for an expected ±1 Amps maximum current and for a 0.1Ohm resistor, and //
  // since no specific device is given as the 3rd parameter all devices are initially set to these values         //
  devicesFound = INA.begin(1,100000);                                         // Set expected Amps and resistor   //
  Serial.print(F(" - Detected "));                                            //                                  //
  Serial.print(devicesFound);                                                 //                                  //
  Serial.println(F(" INA devices on the I2C bus"));                           //                                  //
  INA.setBusConversion(8500);                                                 // Maximum conversion time 8.244ms  //
  INA.setShuntConversion(8500);                                               // Maximum conversion time 8.244ms  //
  INA.setAveraging(128);                                                      // Average each reading n-times     //
  INA.setMode(INA_MODE_CONTINUOUS_BOTH);                                      // Bus/shunt measured continuously  //
} // of method setup()                                                        //                                  //
/*******************************************************************************************************************
** This is the main program for the Arduino IDE, it is called in an infinite loop. The INA measurements are       **
** run in a simple infinite loop                                                                                  **
*******************************************************************************************************************/
void loop() {                                                                 // Main program loop                //
  static uint16_t loopCounter = 0;                                            // Count the number of iterations   //
  Serial.print(F("# Type   Bus V   Shunt mV Bus mA    Bus mW\n"));            // Display the header lines         //
  Serial.print(F("= ====== ======= ======== ========= ========\n"));          //                                  //
  for (uint8_t i=0;i<devicesFound;i++) {                                      // Loop through all devices found   //
    Serial.print(i+1); Serial.print(F(" "));                                  //                                  //
    Serial.print(INA.getDeviceName(i)); Serial.print(F(" "));                 //                                  //
    Serial.print((float)INA.getBusMilliVolts(i)/1000.0,4);                    //                                  //
    Serial.print(F("V "));                                                    //                                  //
    Serial.print((float)INA.getShuntMicroVolts(i)/1000.0,3);                  // Convert to millivolts            //
    Serial.print(F("mV "));                                                   //                                  //
    Serial.print((float)INA.getBusMicroAmps(i)/1000.0,4);                     // Convert to milliamp              //
    Serial.print(F("mA "));                                                   //                                  //
    Serial.print((float)INA.getBusMicroWatts(i)/1000.0,4);                    // Convert to milliwatts            //
    Serial.print(F("mW\n"));                                                  //                                  //
  } // of for-next each device loop                                           //                                  //
  Serial.print(F("\n")  );                                                    //                                  //
  delay(5000);                                                                // Wait 5 seconds for next reading  //
  Serial.print(F("Loop iteration ")  );                                       //                                  //
  Serial.print(++loopCounter);                                                //                                  //
  Serial.print(F("\n\n")  );                                                  //                                  //
} // of method loop                                                           //----------------------------------//
