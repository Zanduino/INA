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
  inaDet ina;                                                                 // Hold device details in structure //
  uint16_t tempRegister;                                                      // Stores 16-bit register contents  //
  if (_DeviceCount==0) {                                                      // Enumerate devices in first call  //
    Wire.begin();                                                             // Start the I2C wire subsystem     //
    for(uint8_t deviceAddress = 64;deviceAddress<79;deviceAddress++) {        // Loop for each possible address   //
      Wire.beginTransmission(deviceAddress);                                  // See if something is at address   //
      if (Wire.endTransmission() == 0) {                                      // by checking the return error     //
        writeWord(INA_CONFIGURATION_REGISTER,INA_RESET_DEVICE,deviceAddress); // Forces INAs to reset             //
        delay(I2C_DELAY);                                                     // Wait for INA to finish resetting //
        tempRegister = readWord(INA_CONFIGURATION_REGISTER,deviceAddress);    // Read the newly reset register    //
        if ((tempRegister&INA_RESET_DEVICE)==0) {                             // INAs will reset MSb              //
          if (readWord(INA_MANUFACTURER_ID_REGISTER,deviceAddress)==0x5449) { // Check hard-coded manufacturerId  //
            ina.type             = INA226;                                    // Set to an INA226                 //
            ina.busVoltage_LSB   = INA226_BUS_VOLTAGE_LSB;                    // Set to hardcoded value           //
            ina.shuntVoltage_LSB = INA226_SHUNT_VOLTAGE_LSB;                  // Set to hardcoded value           //
            ina.current_LSB      = (uint64_t)maxBusAmps*1000000000/32767;     // Get the best possible LSB in nA  //
            ina.calibration      = (uint64_t)51200000/((uint64_t)ina.current_LSB*// Compute calibration register     //
                                   (uint64_t)microOhmR / (uint64_t)100000);   // using 64 bit numbers throughout  //
            ina.power_LSB        = (uint32_t)25*ina.current_LSB;              // Fixed multiplier for INA219      //

          } else {                                                            //                                  //
            ina.type             = INA219;                                    // Set to an INA219                 //
            ina.busVoltage_LSB   = INA219_BUS_VOLTAGE_LSB;                      // Set to hardcoded value           //
            ina.shuntVoltage_LSB = INA219_SHUNT_VOLTAGE_LSB;                  // Set to hardcoded value           //
            ina.current_LSB = (uint64_t)maxBusAmps*1000000000/32767;                    // Get the best possible LSB in nA  //
            ina.calibration = (uint64_t)51200000 / ((uint64_t)ina.current_LSB *         // Compute calibration register     //
                              (uint64_t)microOhmR / (uint64_t)100000);                  // using 64 bit numbers throughout  //
            ina.power_LSB   = (uint32_t)25*ina.current_LSB;                             // Fixed multiplier for INA219      //

          } // of if-then-else INA226 or INA219                               //                                  //            
          writeWord(INA_CALIBRATION_REGISTER,ina.calibration,deviceAddress);  // Write the calibration value      //
          ina.address       = deviceAddress;                                  // Store device address             //
          ina.operatingMode = B111;                                           // Default to continuous mode       //
          #ifdef debug_Mode                                                   // Display values when debugging    //
            Serial.print(F("Address    = ")); Serial.println(ina.address);    //                                  //
            Serial.print(F("Type       = ")); 
            if (ina.type==INA219) 
              Serial.println(F("INA219")); 
            else
              Serial.println(F("INA226"));       //                                  //
            Serial.print(F("BusV_LSB   = ")); Serial.println(ina.busVoltage_LSB);       //                                  //
            Serial.print(F("ShuntV_LSB = ")); Serial.println(ina.shuntVoltage_LSB);       //                                  //
            Serial.print(F("current_LSB= ")); Serial.println(ina.current_LSB);//                                  //
            Serial.print(F("calibration= ")); Serial.println(ina.calibration);//                                  //
            Serial.print(F("power_LSB  = ")); Serial.println(ina.power_LSB);  //                                  //
            Serial.print(F("\n"));                                            //                                  //
          #endif                                                              // end of conditional compile code  //
          if ((_DeviceCount*sizeof(ina))<EEPROM.length()) {                   // If there's space left in EEPROM  //
            EEPROM.put(_DeviceCount*sizeof(ina),ina);                         // Add the structure                //
            _DeviceCount++;                                                   // Increment the device counter     //
          } // of if-then the values will fit into EEPROM                     //                                  //
        } // of if-then we have an INA                                        //                                  //
      } // of if-then we have a device                                        //                                  //
    } // for-next each possible I2C address                                   //                                  //
  } // of if-then first call with no devices found                            //                                  //

/*
  if (deviceNumber==UINT8_MAX) {                                              // If default value, then set all   //
    for(uint8_t i=0;i<_DeviceCount;i++) {                                     // For each device write data       //
      EEPROM.put(i*sizeof(ina),ina);                                          // Write value to address           //
      writeWord(INA_CALIBRATION_REGISTER,ina.calibration,ina.address);        // Write the calibration value      //
    } // of for each device                                                   //                                  //
  } else {                                                                    //                                  //
    EEPROM.put((deviceNumber%_DeviceCount)*sizeof(ina),ina);                  // Write struct, cater for overflow //
    writeWord(INA_CALIBRATION_REGISTER,ina.calibration,ina.address);          // Write the calibration value      //
  } // of if-then-else set one or all devices                                 //                                  //
*/
  
  
  return _DeviceCount;                                                        // Return number of devices found   //
} // of method begin()                                                        //                                  //
/*******************************************************************************************************************
** Method getBusMicroAmps retrieves the computed current in microamps.                                            **
*******************************************************************************************************************/
int32_t INA_Class::getBusMicroAmps(const uint8_t deviceNumber) {           //                                  //
  inaDet ina;                                                                 // Hold device details in structure //
  EEPROM.get((deviceNumber%_DeviceCount)*sizeof(ina),ina);                    // Read EEPROM values               //
  int32_t microAmps = readWord(INA_CURRENT_REGISTER,ina.address);             // Get the raw value                //
//Serial.print("*");Serial.print(microAmps);Serial.print("*");
  microAmps = (int64_t)microAmps*ina.current_LSB/10000;                // Convert to microamps             //
  return(microAmps);                                                          // return computed microamps        //
} // of method getBusMicroAmps()                                              //                                  //

/*******************************************************************************************************************
** Method getBusMicroWatts retrieves the computed power in milliwatts                                             **
*******************************************************************************************************************/
int32_t INA_Class::getBusMicroWatts(const uint8_t deviceNumber) {          //                                  //
  inaDet ina;                                                                 // Hold device details in structure //
  EEPROM.get((deviceNumber%_DeviceCount)*sizeof(ina),ina);                    // Read EEPROM values               //
  int32_t microWatts = readWord(INA_POWER_REGISTER,ina.address);              // Get the raw value                //
          microWatts = (int64_t)microWatts*ina.power_LSB/1000;                // Convert to milliwatts            //
  return(microWatts);                                                         // return computed milliwatts       //
} // of method getBusMicroWatts()                                             //                                  //
/*******************************************************************************************************************
** Method setBusConversion specifies the conversion rate (see datasheet for 8 distinct values) for the bus        **
*******************************************************************************************************************/
void INA_Class::setBusConversion(uint8_t convTime,                         // Set timing for Bus conversions   //
                                    const uint8_t deviceNumber ) {            //                                  //
  inaDet ina;                                                                 // Hold device details in structure //
  EEPROM.get((deviceNumber%_DeviceCount)*sizeof(ina),ina);                    // Read EEPROM values               //
  int16_t configRegister;                                                     // Store configuration register     //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      if (convTime>7) convTime=7;                                             // Use maximum value allowed        //
      configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);      // Get the current register         //
      configRegister &= ~INA_CONFIG_BUS_TIME_MASK;                            // zero out the Bus conversion part //
      configRegister |= (uint16_t)convTime << 6;                              // shift in the averages to register//
      writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);       // Save new value                   //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method setBusConversion()                                             //                                  //
/*******************************************************************************************************************
** Method setShuntConversion specifies the conversion rate (see datasheet for 8 distinct values) for the shunt    **
*******************************************************************************************************************/
void INA_Class::setShuntConversion(uint8_t convTime,                       // Set timing for Bus conversions   //
                                      const uint8_t deviceNumber ) {          //                                  //
  inaDet ina;                                                                 // Hold device details in structure //
  EEPROM.get((deviceNumber%_DeviceCount)*sizeof(ina),ina);                    // Read EEPROM values               //
  int16_t configRegister;                                                     // Store configuration register     //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      if (convTime>7) convTime=7;                                             // Use maximum value allowed        //
      configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);      // Get the current register         //
      configRegister &= ~INA_CONFIG_SHUNT_TIME_MASK;                          // zero out the Bus conversion part //
      configRegister |= (uint16_t)convTime << 3;                              // shift in the averages to register//
      writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);       // Save new value                   //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method setShuntConversion()                                           //                                  //



/*
********************************************************************************************************************
********************************************************************************************************************
********************************************************************************************************************
********************************************************************************************************************
********************************************************************************************************************
** Refactored for both device types                                                                               **
********************************************************************************************************************
********************************************************************************************************************
********************************************************************************************************************
********************************************************************************************************************
********************************************************************************************************************
*/

/*******************************************************************************************************************
** Method readByte reads 1 byte from the specified address                                                        **
*******************************************************************************************************************/
uint8_t INA_Class::readByte(const uint8_t addr,const uint8_t deviceAddr) {    //                                  //
  Wire.beginTransmission(deviceAddr);                                         // Address the I2C device           //
  Wire.write(addr);                                                           // Send the register address to read//
  _TransmissionStatus = Wire.endTransmission();                               // Close transmission               //
  delayMicroseconds(I2C_DELAY);                                               // delay required for sync          //
  Wire.requestFrom(deviceAddr, (uint8_t)1);                                   // Request 1 byte of data           //
  return Wire.read();                                                         // read it and return it            //
} // of method readByte()                                                     //                                  //
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
** Method writeByte write 1 byte to the specified address                                                         **
*******************************************************************************************************************/
void INA_Class::writeByte(const uint8_t addr, const uint8_t data,             //                                  //
                          const uint8_t deviceAddr) {                         //                                  //
  Wire.beginTransmission(deviceAddr);                                         // Address the I2C device           //
  Wire.write(addr);                                                           // Send register address to write   //
  Wire.write(data);                                                           // Send the data to write           //
  _TransmissionStatus = Wire.endTransmission();                               // Close transmission               //
} // of method writeByte()                                                    //                                  //
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
} // of method writeWord()                                                    //                                  //
/*******************************************************************************************************************
** Method getDeviceType retrieves the device type from EEPROM                                                     **
*******************************************************************************************************************/
uint8_t INA_Class::getDeviceType(const uint8_t deviceNumber) {                //                                  //
  inaDet ina;                                                                 // Hold device details in structure //
  EEPROM.get((deviceNumber%_DeviceCount)*sizeof(ina),ina);                    // Read EEPROM values               //
  return(ina.type);                                                           // return device type number        //
} // of method getDeviceType()                                                //                                  //
/*******************************************************************************************************************
** Method getBusMilliVolts retrieves the bus voltage measurement                                                  **
*******************************************************************************************************************/
uint16_t INA_Class::getBusMilliVolts(const bool    waitSwitch,                //                                  //
                                     const uint8_t deviceNumber) {            //                                  //
  inaDet ina;                                                                 // Hold device details in structure //
  EEPROM.get((deviceNumber%_DeviceCount)*sizeof(ina),ina);                    // Read EEPROM values               //
  if (waitSwitch) waitForConversion();                                        // wait for conversion to complete  //
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
int32_t INA_Class::getShuntMicroVolts(const bool waitSwitch,                  //                                  //
                                      const uint8_t deviceNumber) {           //                                  //
  inaDet ina;                                                                 // Hold device details in structure //
  EEPROM.get((deviceNumber%_DeviceCount)*sizeof(ina),ina);                    // Read EEPROM values               //
  if (waitSwitch) waitForConversion();                                        // wait for conversion to complete  //
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
  inaDet ina;                                                                 // Hold device details in structure //
  int16_t configRegister;                                                     // Hold configuration register      //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      EEPROM.get(i*sizeof(ina),ina);                                          // Read EEPROM values               //
      writeWord(INA_CONFIGURATION_REGISTER,0x8000,ina.address);               // Set most significant bit         //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
  delay(I2C_DELAY);                                                           // Allow time to reset device       //
} // of method reset                                                          //                                  //
/*******************************************************************************************************************
** Method getMode returns the current monitoring mode of the device selected                                      **
*******************************************************************************************************************/
uint8_t INA_Class::getMode(const uint8_t deviceNumber ) {                     // Return the monitoring mode       //
  inaDet ina;                                                                 // Hold device details in structure //
  uint8_t tempDevice = deviceNumber+1;                                        // Temporary device number storage  //
  EEPROM.get(tempDevice*sizeof(ina),ina);                                     // Read EEPROM values               //
  return(ina.operatingMode);                                                  // Return stored value              //
} // of method getMode()                                                      //                                  //
/*******************************************************************************************************************
** Method setMode allows the various mode combinations to be set. If no parameter is given the system goes back   **
** to the default startup mode.                                                                                   **
*******************************************************************************************************************/
void INA_Class::setMode(const uint8_t mode,const uint8_t deviceNumber ) {     // Set the monitoring mode          //
  inaDet ina;                                                                 // Hold device details in structure //
  int16_t configRegister;                                                     // Hold configuration register      //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      EEPROM.get(i*sizeof(ina),ina);                                          // Read EEPROM values               //
      configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);      // Get the current register         //
      configRegister &= ~INA_CONFIG_MODE_MASK;                                // zero out the mode bits           //
      ina.operatingMode = B00001111 & mode;                                   // Mask off unused bits             //
      EEPROM.put((i%_DeviceCount)*sizeof(ina),ina);                           // Write new EEPROM values          //
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
  inaDet ina;                                                                 // Hold device details in structure //
  EEPROM.get((deviceNumber%_DeviceCount)*sizeof(ina),ina);                    // Read EEPROM values               //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      cvBits = 0;                                                             //                                  //
      while(cvBits==0) {                                                      // Loop until the value is set      //
        if (ina.type=INA219)                                                  // INA219 and INA226 are different  //
          cvBits = readWord(INA_BUS_VOLTAGE_REGISTER,ina.address)|2;          // Bit 2 set denotes ready          //
        else                                                                  //                                  //
          cvBits = readWord(INA_MASK_ENABLE_REGISTER,ina.address)&(uint16_t)8;//                                  //
      } // of while the conversion hasn't finished                            //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method waitForConversion()                                            //                                  //
/*******************************************************************************************************************
** Method setAlertPinOnConversion configure the INA226 to pull the ALERT pin low when a conversion is complete. As**
** the INA219 doesn't have this pin it won't work for that chip.                                                  **
*******************************************************************************************************************/
void INA_Class::setAlertPinOnConversion(const bool alertState,                // Enable pin change on conversion  //
                                        const uint8_t deviceNumber ) {        //                                  //
  inaDet ina;                                                                 // Hold device details in structure //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      EEPROM.get(i*sizeof(ina),ina);                                          // Read EEPROM values               //
      if ( ina.type = INA226) {                                               // Only set register for INA226s    //
        alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);       // Get the current register         //
        if (!alertState) alertRegister &= ~((uint16_t)1<<10);                 // zero out the alert bit           //
                    else alertRegister |= (uint16_t)(1<<10);                  // turn on the alert bit            //
        writeWord(INA_MASK_ENABLE_REGISTER,alertRegister,ina.address);        // Write register back to device    //
      } // of if-then we have an INA226                                       //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method setAlertPinOnConversion                                        //                                  //
/*******************************************************************************************************************
** Method setAveraging sets the hardware averaging for the different devices                                      **
*******************************************************************************************************************/
void INA_Class::setAveraging(const uint16_t averages,                         // Set the number of averages taken //
const uint8_t deviceNumber ) {                   //                                  //
  uint8_t averageIndex;                                                       // Store indexed value for register //
  int16_t configRegister;                                                     // Configuration register contents  //
  inaDet ina;                                                                 // Hold device details in structure //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i ) {            // If this device needs setting     //
      EEPROM.get(i*sizeof(ina),ina);                                          // Read EEPROM values               //
      configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);      // Get the current register         //
      if (ina.type=INA219) {                                                  // Settings different for INA219|226//
        if      (averages>= 128) averageIndex = 15;                           //                                  //
        else if (averages>=  64) averageIndex = 14;                           //                                  //
        else if (averages>=  32) averageIndex = 13;                           //                                  //
        else if (averages>=  16) averageIndex = 12;                           //                                  //
        else if (averages>=   8) averageIndex = 11;                           //                                  //
        else if (averages>=   4) averageIndex = 10;                           //                                  //
        else if (averages>=   2) averageIndex =  9;                           //                                  //
        else                     averageIndex =  8;                           //                                  //
        configRegister &= ~INA219_CONFIG_AVG_MASK;                            // zero out the averages part       //
        configRegister |= (uint16_t)averageIndex << 7;                        // shift in the averages to register//
        } else {                                                                //                                  //
        if      (averages>=1024) averageIndex = 7;                            // setting depending upon range     //
        else if (averages>= 512) averageIndex = 6;                            //                                  //
        else if (averages>= 256) averageIndex = 5;                            //                                  //
        else if (averages>= 128) averageIndex = 4;                            //                                  //
        else if (averages>=  64) averageIndex = 3;                            //                                  //
        else if (averages>=  16) averageIndex = 2;                            //                                  //
        else if (averages>=   4) averageIndex = 1;                            //                                  //
        else                     averageIndex = 0;                            //                                  //
        configRegister &= ~INA226_CONFIG_AVG_MASK;                            // zero out the averages part       //
        configRegister |= (uint16_t)averageIndex << 9;                        // shift in the averages to register//
      } // of if-then we have an INA219                                       //                                  //
      writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);       // Save new value                   //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method setAveraging()                                                 //                                  //
