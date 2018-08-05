/*******************************************************************************************************************
** Class definition header for the INA class. This library gives a common interface to various INA power monitor  **
** devices, see https://github.com/SV-Zanshin/INA/wiki for a full list of currently supported devices.  The INA   **
** devices have a 3-5V power  supply and, depending upon the model, measure up to 26V or 36V. The are High-Side   **
** Low-Side Measurement, Bi-Directional Current and Power Monitor with I2C Compatible Interface. The devices      **
** are documented at http://www.ti.com/amplifier-circuit/current-sense/power-current-monitors/products.html       **
**                                                                                                                **
** Detailed documentation can be found on the INA GitHub Wiki pages at https://github.com/SV-Zanshin/INA/wiki     **
**                                                                                                                **
** The INA devices, apart from the INA260, require an external shunt of known resistance to be placed across the  **
** high-side or low-side supply or ground line and they use the small current generated by the shunt to compute   **
** the amperage passing across the circuit.  This value, coupled with the voltage measurement, allows the Amperage**
** and Wattage to be computed by the INA device and all of these values can be read using the industry standard   **
** I2C protocol.                                                                                                  **
**                                                                                                                **
** Although programming for the Arduino and in c/c++ is new to me, I'm a professional programmer and have learned,**
** over the years, that it is much easier to ignore superfluous comments than it is to decipher non-existent ones;**
** so both my comments and variable names tend to be somewhat verbose and redundant. The code is written to fit   **
** in the first 80 spaces and the comments start after that and go to column 117, this allows the code to be      **
** printed in A4 landscape mode with correct pagination.                                                          **
**                                                                                                                **
** GNU General Public License v3.0                                                                                **
** ===============================                                                                                **
** This program is free software: you can redistribute it and/or modify it under the terms of the GNU General     **
** Public License as published by the Free Software Foundation, either version 3 of the License, or (at your      **
** option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY     **
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   **
** GNU General Public License for more details. You should have received a copy of the GNU General Public License **
** along with this program (see https://github.com/SV-Zanshin/INA/blob/master/LICENSE).  If not, see              **
** <http://www.gnu.org/licenses/>.                                                                                **
**                                                                                                                **
** Vers.  Date       Developer                     Comments                                                       **
** ====== ========== ============================= ============================================================== **
** 1.0.2  2018-07-22 https://github.com/SV-Zanshin Issue #11. Reduce EEPROM footprint. Removed "deviceName", 8B.  **
**                                                 Changed datatype in structure to bit-level length definitions, **
**                                                 saving additional 3 bytes                                      **
** 1.0.2  2018-07-21 https://github.com/avaldeve   Issue #12. Incorrect const datatype for I2C Speeds             **
** 1.0.2  2018-07-12 https://github.com/coelner    Issue #9. Esplora doesn't accept "Wire.begin()"                **
** 1.0.2  2018-07-08 https://github.com/SV-Zanshin Issue #2. Finished testing INA3221 across all functions        **
** 1.0.2  2018-07-07 https://github.com/dnlwgnd    Issue #4. Guard code used incorrect label                      **
** 1.0.2  2018-06-30 https://github.com/SV-Zanshin Issue #3. Adding faster I2C bus support                        **
** 1.0.2  2018-06-29 https://github.com/SV-Zanshin Issue #2. Adding INA3221 support to library                    **
** 1.0.2  2018-06-29 https://github.com/SV-Zanshin Issue #2. Adding INA3221 support to library                    **
** 1.0.1  2018-06-24 https://github.com/SV-Zanshin Removed extraneous elements from ina structure, optimzed code  **
** 1.0.1b 2018-06-23 https://github.com/SV-Zanshin Fixed error on multiple devices with ina structure contents    **
** 1.0.1a 2018-06-23 https://github.com/SV-Zanshin Removed debug mode and code                                    **
** 1.0.0  2018-06-22 https://github.com/SV-Zanshin Initial release                                                **
** 1.0.0b 2018-06-17 https://github.com/SV-Zanshin Continued coding, tested on INA219 and INA226                  **
** 1.0.0a 2018-06-10 https://github.com/SV-Zanshin Initial coding began                                           **
**                                                                                                                **
*******************************************************************************************************************/
#if ARDUINO >= 100                                                            // The Arduino IDE versions before  //
  #include "Arduino.h"                                                        // 100 need to use the older library//
#else                                                                         // rather than the new one          //
  #include "WProgram.h"                                                       //                                  //
#endif                                                                        //                                  //
#ifndef INA__Class_h                                                          // Guard code definition            //
  #define INA__Class_h                                                        // Define the name inside guard code//
  /*****************************************************************************************************************
  ** Declare structures and enumerated types used in the class                                                    **
  *****************************************************************************************************************/
  typedef struct {                                                            // Structure of values per device   //
    uint8_t  type                 : 4; // 0-15 //                             // see enumerated "ina_Type"        //
    uint8_t  operatingMode        : 4; // 0-15 //                             // Default to continuous mode       //
    uint32_t address              : 7; // 0-127//                             // I2C Address of device            //
    uint32_t maxBusAmps           : 5; // 0-31 //                             // Store initialization value       //
    uint32_t microOhmR            :20; // 0-1.048.575 //                      // Store initialization value       //
  } inaEEPROM; // of structure                                                //                                  //
  typedef struct inaDet : inaEEPROM {                                         // Structure of values per device   //
    uint8_t  busVoltageRegister   : 3; // 0- 7 //                             // Bus Voltage Register             //
    uint8_t  shuntVoltageRegister : 3; // 0- 7 //                             // Shunt Voltage Register           //
    uint8_t  currentRegister      : 3; // 0- 7 //                             // Current Register                 //
    uint16_t shuntVoltage_LSB;                                                // Device dependent LSB factor      //
    uint16_t busVoltage_LSB;                                                  // Device dependent LSB factor      //
    uint32_t current_LSB;                                                     // Amperage LSB                     //
    uint32_t power_LSB;                                                       // Wattage LSB                      //
    inaDet();                                                                 // struct constructor               //
    inaDet(inaEEPROM inaEE);                                                  // for ina = inaEE; assignment      //
  } inaDet; // of structure                                                   //                                  //
                                                                              //                                  //
  enum ina_Type { INA219,                                                     // List of supported devices        //
                  INA226,                                                     //                                  //
                  INA230,                                                     //                                  //
                  INA231,                                                     //                                  //
                  INA260,                                                     //                                  //
                  INA3221_0,                                                  // INA3221 1st channel              //
                  INA3221_1,                                                  // INA3221 2nd channel              //
                  INA3221_2,                                                  // INA3221 3rd channel              //
                  UNKNOWN };                                                  //                                  //
  enum ina_Mode { INA_MODE_SHUTDOWN,                                          // Device powered down              //
                  INA_MODE_TRIGGERED_SHUNT,                                   // Triggered shunt, no bus          //
                  INA_MODE_TRIGGERED_BUS,                                     // Triggered bus, no shunt          //
                  INA_MODE_TRIGGERED_BOTH,                                    // Triggered bus and shunt          //
                  INA_MODE_POWER_DOWN,                                        // shutdown or power-down           //
                  INA_MODE_CONTINUOUS_SHUNT,                                  // Continuous shunt, no bus         //
                  INA_MODE_CONTINUOUS_BUS,                                    // Continuous bus, no shunt         //
                  INA_MODE_CONTINUOUS_BOTH };                                 // Both continuous, default value   //
  /*****************************************************************************************************************
  ** Declare constants used in the class                                                                          **
  *****************************************************************************************************************/
  #ifndef I2C_MODES                                                           // I2C related constants            //
    #define I2C_MODES                                                         // Guard code to prevent multiple   //
    const uint32_t I2C_STANDARD_MODE            =  100000;                    // Default normal I2C 100KHz speed  //
    const uint32_t I2C_FAST_MODE                =  400000;                    // Fast mode                        //
    const uint32_t I2C_FAST_MODE_PLUS           = 1000000;                    // Really fast mode                 //
    const uint32_t I2C_HIGH_SPEED_MODE          = 3400000;                    // Turbo mode                       //
  #endif                                                                      //----------------------------------//
  const uint8_t  INA_CONFIGURATION_REGISTER     =       0;                    // Values common to all INAs        //
  const uint8_t  INA_BUS_VOLTAGE_REGISTER       =       2;                    //                                  //
  const uint8_t  INA_POWER_REGISTER             =       3;                    //                                  //
  const uint8_t  INA_CALIBRATION_REGISTER       =       5;                    //                                  //
  const uint8_t  INA_MASK_ENABLE_REGISTER       =       6;                    // Not found on all devices         //
  const uint8_t  INA_ALERT_LIMIT_REGISTER       =       7;                    // Not found on all devices         //
  const uint8_t  INA_MANUFACTURER_ID_REGISTER   =    0xFE;                    // Not found on all devices         //
  const uint8_t  INA_DIE_ID_REGISTER            =    0xFF;                    // Not found on all devices         //
  const uint16_t INA_RESET_DEVICE               =  0x8000;                    // Write to configuration to reset  //
  const uint16_t INA_CONVERSION_READY_MASK      =  0x0080;                    // Bit 4                            //
  const uint16_t INA_CONFIG_MODE_MASK           =  0x0007;                    // Bits 0-3                         //
  const uint16_t INA_ALERT_MASK                 =  0x03FF;                    // Mask off bits 0-9                //
  const uint8_t  INA_ALERT_SHUNT_OVER_VOLT_BIT  =      15;                    // Register bit                     //
  const uint8_t  INA_ALERT_SHUNT_UNDER_VOLT_BIT =      14;                    // Register bit                     //
  const uint8_t  INA_ALERT_BUS_OVER_VOLT_BIT    =      13;                    // Register bit                     //
  const uint8_t  INA_ALERT_BUS_UNDER_VOLT_BIT   =      12;                    // Register bit                     //
  const uint8_t  INA_ALERT_POWER_OVER_WATT_BIT  =      11;                    // Register bit                     //
  const uint8_t  INA_ALERT_CONVERSION_RDY_BIT   =      10;                    // Register bit                     //
  const uint8_t  INA_DEFAULT_OPERATING_MODE     =    B111;                    // Default continuous mode          //
                                                                              //==================================//
                                                                              // Device-specific values           //
                                                                              //==================================//
  const uint8_t  INA219_SHUNT_VOLTAGE_REGISTER  =       1;                    // Shunt Voltage Register           //
  const uint8_t  INA219_CURRENT_REGISTER        =       4;                    // Current Register                 //
  const uint16_t INA219_BUS_VOLTAGE_LSB         =     400;                    // LSB in uV *100 4.00mV            //
  const uint16_t INA219_SHUNT_VOLTAGE_LSB       =     100;                    // LSB in uV *10  10.0uV            //
  const uint16_t INA219_CONFIG_AVG_MASK         =  0x07F8;                    // Bits 3-6, 7-10                   //
  const uint16_t INA219_CONFIG_PG_MASK          =  0xE7FF;                    // Bits 11-12 masked                //
  const uint16_t INA219_CONFIG_BADC_MASK        =  0x0780;                    // Bits 7-10  masked                //
  const uint16_t INA219_CONFIG_SADC_MASK        =  0x0038;                    // Bits 3-5                         //
  const uint8_t  INA219_BRNG_BIT                =      13;                    // Bit for BRNG in config register  //
  const uint8_t  INA219_PG_FIRST_BIT            =      11;                    // first bit of Programmable Gain   //
                                                                              //----------------------------------//
  const uint8_t  INA226_SHUNT_VOLTAGE_REGISTER  =       1;                    // Shunt Voltage Register           //
  const uint8_t  INA226_CURRENT_REGISTER        =       4;                    // Current Register                 //
  const uint16_t INA226_BUS_VOLTAGE_LSB         =     125;                    // LSB in uV *100 1.25mV            //
  const uint16_t INA226_SHUNT_VOLTAGE_LSB       =      25;                    // LSB in uV *10  2.5uV             //
  const uint16_t INA226_CONFIG_AVG_MASK         =  0x0E00;                    // Bits 9-11                        //
  const uint16_t INA226_DIE_ID_VALUE            =  0x2260;                    // Hard-coded Die ID for INA226     //
  const uint16_t INA226_CONFIG_BADC_MASK        =  0x01C0;                    // Bits 6-8  masked                //
  const uint16_t INA226_CONFIG_SADC_MASK        =  0x0018;                    // Bits 3-4                         //
                                                                              //==================================//
  const uint8_t  INA260_SHUNT_VOLTAGE_REGISTER  =       0;                    // Register doesn't exist on device //
  const uint8_t  INA260_CURRENT_REGISTER        =       1;                    // Current Register                 //
  const uint16_t INA260_BUS_VOLTAGE_LSB         =     125;                    // LSB in uV *100 1.25mV            //
  const uint16_t INA260_CONFIG_BADC_MASK        =  0x01C0;                    // Bits 6-8  masked                 //
  const uint16_t INA260_CONFIG_SADC_MASK        =  0x0038;                    // Bits 3-5  masked                 //
                                                                              //----------------------------------//
  const uint8_t  INA3221_SHUNT_VOLTAGE_REGISTER =       1;                    // Register number 1                //
  const uint16_t INA3221_BUS_VOLTAGE_LSB        =     800;                    // LSB in uV *100 8mV               //
  const uint16_t INA3221_SHUNT_VOLTAGE_LSB      =     400;                    // LSB in uV *10  40uV              //
  const uint16_t INA3221_CONFIG_BADC_MASK       =  0x01C0;                    // Bits 7-10  masked                //
  const uint8_t  INA3221_MASK_REGISTER          =     0xF;                    // Mask register                    //
                                                                              //==================================//
  const uint8_t  I2C_DELAY                      =      10;                    // Microsecond delay on write       //
  /*****************************************************************************************************************
  ** Declare class header                                                                                         **
  *****************************************************************************************************************/
  class INA_Class {                                                           // Class definition                 //
    public:                                                                   // Publicly visible methods         //
      INA_Class();                                                            // Class constructor                //
      ~INA_Class();                                                           // Class destructor                 //
      uint8_t  begin             (const uint8_t  maxBusAmps,                  // Class initializer                //
                                  const uint32_t microOhmR,                   //                                  //
                                  const uint8_t  deviceNumber = UINT8_MAX );  //                                  //
      void     setI2CSpeed       (const uint32_t i2cSpeed =I2C_STANDARD_MODE);// Adjust the I2C bus speed         //
      void     setMode           (const uint8_t  mode,                        // Set the monitoring mode          //
                                  const uint8_t  devNumber=UINT8_MAX);        //                                  //
      void     setAveraging      (const uint16_t averages,                    // Set the number of averages taken //
                                  const uint8_t  deviceNumber=UINT8_MAX);     //                                  //
      void     setBusConversion  (const uint32_t convTime,                    // Set timing for Bus conversions   //
                                  const uint8_t  deviceNumber=UINT8_MAX);     //                                  //
      void     setShuntConversion(const uint32_t convTime,                    // Set timing for Shunt conversions //
                                  const uint8_t  deviceNumber=UINT8_MAX);     //                                  //
      uint16_t getBusMilliVolts  (const uint8_t  deviceNumber=0);             // Retrieve Bus voltage in mV       //
      int32_t  getShuntMicroVolts(const uint8_t  deviceNumber=0);             // Retrieve Shunt voltage in uV     //
      int32_t  getBusMicroAmps   (const uint8_t  deviceNumber=0);             // Retrieve micro-amps              //
      int32_t  getBusMicroWatts  (const uint8_t  deviceNumber=0);             // Retrieve micro-watts             //
      const char* getDeviceName  (const uint8_t  deviceNumber=0);             // Retrieve device name (const char)//
      void     reset             (const uint8_t  deviceNumber=0);             // Reset the device                 //
      void     waitForConversion (const uint8_t  deviceNumber=UINT8_MAX);     // wait for conversion to complete  //
      bool     AlertOnConversion (const bool alertState,                      // Enable pin change on conversion  //
                                  const uint8_t deviceNumber=UINT8_MAX);      //                                  //
      bool     AlertOnShuntOverVoltage (const bool alertState,                // Enable pin change on conversion  //
                                        const int32_t milliVolts,             //                                  //
                                        const uint8_t deviceNumber=UINT8_MAX);//                                  //
      bool     AlertOnShuntUnderVoltage(const bool alertState,                // Enable pin change on conversion  //
                                        const int32_t milliVolts,             //                                  //
                                        const uint8_t deviceNumber=UINT8_MAX);//                                  //
      bool     AlertOnBusOverVoltage   (const bool alertState,                // Enable pin change on conversion  //
                                        const int32_t milliVolts,             //                                  //
                                        const uint8_t deviceNumber=UINT8_MAX);//                                  //
      bool     AlertOnBusUnderVoltage  (const bool alertState,                // Enable pin change on conversion  //
                                        const int32_t milliVolts,             //                                  //
                                        const uint8_t deviceNumber=UINT8_MAX);//                                  //
      bool     AlertOnPowerOverLimit   (const bool alertState,                // Enable pin change on conversion  //
                                        const int32_t milliAmps,              //                                  //
                                        const uint8_t deviceNumber=UINT8_MAX);//                                  //
    private:                                                                  // Private variables and methods    //
      int16_t  readWord         (const uint8_t addr,                          // Read a word from an I2C address  //
                                 const uint8_t deviceAddress);                //                                  //
      void     writeWord        (const uint8_t addr, const uint16_t data,     // Write a word to an I2C address   //
                                 const uint8_t deviceAddress);                //                                  //
      void     readInafromEEPROM(const uint8_t deviceNumber);                 // Retrieve structure from EEPROM   //
      void     writeInatoEEPROM (const uint8_t deviceNumber);                 // Write structure to EEPROM        //
      void     initDevice       (const uint8_t deviceNumber);                 // Initialize any Device            //
      uint8_t  _DeviceCount        = 0;                                       // Number of INAs detected          //
      uint8_t  _currentINA         = UINT8_MAX;                               // Stores current INA device number //
      inaEEPROM inaEE;                                                        // Declare a single global value    //
      inaDet   ina;                                                           // Declare a single global value    //
  }; // of INA_Class definition                                               //                                  //
#endif                                                                        //----------------------------------//
