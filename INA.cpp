/*******************************************************************************************************************
** INA class method definitions for INA Library.                                                                  **
**                                                                                                                **
** See the INA.h header file comments for version information. Detailed documentation for the library can be      **
** found on the GitHub Wiki pages at https://github.com/SV-Zanshin/INA/wiki                                       **
**                                                                                                                **
** This program is free software: you can redistribute it and/or modify it under the terms of the GNU General     **
** Public License as published by the Free Software Foundation, either version 3 of the License, or (at your      **
** option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY     **
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   **
** GNU General Public License for more details. You should have received a copy of the GNU General Public License **
** along with this program.  If not, see <http://www.gnu.org/licenses/>.                                          **
**                                                                                                                **
*******************************************************************************************************************/
#include "INA.h"                                                              // Include the header definition    //
#include <Wire.h>                                                             // I2C Library definition           //
#include <EEPROM.h>                                                           // Include the EEPROM library       //
INA_Class::INA_Class()  {}                                                    // Class constructor                //
INA_Class::~INA_Class() {}                                                    // Unused class destructor          //
/*******************************************************************************************************************
** Method begin() searches for possible devices and sets the INA Configuration details, without which meaningful  **
** readings cannot be made. If it is called without the option deviceNumber parameter then the settings are       **
** applied to all devices, otherwise just that specific device is targeted.                                       **
*******************************************************************************************************************/
uint8_t INA_Class::begin(const uint8_t maxBusAmps,                            // Class initializer                //
                            const uint32_t microOhmR,                         //                                  //
                            const uint8_t deviceNumber ) {                    //                                  //
  uint16_t originalRegister,tempRegister;                                     // Stores 16-bit register contents  //
  if (_DeviceCount==0) {                                                      // Enumerate devices in first call  //
    Wire.begin();                                                             // Start the I2C wire subsystem     //
    for(uint8_t deviceAddress = 64;deviceAddress<79;deviceAddress++) {        // Loop for each possible address   //
      ina.address = deviceAddress;                                            // Store device address             //
      Wire.beginTransmission(deviceAddress);                                  // See if something is at address   //
      if (Wire.endTransmission() == 0) {                                      // by checking the return error     //
        originalRegister = readWord(INA_CONFIGURATION_REGISTER,deviceAddress);// Save original register settings  //
        writeWord(INA_CONFIGURATION_REGISTER,INA_RESET_DEVICE,deviceAddress); // Forces INAs to reset             //
        tempRegister = readWord(INA_CONFIGURATION_REGISTER,deviceAddress);    // Read the newly reset register    //
        if (tempRegister==INA_RESET_DEVICE ){                                 // If the register isn't reset then //
           writeWord(INA_CONFIGURATION_REGISTER,originalRegister,deviceAddress);// this is not an an INA          //
        } else {                                                              // otherwise we know it is an INA   //
          if (tempRegister==0x399F) {                                         // INA219, INA220                   //
            strcpy(ina.deviceName,"INA219");                                  // Set string                       //
            initINA219_INA220(maxBusAmps,microOhmR,_DeviceCount);             // perform initialization on device //
          } else {                                                            //                                  //
            if (tempRegister==0x4127) {                                       // INA226, INA230, INA231           //
              tempRegister = readWord(INA_DIE_ID_REGISTER,deviceAddress);     // Read the INA209 high-register    //
              if (tempRegister==INA226_DIE_ID_VALUE) {                        // We've identified an INA226       //
                strcpy(ina.deviceName,"INA226");                              // Set string                       //
                initINA226(maxBusAmps,microOhmR,_DeviceCount);                // perform initialization on device //
              } else {                                                        //                                  //
                if (tempRegister!=0) {                                        // If register exists,              //
                  strcpy(ina.deviceName,"INA230");                            // Set string                       //
                  initINA226(maxBusAmps,microOhmR,_DeviceCount);              // perform initialization on device //
                } else {                                                      //                                  //
                  strcpy(ina.deviceName,"INA231");                            // Set string                       //
                  initINA226(maxBusAmps,microOhmR,_DeviceCount);              // perform initialization on device //
                } // of if-then-else a INA230 or INA231                       //                                  //
              } // of if-then-else an INA226                                  //                                  //
            } else {                                                          //                                  //
              if (tempRegister==0x6127) {                                     // INA260                           //
                ina.type       = INA260;                                      // Set to an INA260                 //
                strcpy(ina.deviceName,"INA260");                              // Set string                       //
                /************************                                     //                                  //
                ** NOT IMPLEMENTED YET **                                     //                                  //
                ************************/                                     //                                  //
              } else {                                                        //                                  //
                if (tempRegister==0x7127) {                                   // INA3221                          //
                  ina.type       = INA3221;                                   // Set to an INA3221                //
                  strcpy(ina.deviceName,"IN3221");                            // Set string                       //
                  /************************                                   //                                  //
                  ** NOT IMPLEMENTED YET **                                   //                                  //
                  ************************/                                   //                                  //
                } else {                                                      //                                  //
                  ina.type       = UNKNOWN;                                   //                                  //
                  strcpy(ina.deviceName,"UNKNWN");                            // Set string                       //
                } // of if-then-else it is an INA3221                         //                                  //
              } // of if-then-else it is an INA260                            //                                  //
            } // of if-then-else it is an INA226, INA230, INA231              //                                  //
          } // of if-then-else it is an INA209, INA219, INA220                //                                  //
          #ifdef debug_Mode                                                   // Display values when debugging    //
            Serial.print(F("Address    = ")); Serial.println(ina.address);    //                                  //
            Serial.print(F("Type       = ")); Serial.println(ina.type);       //                                  //
            Serial.print(F("Name       = ")); Serial.println(ina.deviceName); //                                  //
            Serial.print(F("BusV_LSB   = "));Serial.println(ina.busVoltage_LSB);//                                //
            Serial.print(F("ShuntV_LSB = "));Serial.println(ina.shuntVoltage_LSB);//                              //
            Serial.print(F("current_LSB= "));Serial.println(ina.current_LSB); //                                  //
            Serial.print(F("calibration= "));Serial.println(ina.calibration); //                                  //
            Serial.print(F("power_LSB  = "));Serial.println(ina.power_LSB);   //                                  //
            Serial.print(F("Gain       = "));Serial.println(ina.programmableGain);//                              //
            Serial.print(F("Calib.Const= "));Serial.println(ina.calibConst);  //                                  //
            Serial.print(F("Power Const= "));Serial.println(ina.powerConstant);//                                 //
            Serial.print(F("\n"));                                            //                                  //
          #endif                                                              // end of conditional compile code  //
          if ((_DeviceCount*sizeof(ina))<EEPROM.length()) _DeviceCount++;     // Increment device if space avail. //
        } // of if-then-else we have an INA-Type device                       //                                  //
      } // of if-then we have a device                                        //                                  //
    } // for-next each possible I2C address                                   //                                  //
  } else {                                                                    // otherwise we need to recompute   //
    readInafromEEPROM(deviceNumber);                                          // Load EEPROM to ina structure     //
    switch (ina.type) {                                                       // Select appropriate device        //
      case INA219:initINA219_INA220(ina.maxBusAmps,ina.microOhmR,deviceNumber);break;//                           //
      case INA226:initINA226(ina.maxBusAmps,ina.microOhmR,deviceNumber);break;//                                  //
    } // of switch type                                                       //                                  //
  } // of if-then-else first call                                             //                                  //
  return _DeviceCount;                                                        // Return number of devices found   //
} // of method begin()                                                        //                                  //
/*******************************************************************************************************************
** Method initINA219_INA220 sets up the device and fills the ina-structure                                        **
*******************************************************************************************************************/
void INA_Class::initINA219_INA220(const uint8_t maxBusAmps,                   // Set up INA219 or INA220          //
                                  const uint32_t microOhmR,                   //                                  //
                                  const uint8_t deviceNumber) {               //                                  //
  ina.type                = INA219;                                           // Set to an INA219                 //
  ina.busVoltage_LSB      = INA219_BUS_VOLTAGE_LSB;                           // Set to hard-coded value          //
  ina.shuntVoltage_LSB    = INA219_SHUNT_VOLTAGE_LSB;                         // Set to hard-coded value          //
  ina.calibConst          = 4096;                                             // Device specific constant         //
  ina.powerConstant       = 20;                                               // Device specific constant         //
  ina.operatingMode       = B111;                                             // Default to continuous mode       //
  ina.current_LSB = (uint64_t)maxBusAmps * 1000000000 / 32767;                // Get the best possible LSB in nA  //
  ina.calibration = (uint64_t)ina.calibConst*(uint64_t)100000 /               // Compute calibration register     //
                    ((uint64_t)ina.current_LSB*(uint64_t)microOhmR/           // using 64 bit numbers throughout  //
                    (uint64_t)100000);                                        //                                  //
  ina.power_LSB   = (uint32_t)ina.powerConstant*ina.current_LSB;              // Fixed multiplier per device      //
  writeWord(INA_CALIBRATION_REGISTER,ina.calibration,ina.address);            // Write the calibration value      //
  // Determine optimal programmable gain so that there is no chance of an overflow yet with maximum accuracy      //
  uint16_t maxShuntmV = maxBusAmps*microOhmR/1000;                            // Compute maximum shunt millivolts //
  if      (maxShuntmV<=40)  ina.programmableGain = 0;                         // gain x1 for +- 40mV              //
  else if (maxShuntmV<=80)  ina.programmableGain = 1;                         // gain x2 for +- 80mV              //
  else if (maxShuntmV<=160) ina.programmableGain = 2;                         // gain x4 for +- 160mV             //
  else                      ina.programmableGain = 3;                         // default gain x8 for +- 320mV     //
  uint16_t tempRegister = 0x399F & INA219_CONFIG_PG_MASK;                     // Zero out the programmable gain   //
  tempRegister |= ina.programmableGain<<INA219_PG_FIRST_BIT;                  // Overwrite the new values         //
  writeWord(INA_CONFIGURATION_REGISTER,tempRegister,ina.address);             // Write new value to config reg    //
  if ((deviceNumber*sizeof(ina))<EEPROM.length())                             // If there's space left in EEPROM  //
    EEPROM.put(deviceNumber*sizeof(ina),ina);                                 // Add the structure                //
  uint16_t tempBusmV = getBusMilliVolts(deviceNumber);                        // Get the voltage on the bus       //
  if (tempBusmV > 20 && tempBusmV < 16000) {                                  // If we have a voltage             //
    bitClear(tempRegister,INA219_BRNG_BIT);                                   // set to 0 for 0-16 volts          //
    writeWord(INA_CONFIGURATION_REGISTER,tempRegister,ina.address);           // Write new value to config reg    //
  } // if-then set the range to 0-16V                                         //                                  //
  return;                                                                     // return to caller                 //
} // of method initINA219_INA220()                                            //                                  //
/*******************************************************************************************************************
** Method initINA226 sets up the device and fills the ina-structure                                               **
*******************************************************************************************************************/
void INA_Class::initINA226(const uint8_t maxBusAmps,const uint32_t microOhmR, // Set up INA226                    //
                           const uint8_t deviceNumber) {                      //                                  //
  ina.type             = INA226;                                              // Set to an INA226                 //
  ina.busVoltage_LSB   = INA226_BUS_VOLTAGE_LSB;                              // Set to hard-coded value          //
  ina.shuntVoltage_LSB = INA226_SHUNT_VOLTAGE_LSB;                            // Set to hard-coded value          //
  ina.calibConst       = 512;                                                 // Device specific constant         //
  ina.powerConstant    = 25;                                                  // Device specific constant         //
  ina.programmableGain = 0;                                                   // Programmable gain not used       //
  ina.operatingMode    = B111;                                                // Default to continuous mode       //
  ina.current_LSB      = (uint64_t)maxBusAmps * 1000000000 / 32767;           // Get the best possible LSB in nA  //
  ina.calibration      = (uint64_t)ina.calibConst*(uint64_t)100000 /          // Compute calibration register     //
                         ((uint64_t)ina.current_LSB*(uint64_t)microOhmR/      // using 64 bit numbers throughout  //
                         (uint64_t)100000);                                   //                                  //
  ina.power_LSB        = (uint32_t)ina.powerConstant*ina.current_LSB;         // Fixed multiplier per device      //
  writeWord(INA_CALIBRATION_REGISTER,ina.calibration,ina.address);            // Write the calibration value      //
  if ((deviceNumber*sizeof(ina))<EEPROM.length())                             // If there's space left in EEPROM  //
    EEPROM.put(deviceNumber*sizeof(ina),ina);                                 // Add the structure                //
  return;                                                                     // return to caller                 //
} // of method initINA226()                                                   //                                  //
/*******************************************************************************************************************
** Method getBusMicroAmps retrieves the computed current in microamps.                                            **
*******************************************************************************************************************/
int32_t INA_Class::getBusMicroAmps(const uint8_t deviceNumber) {              //                                  //
  readInafromEEPROM(deviceNumber);                                            // Load EEPROM to ina structure     //
  int32_t microAmps = readWord(INA_CURRENT_REGISTER,ina.address);             // Get the raw value                //
          microAmps = (int64_t)microAmps * ina.current_LSB / 1000;            // Convert to micro-amps            //
  return(microAmps);                                                          // return computed micro-amps       //
} // of method getBusMicroAmps()                                              //                                  //
/*******************************************************************************************************************
** Method getBusMicroWatts retrieves the computed power in milliwatts                                             **
*******************************************************************************************************************/
int32_t INA_Class::getBusMicroWatts(const uint8_t deviceNumber) {             //                                  //
  readInafromEEPROM(deviceNumber);                                            // Load EEPROM to ina structure     //
  int32_t microWatts = readWord(INA_POWER_REGISTER,ina.address);              // Get the raw value                //
          microWatts = (int64_t)microWatts * ina.power_LSB / 1000;            // Convert to milliwatts            //
  return(microWatts);                                                         // return computed milliwatts       //
} // of method getBusMicroWatts()                                             //                                  //
/*******************************************************************************************************************
** Method setBusConversion specifies the conversion rate in microseconds, rounded to the nearest valid value      **
*******************************************************************************************************************/
void INA_Class::setBusConversion(const uint32_t convTime,                     // Set timing for Bus conversions   //
                                 const uint8_t deviceNumber ) {               //                                  //
  int16_t configRegister, convRate;                                           // Store configuration register     //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);      // Get the current register         //
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA219 : if      (convTime>= 68100) convRate = 15;               //                                  //
                      else if (convTime>= 34050) convRate = 14;               //                                  //
                      else if (convTime>= 17020) convRate = 13;               //                                  //
                      else if (convTime>=  8510) convRate = 12;               //                                  //
                      else if (convTime>=  4260) convRate = 11;               //                                  //
                      else if (convTime>=  2130) convRate = 10;               //                                  //
                      else if (convTime>=  1060) convRate =  9;               //                                  //
                      else if (convTime>=   532) convRate =  8;               //                                  //
                      else if (convTime>=   276) convRate =  2;               //                                  //
                      else if (convTime>=   148) convRate =  1;               //                                  //
                      else                       convRate =  0;               //                                  //
                      configRegister &= ~INA219_CONFIG_BADC_MASK;             // zero out the averages part       //
                      configRegister |= convRate << 7;                        // shift in the BADC averages       //
                      break;                                                  //                                  //
        case INA226 : if      (convTime>= 82440) convRate = 7;                // setting depending upon range     //
                      else if (convTime>= 41560) convRate = 6;                //                                  //
                      else if (convTime>= 21160) convRate = 5;                //                                  //
                      else if (convTime>= 11000) convRate = 4;                //                                  //
                      else if (convTime>=   588) convRate = 3;                //                                  //
                      else if (convTime>=   332) convRate = 2;                //                                  //
                      else if (convTime>=   204) convRate = 1;                //                                  //
                      else                       convRate = 0;                //                                  //
                      configRegister &= ~INA226_CONFIG_BADC_MASK;             // zero out the averages part       //
                      configRegister |= convRate << 6;                        // shift in the averages to register//
                      break;                                                  //                                  //
      } // of switch type                                                     //                                  //
      writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);       // Save new value                   //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method setBusConversion()                                             //                                  //
/*******************************************************************************************************************
** Method setShuntConversion specifies the conversion rate (see datasheet for 8 distinct values) for the shunt    **
*******************************************************************************************************************/
void INA_Class::setShuntConversion(const uint32_t convTime,                   // Set timing for Bus conversions   //
                                   const uint8_t deviceNumber ) {             //                                  //
  int16_t configRegister, convRate;                                           // Store configuration register     //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);      // Get the current register         //
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA219 : if      (convTime>= 68100) convRate = 15;               //                                  //
                      else if (convTime>= 34050) convRate = 14;               //                                  //
                      else if (convTime>= 17020) convRate = 13;               //                                  //
                      else if (convTime>=  8510) convRate = 12;               //                                  //
                      else if (convTime>=  4260) convRate = 11;               //                                  //
                      else if (convTime>=  2130) convRate = 10;               //                                  //
                      else if (convTime>=  1060) convRate =  9;               //                                  //
                      else if (convTime>=   532) convRate =  8;               //                                  //
                      else if (convTime>=   276) convRate =  2;               //                                  //
                      else if (convTime>=   148) convRate =  1;               //                                  //
                      else                       convRate =  0;               //                                  //
                      configRegister &= ~INA219_CONFIG_SADC_MASK;             // zero out the averages part       //
                      configRegister |= convRate << 3;                        // shift in the BADC averages       //
                      break;                                                  //                                  //
        case INA226 : if      (convTime>= 82440) convRate = 7;                // setting depending upon range     //
                      else if (convTime>= 41560) convRate = 6;                //                                  //
                      else if (convTime>= 21160) convRate = 5;                //                                  //
                      else if (convTime>= 11000) convRate = 4;                //                                  //
                      else if (convTime>=   588) convRate = 3;                //                                  //
                      else if (convTime>=   332) convRate = 2;                //                                  //
                      else if (convTime>=   204) convRate = 1;                //                                  //
                      else                       convRate = 0;                //                                  //
                      configRegister &= ~INA226_CONFIG_SADC_MASK;             // zero out the averages part       //
                      configRegister |= convRate << 3;                        // shift in the averages to register//
                      break;                                                  //                                  //
      } // of switch type                                                     //                                  //
      writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);       // Save new value                   //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method setShuntConversion()                                           //                                  //
/*******************************************************************************************************************
** Method readWord reads 2 bytes from the specified address                                                       **
*******************************************************************************************************************/
int16_t INA_Class::readWord(const uint8_t addr,const uint8_t deviceAddr) {    //                                  //
  int16_t returnData;                                                         // Store return value               //
  Wire.beginTransmission(deviceAddr);                                         // Address the I2C device           //
  Wire.write(addr);                                                           // Send the register address to read//
  _TransmissionStatus = Wire.endTransmission();                               // Close transmission               //
  delayMicroseconds(I2C_DELAY);                                               // delay required for sync          //
  Wire.requestFrom(deviceAddr, (uint8_t)2);                                   // Request 2 consecutive bytes      //
  returnData = Wire.read();                                                   // Read the msb                     //
  returnData = returnData<<8;                                                 // shift the data over              //
  returnData|= Wire.read();                                                   // Read the lsb                     //
  return returnData;                                                          // read it and return it            //
} // of method readWord()                                                     //                                  //
/*******************************************************************************************************************
** Method writeWord writes 2 byte to the specified address                                                        **
*******************************************************************************************************************/
void INA_Class::writeWord(const uint8_t addr, const uint16_t data,            //                                  //
                          const uint8_t deviceAddr) {                         //                                  //
  Wire.beginTransmission(deviceAddr);                                         // Address the I2C device           //
  Wire.write(addr);                                                           // Send register address to write   //
  Wire.write((uint8_t)(data>>8));                                             // Write the first byte             //
  Wire.write((uint8_t)data);                                                  // and then the second              //
  _TransmissionStatus = Wire.endTransmission();                               // Close transmission               //
  delayMicroseconds(I2C_DELAY);                                               // delay required for sync          //
} // of method writeWord()                                                    //                                  //
/*******************************************************************************************************************
** Method readInafromEEPROM retrieves the device structure to the global "ina" from EEPROM                        **
*******************************************************************************************************************/
void INA_Class::readInafromEEPROM(const uint8_t deviceNumber) {               //                                  //
  static uint8_t currentINA = UINT8_MAX;                                      // Stores current INA device number //
  if (deviceNumber!=currentINA) {                                             // Only read EEPROM if necessary    //
    EEPROM.get((deviceNumber%_DeviceCount)*sizeof(ina),ina);                  // Read EEPROM values               //
    currentINA = deviceNumber;                                                // Store new current value          //
  } // of if-then we have a new device                                        //                                  //
  return;                                                                     // return nothing                   //
} // of method readInafromEEPROM()                                            //                                  //
/*******************************************************************************************************************
** Method getDeviceType retrieves the device type from EEPROM                                                     **
*******************************************************************************************************************/
uint8_t INA_Class::getDeviceType(const uint8_t deviceNumber) {                //                                  //
  readInafromEEPROM(deviceNumber);                                            // Load EEPROM to ina structure     //
  return(ina.type);                                                           // return device type number        //
} // of method getDeviceType()                                                //                                  //
/*******************************************************************************************************************
** Method getDeviceName retrieves the device name from EEPROM                                                     **
*******************************************************************************************************************/
char * INA_Class::getDeviceName(const uint8_t deviceNumber) {                 //                                  //
  readInafromEEPROM(deviceNumber);                                            // Load EEPROM to ina structure     //
  return(ina.deviceName);                                                     // return device type number        //
} // of method getDeviceName()                                                //                                  //
/*******************************************************************************************************************
** Method getBusMilliVolts retrieves the bus voltage measurement                                                  **
*******************************************************************************************************************/
uint16_t INA_Class::getBusMilliVolts(const uint8_t deviceNumber) {            //                                  //
  readInafromEEPROM(deviceNumber);                                            // Load EEPROM to ina structure     //
  uint16_t busVoltage = readWord(INA_BUS_VOLTAGE_REGISTER,ina.address);       // Get the raw value and apply      //
  if (ina.type==INA219) busVoltage = busVoltage >> 3;                         // INA219 3 are LSB unused, so shift//
  busVoltage = (uint32_t)busVoltage*ina.busVoltage_LSB/100;                   // conversion to get milliVolts     //
  if (!bitRead(ina.operatingMode,2) && bitRead(ina.operatingMode,1)) {        // If triggered mode and bus active //
    int16_t configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);// Get the current register         //
    writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);         // Write back to trigger next       //
  } // of if-then triggered mode enabled                                      //                                  //
  return(busVoltage);                                                         // return computed milliVolts       //
} // of method getBusMilliVolts()                                             //                                  //
/*******************************************************************************************************************
** Method getShuntMicroVolts retrieves the shunt voltage measurement                                              **
*******************************************************************************************************************/
int32_t INA_Class::getShuntMicroVolts(const uint8_t deviceNumber) {           //                                  //
  readInafromEEPROM(deviceNumber);                                            // Load EEPROM to ina structure     //
  int32_t shuntVoltage = readWord(INA_SHUNT_VOLTAGE_REGISTER,ina.address);    // Get the raw value                //
  shuntVoltage = shuntVoltage*ina.shuntVoltage_LSB/10;                        // Convert to microvolts            //
  if (!bitRead(ina.operatingMode,2) && bitRead(ina.operatingMode,0)) {        // If triggered and shunt active    //
    int16_t configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);// Get the current register         //
    writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);         // Write back to trigger next       //
  } // of if-then triggered mode enabled                                      //                                  //
  return(shuntVoltage);                                                       // return computed microvolts       //
} // of method getShuntMicroVolts()                                           //                                  //
/*******************************************************************************************************************
** Method reset resets the INA using the first bit in the configuration register                                  **
*******************************************************************************************************************/
void INA_Class::reset(const uint8_t deviceNumber) {                           // Reset the INA                    //
  int16_t configRegister;                                                     // Hold configuration register      //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      writeWord(INA_CONFIGURATION_REGISTER,INA_RESET_DEVICE,ina.address);     // Set most significant bit to reset//
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA219 : initINA219_INA220(ina.maxBusAmps,ina.microOhmR,i);break;//                                  //
        case INA226 : initINA226(ina.maxBusAmps,ina.microOhmR,i); break;      //                                  //
      } // of switch type                                                     //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method reset                                                          //                                  //
/*******************************************************************************************************************
** Method setMode allows the various mode combinations to be set. If no parameter is given the system goes back   **
** to the default startup mode.                                                                                   **
*******************************************************************************************************************/
void INA_Class::setMode(const uint8_t mode,const uint8_t deviceNumber ) {     // Set the monitoring mode          //
  int16_t configRegister;                                                     // Hold configuration register      //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);      // Get the current register         //
      configRegister &= ~INA_CONFIG_MODE_MASK;                                // zero out the mode bits           //
      ina.operatingMode = B00001111 & mode;                                   // Mask off unused bits             //
      EEPROM.put(i*sizeof(ina),ina);                                          // Write new EEPROM values          //
      configRegister |= ina.operatingMode;                                    // shift in the mode settings       //
      writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);       // Save new value                   //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method setMode()                                                      //                                  //
/*******************************************************************************************************************
** Method waitForConversion loops until the current conversion is marked as finished. If the conversion has       **
** completed already then the flag (and interrupt pin, if activated) is also reset.                               **
*******************************************************************************************************************/
void INA_Class::waitForConversion(const uint8_t deviceNumber) {               // Wait for current conversion      //
  uint16_t cvBits = 0;                                                        //                                  //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      cvBits = 0;                                                             //                                  //
      while(cvBits==0) {                                                      // Loop until the value is set      //
      switch (ina.type) {                                                     // Select appropriate device        //
          case INA219:cvBits=readWord(INA_BUS_VOLTAGE_REGISTER,ina.address)|2;// Bit 2 set denotes ready          //
                      readWord(INA_POWER_REGISTER,ina.address);               // Resets the "ready" bit           //
                      break;                                                  //                                  //
          case INA226:cvBits = readWord(INA_MASK_ENABLE_REGISTER,ina.address) //                                  //
                               &(uint16_t)8;                                  //                                  //
                      break;                                                  //                                  //
          default    :cvBits = 1;                                             //                                  //
        } // of switch type                                                   //                                  //
      } // of while the conversion hasn't finished                            //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method waitForConversion()                                            //                                  //
/*******************************************************************************************************************
** Method AlertOnConversion configures the INA devices which support this functionality to pull the ALERT pin low **
** when a conversion is complete. This call is ignored and returns false when called for an invalid device        **
** as the INA219 doesn't have this pin it won't work for that device.                                             **
*******************************************************************************************************************/
bool INA_Class::AlertOnConversion(const bool alertState,                      // Enable pin change on conversion  //
                                  const uint8_t deviceNumber ) {              //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = true;                                                     // Assume that this is successful   //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA226 :                                                         // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState) bitSet(alertRegister,INA_ALERT_CONVERSION_RDY_BIT); // Turn on the bit                  //
          writeWord(INA_MASK_ENABLE_REGISTER,alertRegister,ina.address);      // Write register back to device    //
        break;                                                                //                                  //
        default : returnCode = false;                                         //                                  //
      } // of switch type                                                     //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
  return(returnCode);                                                         // return the appropriate status    //
} // of method AlertOnConversion                                              //                                  //
/*******************************************************************************************************************
** Method AlertOnShuntOverVoltageConversion configures the INA devices which support this functionality to pull   **
** the ALERT pin low when the shunt current exceeds the value given in the parameter in millivolts. This call is  **
** ignored and returns false when called for an invalid device                                                    **
*******************************************************************************************************************/
bool INA_Class::AlertOnShuntOverVoltage(const bool alertState,                // Enable pin change on conversion  //
                                        const int32_t milliVolts,             //                                  //
                                        const uint8_t deviceNumber ) {        //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = true;                                                     // Assume that this is successful   //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA226 :                                                         // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState) {                                                   // If true, then also set threshold //
            bitSet(alertRegister,INA_ALERT_SHUNT_OVER_VOLT_BIT);              // Turn on the bit                  //
            uint16_t threshold = milliVolts*1000/ina.shuntVoltage_LSB;        // Compute using LSB value          //
            writeWord(INA_ALERT_LIMIT_REGISTER,threshold,ina.address);        // Write register to device         //
          } // of if we are setting a value                                   //                                  //
          writeWord(INA_MASK_ENABLE_REGISTER,alertRegister,ina.address);      // Write register back to device    //
          break;                                                              //                                  //
        default : returnCode = false;                                         //                                  //
      } // of switch type                                                     //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
  return(returnCode);                                                         // return the appropriate status    //
} // of method AlertOnShuntOverVoltage                                        //                                  //
/*******************************************************************************************************************
** Method AlertOnShuntUnderVoltageConversion configures the INA devices which support this functionality to pull  **
** the ALERT pin low when the shunt current goes below the value given in the parameter in millivolts. This call  **
** is ignored and returns false when called for an invalid device                                                 **
*******************************************************************************************************************/
bool INA_Class::AlertOnShuntUnderVoltage(const bool alertState,               // Enable pin change on conversion  //
                                         const int32_t milliVolts,            //                                  //
                                         const uint8_t deviceNumber ) {       //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = true;                                                     // Assume that this is successful   //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA226 :                                                         // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState) {                                                   // If true, then also set threshold //
            bitSet(alertRegister,INA_ALERT_SHUNT_UNDER_VOLT_BIT);             // Turn on the bit                  //
            uint16_t threshold = milliVolts*1000/ina.shuntVoltage_LSB;        // Compute using LSB value          //
            writeWord(INA_ALERT_LIMIT_REGISTER,threshold,ina.address);        // Write register to device         //
          } // of if we are setting a value                                   //                                  //
          writeWord(INA_MASK_ENABLE_REGISTER,alertRegister,ina.address);      // Write register back to device    //
          break;                                                              //                                  //
        default : returnCode = false;                                         //                                  //
      } // of switch type                                                     //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
  return(returnCode);                                                         // return the appropriate status    //
} // of method AlertOnShuntUnderVoltage                                       //                                  //
/*******************************************************************************************************************
** Method AlertOnBusOverVoltageConversion configures the INA devices which support this functionality to pull     **
** the ALERT pin low when the bus current goes aboe the value given in the parameter in millivolts. This call is  **
** ignored and returns false when called for an invalid device                                                    **
*******************************************************************************************************************/
bool INA_Class::AlertOnBusOverVoltage(const bool alertState,                  // Enable pin change on conversion  //
                                      const int32_t milliVolts,               //                                  //
                                      const uint8_t deviceNumber ) {          //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = true;                                                     // Assume that this is successful   //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA226 :                                                         // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState) {                                                   // If true, then also set threshold //
            bitSet(alertRegister,INA_ALERT_BUS_OVER_VOLT_BIT);                // Turn on the bit                  //
            uint16_t threshold = milliVolts * 100 / ina.busVoltage_LSB;       // Compute using LSB value          //
            writeWord(INA_ALERT_LIMIT_REGISTER,threshold,ina.address);        // Write register to device         //
          } // of if we are setting a value                                   //                                  //
          writeWord(INA_MASK_ENABLE_REGISTER,alertRegister,ina.address);      // Write register back to device    //
          break;                                                              //                                  //
        default : returnCode = false;                                         //                                  //
      } // of switch type                                                     //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
  return(returnCode);                                                         // return the appropriate status    //
} // of method AlertOnBusOverVoltageConversion                                //                                  //
/*******************************************************************************************************************
** Method AlertOnBusUnderVoltage configures the INA devices which support this functionality to pull              **
** the ALERT pin low when the bus current goes aboe the value given in the parameter in millivolts. This call is  **
** ignored and returns false when called for an invalid device                                                    **
*******************************************************************************************************************/
bool INA_Class::AlertOnBusUnderVoltage(const bool alertState,                 // Enable pin change on conversion  //
                                       const int32_t milliVolts,              //                                  //
                                       const uint8_t deviceNumber ) {         //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = true;                                                     // Assume that this is successful   //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA226 :                                                         // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState) {                                                   // If true, then also set threshold //
            bitSet(alertRegister,INA_ALERT_BUS_UNDER_VOLT_BIT);               // Turn on the bit                  //
            uint16_t threshold = milliVolts * 100 / ina.busVoltage_LSB;       // Compute using LSB value          //
            writeWord(INA_ALERT_LIMIT_REGISTER,threshold,ina.address);        // Write register to device         //
          } // of if we are setting a value                                   //                                  //
          writeWord(INA_MASK_ENABLE_REGISTER,alertRegister,ina.address);      // Write register back to device    //
          break;                                                              //                                  //
        default : returnCode = false;                                         //                                  //
      } // of switch type                                                     //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
  return(returnCode);                                                         // return the appropriate status    //
} // of method AlertOnBusUnderVoltage                                         //                                  //
/*******************************************************************************************************************
** Method AlertOnPowerOverLimit configures the INA devices which support this functionality to pull               **
** the ALERT pin low when the power exceeds the value set in the parameter in millivolts. This call is ignored    **
** and returns false when called for an invalid device                                                            **
*******************************************************************************************************************/
bool INA_Class::AlertOnPowerOverLimit(const bool alertState,                  // Enable pin change on conversion  //
                                      const int32_t milliAmps,                //                                  //
                                      const uint8_t deviceNumber ) {          //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = true;                                                     // Assume that this is successful   //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA226 :                                                         // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState) {                                                   // If true, then also set threshold //
            bitSet(alertRegister,INA_ALERT_POWER_OVER_WATT_BIT);              // Turn on the bit                  //
            uint16_t threshold = milliAmps * 1000000 / ina.power_LSB;         // Compute using LSB value          //
            writeWord(INA_ALERT_LIMIT_REGISTER,threshold,ina.address);        // Write register to device         //
          } // of if we are setting a value                                   //                                  //
          writeWord(INA_MASK_ENABLE_REGISTER,alertRegister,ina.address);      // Write register back to device    //
          break;                                                              //                                  //
        default : returnCode = false;                                         //                                  //
      } // of switch type                                                     //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
  return(returnCode);                                                         // return the appropriate status    //
} // of method AlertOnPowerOverLimit                                          //                                  //
/*******************************************************************************************************************
** Method setAveraging sets the hardware averaging for the different devices                                      **
*******************************************************************************************************************/
void INA_Class::setAveraging(const uint16_t averages,                         // Set the number of averages taken //
                             const uint8_t deviceNumber ) {                   //                                  //
  uint16_t averageIndex;                                                      // Store indexed value for register //
  int16_t  configRegister;                                                    // Configuration register contents  //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);      // Get the current register         //
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA219 : if      (averages>= 128) averageIndex = 15;             //                                  //
                      else if (averages>=  64) averageIndex = 14;             //                                  //
                      else if (averages>=  32) averageIndex = 13;             //                                  //
                      else if (averages>=  16) averageIndex = 12;             //                                  //
                      else if (averages>=   8) averageIndex = 11;             //                                  //
                      else if (averages>=   4) averageIndex = 10;             //                                  //
                      else if (averages>=   2) averageIndex =  9;             //                                  //
                      else                     averageIndex =  8;             //                                  //
                      configRegister &= ~INA219_CONFIG_AVG_MASK;              // zero out the averages part       //
                      configRegister |= averageIndex << 3;                    // shift in the SADC averages       //
                      configRegister |= averageIndex << 7;                    // shift in the BADC averages       //
                      break;                                                  //                                  //
        case INA226 : if      (averages>=1024) averageIndex = 7;              // setting depending upon range     //
                      else if (averages>= 512) averageIndex = 6;              //                                  //
                      else if (averages>= 256) averageIndex = 5;              //                                  //
                      else if (averages>= 128) averageIndex = 4;              //                                  //
                      else if (averages>=  64) averageIndex = 3;              //                                  //
                      else if (averages>=  16) averageIndex = 2;              //                                  //
                      else if (averages>=   4) averageIndex = 1;              //                                  //
                      else                     averageIndex = 0;              //                                  //
                      configRegister &= ~INA226_CONFIG_AVG_MASK;              // zero out the averages part       //
                      configRegister |= averageIndex << 9;                    // shift in the averages to register//
                      break;                                                  //                                  //
      } // of switch type                                                     //                                  //
      writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);       // Save new value                   //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method setAveraging()                                                 //                                  //
