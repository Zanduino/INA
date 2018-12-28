/*!
@file INA.cpp

@brief INA Class library definitions file

@section Library_definition_intro_section File Description
See the INA.h header file comments for version information. Detailed documentation for the library can be
found on the GitHub Wiki pages at https://github.com/SV-Zanshin/INA/wiki

@section header_license GNU General Public License v3.0

This program is free software : you can redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
option) any later version.This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.You should have received a copy of the GNU General Public License
along with this program(see https://github.com/SV-Zanshin/INA/blob/master/LICENSE).  If not, see
<http://www.gnu.org/licenses/>.

@section cpp_author Author

Written by Arnd\@SV-Zanshin

*/

#include "INA.h"                ///< Include the header definition
#include <Wire.h>               ///< I2C Library definition
#include <EEPROM.h>             ///< Include the EEPROM library
inaDet::inaDet(){}              ///< Constructor for structure
inaDet::inaDet(inaEEPROM inaEE) ///< Constructor from saved EEPROM values

/*!
* Class constructor using saved values from EEPROM
*/
{
  type          = inaEE.type;
  operatingMode = inaEE.operatingMode;
  address       = inaEE.address;
  maxBusAmps    = inaEE.maxBusAmps;
  microOhmR     = inaEE.microOhmR;
  switch (type)
  {
  case INA219:
    busVoltageRegister   = INA_BUS_VOLTAGE_REGISTER;
    shuntVoltageRegister = INA219_SHUNT_VOLTAGE_REGISTER;
    currentRegister      = INA219_CURRENT_REGISTER;
    busVoltage_LSB       = INA219_BUS_VOLTAGE_LSB;
    shuntVoltage_LSB     = INA219_SHUNT_VOLTAGE_LSB;
    current_LSB          = (uint64_t)maxBusAmps * 1000000000 / 32767; // Get the best possible LSB in nA
    power_LSB            = (uint32_t)20*current_LSB; // Fixed multiplier per device
    break;
  case INA226:
  case INA230:
  case INA231:
    busVoltageRegister   = INA_BUS_VOLTAGE_REGISTER;
    shuntVoltageRegister = INA226_SHUNT_VOLTAGE_REGISTER;
    currentRegister      = INA226_CURRENT_REGISTER;
    busVoltage_LSB       = INA226_BUS_VOLTAGE_LSB;
    shuntVoltage_LSB     = INA226_SHUNT_VOLTAGE_LSB;
    current_LSB          = (uint64_t)maxBusAmps * 1000000000 / 32767;
    power_LSB            = (uint32_t)25*current_LSB;
    break;
  case INA260:
    busVoltageRegister   = INA_BUS_VOLTAGE_REGISTER;
    shuntVoltageRegister = INA260_SHUNT_VOLTAGE_REGISTER; // Register not present
    currentRegister      = INA260_CURRENT_REGISTER;
    busVoltage_LSB       = INA260_BUS_VOLTAGE_LSB;
    current_LSB          = 1250000;  // Fixed LSB of 1.25mv
    power_LSB            = 10000000; // Fixed multiplier per device
    break;
  case INA3221_0:
  case INA3221_1:
  case INA3221_2:
    busVoltageRegister   = INA_BUS_VOLTAGE_REGISTER;
    shuntVoltageRegister = INA3221_SHUNT_VOLTAGE_REGISTER;
    currentRegister      = 0; // INA3221 has no current Register
    busVoltage_LSB       = INA3221_BUS_VOLTAGE_LSB;
    shuntVoltage_LSB     = INA3221_SHUNT_VOLTAGE_LSB;
    current_LSB          = 0; // INA3221 has no current register
    power_LSB            = 0; // INA3221 has no power register
    if (type==INA3221_1)
    {
      busVoltageRegister   += 2; // Register for 2nd bus voltage
      shuntVoltageRegister += 2; // Register for 2nd shunt voltage
    }
    else
    {
      if (type==INA3221_2)
      {
        busVoltageRegister   += 4; // Register for 3rd bus voltage
        shuntVoltageRegister += 4; // Register for 3rd shunt voltage
      } // of if-then INA322_2
    } // of if-then-else INA3221_1
    break;
  } // of switch type
} // of constructor
INA_Class::INA_Class()  {} ///< Unused Class constructor
INA_Class::~INA_Class() {} ///< Unused Class destructor

/*!
* Read 2 bytes from the specified I2C address. Standard I2C protocol is used, but a delay of I2C_DELAY microseconds
* has been added to let the INAxxx devices have time to get the return data ready.
*/
int16_t INA_Class::readWord(const uint8_t addr, const uint8_t deviceAddress)
{
  Wire.beginTransmission(deviceAddress);       // Address the I2C device
  Wire.write(addr);                            // Send register address to read
  Wire.endTransmission();                      // Close transmission
  delayMicroseconds(I2C_DELAY);                // delay required for sync
  Wire.requestFrom(deviceAddress, (uint8_t)2); // Request 2 consecutive bytes
  int16_t returnData  = Wire.read();           // Read the msb
  returnData  = returnData<<8;                 // shift the data over 8 bits
  returnData |= Wire.read();                   // Read the lsb
  return returnData;
} // of method readWord()

/*!
* Write 2 bytes to the specified I2C address. Standard I2C protocol is used, but a delay of I2C_DELAY microseconds
* has been added after the transmission to let the INAxxx devices have time to store the data.
*/
void INA_Class::writeWord(const uint8_t addr, const uint16_t data, const uint8_t deviceAddress)
{
  Wire.beginTransmission(deviceAddress); // Address the I2C device
  Wire.write(addr);                      // Send register address to write
  Wire.write((uint8_t)(data>>8));        // Write the first (MSB) byte
  Wire.write((uint8_t)data);             // and then the second
  Wire.endTransmission();                // Close transmission and actually send data
  delayMicroseconds(I2C_DELAY);          // delay required for sync
} // of method writeWord()

/*!
* Retrieve the stored information for a device from EEPROM. Since this method is private and access is controlled,
* no range error checking is performed
*/
void INA_Class::readInafromEEPROM(const uint8_t devNo)
{
  if (devNo !=_currentINA)// Don't bother if we already have device data
  {
    #ifdef __STM32F1__                         // STM32F1 has no built-in EEPROM
      uint16_t e = devNo *sizeof(inaEE); // it uses flash memory to emulate
      uint16_t *ptr = (uint16_t*) &inaEE;      // "EEPROM" calls are uint16_t type
      for(uint8_t n = sizeof(inaEE); n ;--n)   // Implement EEPROM.get template
      {
        EEPROM.read(e++, ptr++); // for ina (inaDet type)
      } // of for-next each byte
    #else
      EEPROM.get(devNo *sizeof(inaEE),inaEE); // Read EEPROM values to structure
    #endif
    _currentINA = devNo;
    ina = inaEE; // see inaDet constructor
  } // of if-then we have a new device
} // of method readInafromEEPROM()

/*!
* Write the current INA device structure to EEPROM
*/
void INA_Class::writeInatoEEPROM(const uint8_t devNo)
{
  inaEE = ina; // only save relevant part of ina to EEPROM
  #ifdef __STM32F1__                                // STM32F1 has no built-in EEPROM
    uint16_t e = devNo *sizeof(inaEE);              // it uses flash memory to emulate
    const uint16_t *ptr = (const uint16_t*) &inaEE; // "EEPROM" calls are uint16_t type
    for(uint8_t n = sizeof(inaEE); n ;--n)          // Implement EEPROM.put template
    {
      EEPROM.update(e++, *ptr++); // for ina (inaDet type)
    } // for-next
  #else
    EEPROM.put(devNo *sizeof(inaEE),inaEE); // Write the structure
    #ifdef ESP32
    EEPROM.commit(); // Force write to EEPROM when ESP32
    #endif
  #endif
} // of method writeInatoEEPROM()

/*!
* Set a new I2C speed. No error checking is done
*/
void INA_Class::setI2CSpeed(const uint32_t i2cSpeed )
{
  Wire.setClock(i2cSpeed);
} // of method setI2CSpeed

/*!
* Initializes the contents of the class. searches for possible devices and sets the INA Configuration details, 
* without which meaningful readings cannot be made. If it is called without the option deviceNumber parameter 
* then the settings are applied to all devices, otherwise just that specific device is targeted.
*/
uint8_t INA_Class::begin(const uint8_t maxBusAmps, const uint32_t microOhmR, const uint8_t deviceNumber )
{
  uint16_t originalRegister,tempRegister;
  if (_DeviceCount==0) // Enumerate all devices on first call
  {
    uint16_t maxDevices = 0;
    #if defined(ESP32) || defined(ESP8266)
      EEPROM.begin(512);                // If ESP32 then allocate 512 Bytes
      maxDevices = 512 / sizeof(inaEE); // and compute number of devices
    #else
      Wire.begin(); // Start I2C communications
    #endif
    #if defined(__STM32F1__)                          // Emulated EEPROM for STM32F1
      maxDevices = EEPROM.maxcount() / sizeof(inaEE); // Compute number devices possible
    #elif defined(CORE_TEENSY)                        // TEENSY doesn't have EEPROM.length
      maxDevices = 2048 / sizeof(inaEE);              // defined, so use 2Kb as value
    #else
      maxDevices = EEPROM.length() / sizeof(inaEE);   // Compute number devices possible
    #endif
    if (maxDevices > 255) // Limit number of devices to an 8-bit number
    {
      maxDevices = 255;
    } // of if-then more than 255 devices possible
    for(uint8_t deviceAddress = 0x40;deviceAddress<0x80;deviceAddress++) // Loop for each possible I2C address
    {
      Wire.beginTransmission(deviceAddress);
      if (Wire.endTransmission() == 0 && _DeviceCount < maxDevices) // If no error and EEPROM has space
      {
        originalRegister = readWord(INA_CONFIGURATION_REGISTER,deviceAddress);// Save original register settings
        writeWord(INA_CONFIGURATION_REGISTER,INA_RESET_DEVICE,deviceAddress); // Forces INAs to reset
        tempRegister = readWord(INA_CONFIGURATION_REGISTER,deviceAddress); // Read the newly reset register
        if (tempRegister==INA_RESET_DEVICE ) // If the register isn't reset then this is not an INA
        {
          writeWord(INA_CONFIGURATION_REGISTER,originalRegister,deviceAddress); // write back original value
        }
        else
        {
          if (tempRegister==0x399F)
          {
            inaEE.type = INA219;
          }
          else
          {
            if (tempRegister==0x4127) // INA226, INA230, INA231
            {
              tempRegister = readWord(INA_DIE_ID_REGISTER,deviceAddress); // Read the INA high-register
              if (tempRegister==INA226_DIE_ID_VALUE)
              {
                inaEE.type = INA226;
              }
              else
              {
                if (tempRegister!=0)
                {
                  inaEE.type = INA230;
                }
                else
                {
                  inaEE.type = INA231;
                } // of if-then-else a INA230 or INA231
              } // of if-then-else an INA226
            } else {
              if (tempRegister==0x6127)
              {
                inaEE.type = INA260;
              }
              else
              {
                if (tempRegister==0x7127)
                {
                  inaEE.type = INA3221_0;
                }
                else
                {
                  inaEE.type = INA_UNKNOWN;
                } // of if-then-else it is an INA3221
              } // of if-then-else it is an INA260
            } // of if-then-else it is an INA226, INA230, INA231
          } // of if-then-else it is an INA209, INA219, INA220
          if (inaEE.type != INA_UNKNOWN ) // Increment device if valid INA2xx
          {
            inaEE.address    = deviceAddress;
            inaEE.maxBusAmps = maxBusAmps;
            inaEE.microOhmR  = microOhmR;
            ina              = inaEE; // see inaDet constructor
            if (inaEE.type == INA3221_0 )
            {
              ina.type = INA3221_0; // Set to INA3221 1st channel
              initDevice(_DeviceCount);
              _DeviceCount = ((_DeviceCount+1)%maxDevices);
              ina.type = INA3221_1; // Set to INA3221 2nd channel
              initDevice(_DeviceCount);
              _DeviceCount = ((_DeviceCount+1)%maxDevices);
              ina.type = INA3221_2; // Set to INA3221 3rd channel
              initDevice(_DeviceCount);
              _DeviceCount = ((_DeviceCount+1)%maxDevices);
            }
            else
            {
              initDevice(_DeviceCount); // perform initialization on device
              _DeviceCount = ((_DeviceCount+1)%maxDevices); // start again at 0 if overflow
            } // of if-then inaEE.type
          } // of if-then we can add device
        } // of if-then-else we have an INA-Type device
      } // of if-then we have a device
    } // for-next each possible I2C address
  }
  else
  {
    readInafromEEPROM(deviceNumber); // Load EEPROM to ina structure
    initDevice(deviceNumber);
  } // of if-then-else first call
  _currentINA = UINT8_MAX; // Force read on next call
  return _DeviceCount;
} // of method begin()

/*!
* Initializes the given device, using the settings from the structure. This includes (re)computing the device's
* calibration values.
*/
void INA_Class::initDevice(const uint8_t deviceNumber)
{
  ina.operatingMode = INA_DEFAULT_OPERATING_MODE; // Default to continuous mode
  writeInatoEEPROM(deviceNumber);                 // Store the structure to EEPROM
  uint8_t programmableGain;
  uint16_t calibration, maxShuntmV, tempRegister, tempBusmV; // Calibration temporary variables
  switch (ina.type)
  {
    case INA219: // Set up INA219 or INA220
      // Compute calibration register
      calibration  = (uint64_t)409600000 / ((uint64_t)ina.current_LSB*(uint64_t)ina.microOhmR/(uint64_t)100000);                                       //                                  //
      writeWord(INA_CALIBRATION_REGISTER,calibration,ina.address); // Write calibration value to device register
      /* Determine optimal programmable gain so that there is no chance of an overflow yet with maximum accuracy */
      maxShuntmV = ina.maxBusAmps*ina.microOhmR/1000; // Compute maximum shunt millivolts
      if      (maxShuntmV<=40)  programmableGain = 0; // gain x1 for +- 40mV
      else if (maxShuntmV<=80)  programmableGain = 1; // gain x2 for +- 80mV
      else if (maxShuntmV<=160) programmableGain = 2; // gain x4 for +- 160mV
      else                      programmableGain = 3; // default gain x8 for +- 320mV
      tempRegister  = 0x399F & INA219_CONFIG_PG_MASK;        // Zero out the programmable gain
      tempRegister |= programmableGain<<INA219_PG_FIRST_BIT; // Overwrite the new values
      bitSet(tempRegister, INA219_BRNG_BIT);                  // set to 1 for 0-32 volts
      writeWord(INA_CONFIGURATION_REGISTER,tempRegister,ina.address); // Write new value to config register
      break;
    case INA226:
    case INA230:
    case INA231:
      // Compute calibration register
      calibration = (uint64_t)51200000 / ((uint64_t)ina.current_LSB*(uint64_t)ina.microOhmR/(uint64_t)100000);                                        //                                  //
      writeWord(INA_CALIBRATION_REGISTER,calibration,ina.address); // Write calibration value to device register
      break;
    case INA260:
    case INA3221_0:
    case INA3221_1:
    case INA3221_2:
      break;
  } // of switch type
} // of method initDevice()

/*!
* Method setBusConversion specifies the conversion rate in microseconds, rounded to the nearest valid value
*/
void INA_Class::setBusConversion(const uint32_t convTime, const uint8_t deviceNumber ) 
{
  int16_t configRegister, convRate;
  for(uint8_t i=0;i<_DeviceCount;i++) // Loop for each device found
  {
    if( deviceNumber == UINT8_MAX || deviceNumber % _DeviceCount == i ) // If this device needs setting
    {
      readInafromEEPROM(i); // Load EEPROM values to ina structure
      configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address); // Get the current register
      switch (ina.type)
      {
        case INA219 : if      (convTime>= 68100) convRate = 15;
                      else if (convTime>= 34050) convRate = 14;
                      else if (convTime>= 17020) convRate = 13;
                      else if (convTime>=  8510) convRate = 12;
                      else if (convTime>=  4260) convRate = 11;
                      else if (convTime>=  2130) convRate = 10;
                      else if (convTime>=  1060) convRate =  9;
                      else if (convTime>=   532) convRate =  8;
                      else if (convTime>=   276) convRate =  2;
                      else if (convTime>=   148) convRate =  1;
                      else                       convRate =  0;
                      configRegister &= ~INA219_CONFIG_BADC_MASK; // zero out the averages part
                      configRegister |= convRate << 7;            // shift in the BADC averages
                      break;
        case INA226 :
        case INA230 :
        case INA231 :
        case INA3221_0:
        case INA3221_1:
        case INA3221_2:
        case INA260 : if      (convTime>= 82440) convRate = 7;
                      else if (convTime>= 41560) convRate = 6;
                      else if (convTime>= 21160) convRate = 5;
                      else if (convTime>= 11000) convRate = 4;
                      else if (convTime>=   588) convRate = 3;
                      else if (convTime>=   332) convRate = 2;
                      else if (convTime>=   204) convRate = 1;
                      else                       convRate = 0;
                      if (ina.type==INA226 || ina.type==INA3221_0 || ina.type==INA3221_1 || ina.type==INA3221_2)
                      {
                        configRegister &= ~INA226_CONFIG_BADC_MASK; // zero out the averages part
                        configRegister |= convRate << 6;            // shift in the averages to register
                      } 
                      else
                      {
                        configRegister &= ~INA260_CONFIG_BADC_MASK; // zero out the averages part
                        configRegister |= convRate << 7;            // shift in the averages to register
                      } // of if-then an INA226 or INA260
                      break;
      } // of switch type
      writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address); // Save new value to device
    } // of if this device needs to be set
  } // for-next each device loop
} // of method setBusConversion()



void INA_Class::setShuntConversion(const uint32_t convTime, const uint8_t deviceNumber )
/*******************************************************************************************************************
** Method setShuntConversion specifies the conversion rate (see datasheet for 8 distinct values) for the shunt    **
*******************************************************************************************************************/
{                                                                             //                                  //
  int16_t configRegister, convRate;                                           // Store configuration register     //
  for(uint8_t i=0;i<_DeviceCount;i++) {                                       // Loop for each device found       //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i )              // If this device needs setting     //
    {                                                                         //                                  //
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
        case INA226 :                                                         // INA226,                          //
        case INA230 :                                                         // INA230,                          //
        case INA231 :                                                         // INA231 and                       //
        case INA3221_0:                                                       // INA3221                          //
        case INA3221_1:                                                       // are the same as INA260           //
        case INA3221_2:                                                       //                                  //
        case INA260 : if      (convTime>= 82440) convRate = 7;                // setting depending upon range     //
                      else if (convTime>= 41560) convRate = 6;                //                                  //
                      else if (convTime>= 21160) convRate = 5;                //                                  //
                      else if (convTime>= 11000) convRate = 4;                //                                  //
                      else if (convTime>=   588) convRate = 3;                //                                  //
                      else if (convTime>=   332) convRate = 2;                //                                  //
                      else if (convTime>=   204) convRate = 1;                //                                  //
                      else                       convRate = 0;                //                                  //
                      if (ina.type==INA226    || ina.type==INA3221_0 ||       // Select mask depending on device  //
                          ina.type==INA3221_1 || ina.type==INA3221_2) {       //                                  //
                        configRegister &= ~INA226_CONFIG_SADC_MASK;           // zero out the averages part       //
                      } else {                                                //                                  //
                        configRegister &= ~INA260_CONFIG_SADC_MASK;           // zero out the averages part       //
                      } // of if-then-else either INA226/INA3221 or a INA260  //                                  //
                      configRegister |= convRate << 3;                        // shift in the averages to register//
                      break;                                                  //                                  //
      } // of switch type                                                     //                                  //
      writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);       // Save new value                   //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method setShuntConversion()                                           //                                  //

const char* INA_Class::getDeviceName(const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method getDeviceName returns a text representation of the device name according to the device type stored in   **
** the EEPROM structure                                                                                           **
*******************************************************************************************************************/
{                                                                             //                                  //
  readInafromEEPROM(deviceNumber);                                            // Load EEPROM to ina structure     //
  switch ( ina.type )                                                         // Set value depending on type      //
  {                                                                           //                                  //
    case INA219 : return("INA219");                                           //                                  //
    case INA226 : return("INA226");                                           //                                  //
    case INA230 : return("INA230");                                           //                                  //
    case INA231 : return("INA231");                                           //                                  //
    case INA260 : return("INA260");                                           //                                  //
    case INA3221_0:                                                           //                                  //
    case INA3221_1:                                                           //                                  //
    case INA3221_2:return("INA3221");                                         //                                  //
    default:      return("UNKNOWN");                                          //                                  //
  } // of switch type                                                         //                                  //
} // of method getDeviceName()                                                //                                  //

uint16_t INA_Class::getBusMilliVolts(const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method getBusMilliVolts retrieves the bus voltage measurement                                                  **
*******************************************************************************************************************/
{                                                                             //                                  //
  uint16_t busVoltage = getBusRaw(deviceNumber);                              // Get raw voltage from device      //
  busVoltage = (uint32_t)busVoltage*ina.busVoltage_LSB/100;                   // conversion to get milliVolts     //
  return(busVoltage);                                                         // return computed milliVolts       //
} // of method getBusMilliVolts()                                             //                                  //

uint16_t INA_Class::getBusRaw(const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method getBusRaw returns the raw value from the INA device                                                     **
*******************************************************************************************************************/
{                                                                             //                                  //
  readInafromEEPROM(deviceNumber);                                            // Load EEPROM to ina structure     //
  uint16_t raw = readWord(ina.busVoltageRegister, ina.address);               // Get the raw value from register  //
  if (ina.type==INA3221_0 || ina.type==INA3221_1 || ina.type==INA3221_2 ||    //                                  //
      ina.type==INA219)                                                       //                                  //
  {                                                                           //                                  //
    raw = raw >> 3;                                                           // INA219 - 3LSB unused, so shift   //
  } // of if-then an INA3221                                                  //                                  //
  if (!bitRead(ina.operatingMode, 2) && bitRead(ina.operatingMode, 1))        // If triggered mode and bus active //
  {                                                                           //                                  //
    int16_t configRegister=readWord(INA_CONFIGURATION_REGISTER, ina.address); // Get the current register         //
    writeWord(INA_CONFIGURATION_REGISTER, configRegister, ina.address);       // Write back to trigger next       //
  } // of if-then triggered mode enabled                                      //                                  //
  return(raw);                                                                // return raw register value        //
} // of method getBusRaw()                                                    //                                  //

int32_t INA_Class::getShuntMicroVolts(const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method getShuntMicroVolts retrieves the shunt voltage measurement                                              **
*******************************************************************************************************************/
{                                                                             //                                  //
  int32_t shuntVoltage = getShuntRaw(deviceNumber);                           // Get raw readings from register   //
  if (ina.type == INA260)                                                     // INA260 has a built-in shunt      //
  {                                                                           //                                  //
    int32_t  busMicroAmps = getBusMicroAmps(deviceNumber);                    // Get the amps on the bus          //
    shuntVoltage = busMicroAmps / 200;                                        // 2mOhm resistor, Ohm's law        //
  }                                                                           //                                  //
  else                                                                        //                                  //
  {                                                                           //                                  //
    shuntVoltage = shuntVoltage * ina.shuntVoltage_LSB / 10;                  // Convert to microvolts            //
  } // of if-then-else an INA260                                              //                                  //
  if (!bitRead(ina.operatingMode,2) && bitRead(ina.operatingMode,0))          // If triggered and shunt active    //
  {                                                                           //                                  //
    int16_t configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);// Get the current register         //
    writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);         // Write back to trigger next       //
  } // of if-then triggered mode enabled                                      //                                  //
  return(shuntVoltage);                                                       // return computed microvolts       //
} // of method getShuntMicroVolts()                                           //                                  //

int16_t INA_Class::getShuntRaw(const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method getShuntRaw returns the raw register value from the device                                              **
*******************************************************************************************************************/
{                                                                             //                                  //
  int16_t raw;                                                                // Declare local variable           //
  readInafromEEPROM(deviceNumber);                                            // Load EEPROM to ina structure     //
  if (ina.type == INA260)                                                     // INA260 has a built-in shunt      //
  {                                                                           //                                  //
    int32_t  busMicroAmps = getBusMicroAmps(deviceNumber);                    // Get the amps on the bus          //
             raw          = busMicroAmps / 200 / 1000;                        // 2mOhm resistor, Ohm's law        //
  }                                                                           //                                  //
  else                                                                        //                                  //
  {                                                                           //                                  //
    raw = readWord(ina.shuntVoltageRegister, ina.address);                    // Get the raw value from register  //
    if (ina.type==INA3221_0 || ina.type==INA3221_1 || ina.type==INA3221_2)    // INA3221 doesn't use 3 LSB        //
    {                                                                         //                                  //
      if (raw | 0x8000) {                                                     // If the shunt is negative, then   //
        raw = (raw >> 3) & 0xE000;                                            // shift over 3, then 3 MSB to 1    //
      }                                                                       //                                  //
      else                                                                    //                                  //
      {                                                                       //                                  //
        raw = raw >> 3;                                                       // INA3221 - 3LSB unused, so shift  //
      } // of if-then-else we have a negative value                           //                                  //
    } // of if-then we need to shift INA3221 reading over                     //                                  //
  } // of if-then-else an INA260 with inbuilt shunt                           //                                  //
  if (!bitRead(ina.operatingMode, 2) && bitRead(ina.operatingMode, 0))        // If triggered and shunt active    //
  {                                                                           //                                  //
    int16_t configRegister=readWord(INA_CONFIGURATION_REGISTER, ina.address); // Get the current register         //
    writeWord(INA_CONFIGURATION_REGISTER, configRegister, ina.address);       // Write back to trigger next       //
  } // of if-then triggered mode enabled                                      //                                  //
  return(raw);                                                                // return raw register value        //
} // of method getShuntMicroVolts()                                           //                                  //

int32_t INA_Class::getBusMicroAmps(const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method getBusMicroAmps retrieves the computed current in microamps.                                            **
*******************************************************************************************************************/
{                                                                             //                                  //  
  readInafromEEPROM(deviceNumber);                                            // Load EEPROM to ina structure     //
  int32_t microAmps = 0;                                                      // Initialize return variable       //
  if (ina.type==INA3221_0 || ina.type==INA3221_1 || ina.type==INA3221_2) {    // INA3221 doesn't compute Amps     //
    microAmps = getShuntMicroVolts(deviceNumber) *                            // Compute and convert units        //
                ((int32_t)1000000 / (int32_t)ina.microOhmR);                  //                                  //
  }                                                                           //                                  //
  else                                                                        //                                  //
  {                                                                           //                                  //
    microAmps = (int64_t)readWord(ina.currentRegister,ina.address) *          // Convert to micro-amps            //
                ina.current_LSB / 1000;                                       //                                  //
  } // of if-then-else an INA3221                                             //                                  //
  return(microAmps);                                                          // return computed micro-amps       //
} // of method getBusMicroAmps()                                              //                                  //

int32_t INA_Class::getBusMicroWatts(const uint8_t deviceNumber) 
/*******************************************************************************************************************
** Method getBusMicroWatts retrieves the computed power in milliwatts                                             **
*******************************************************************************************************************/
{                                                                             //                                  //
  int32_t microWatts = 0;                                                     // Initialize return variable       //
  readInafromEEPROM(deviceNumber);                                            // Load EEPROM to ina structure     //
  if (ina.type==INA3221_0 || ina.type==INA3221_1 || ina.type==INA3221_2)      // INA3221 doesn't compute Amps     //
  {                                                                           //                                  //
    microWatts = (getShuntMicroVolts(deviceNumber)*1000000/ina.microOhmR) *   // compute watts = volts * amps     //
                 getBusMilliVolts(deviceNumber) / 1000;                       //                                  //
  }                                                                           //                                  //
  else                                                                        //                                  //
  {                                                                           //                                  //
    microWatts = (int64_t)readWord(INA_POWER_REGISTER,ina.address) *          // Get power register value and     //
                 ina.power_LSB / 1000;                                        // convert to milliwatts            //
  } // of if-then-else an INA3221                                             //                                  //
  return(microWatts);                                                         // return computed milliwatts       //
} // of method getBusMicroWatts()                                             //                                  //

void INA_Class::reset(const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method reset resets the INA using the first bit in the configuration register                                  **
*******************************************************************************************************************/
{                                                                             // Reset the INA                    //
   for(uint8_t i=0;i<_DeviceCount;i++)                                        // Loop for each device found       //
   {                                                                          //                                  //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i )              // If this device needs setting     //
    {                                                                         //                                  //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      writeWord(INA_CONFIGURATION_REGISTER,INA_RESET_DEVICE,ina.address);     // Set most significant bit to reset//
      initDevice(i);                                                          //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method reset                                                          //                                  //

void INA_Class::setMode(const uint8_t mode, const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method setMode allows the various mode combinations to be set. If no parameter is given the system goes back   **
** to the default startup mode.                                                                                   **
*******************************************************************************************************************/
{                                                                             // Set the monitoring mode          //
   int16_t configRegister;                                                    // Hold configuration register      //
  for(uint8_t i=0;i<_DeviceCount;i++)                                         // Loop for each device found       //
  {                                                                           //                                  //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i )              // If this device needs setting     //
    {                                                                         //                                  //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);      // Get the current register         //
      configRegister &= ~INA_CONFIG_MODE_MASK;                                // zero out the mode bits           //
      ina.operatingMode = B00000111 & mode;                                   // Mask off unused bits             //
      writeInatoEEPROM(i);                                                    // Store the structure to EEPROM    //
      configRegister |= ina.operatingMode;                                    // shift in the mode settings       //
      writeWord(INA_CONFIGURATION_REGISTER,configRegister,ina.address);       // Save new value                   //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method setMode()                                                      //                                  //

void INA_Class::waitForConversion(const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method waitForConversion loops until the current conversion is marked as finished. If the conversion has       **
** completed already then the flag (and interrupt pin, if activated) is also reset.                               **
*******************************************************************************************************************/
{                                                                             //                                  //
  uint16_t cvBits = 0;                                                        //                                  //
  for(uint8_t i=0;i<_DeviceCount;i++)                                         // Loop for each device found       //
  {                                                                           //                                  //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i )              // If this device needs setting     //
    {                                                                         //                                  //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      cvBits = 0;                                                             //                                  //
      while(cvBits==0)                                                        // Loop until the value is set      //
      {                                                                       //                                  //
        switch (ina.type)                                                     // Select appropriate device        //
        {                                                                     //                                  //
          case INA219:                                                        //                                  //
            cvBits=readWord(INA_BUS_VOLTAGE_REGISTER,ina.address) | 2;        // Bit 2 set denotes ready          //
            readWord(INA_POWER_REGISTER,ina.address);                         // Resets the "ready" bit           //
            break;                                                            //                                  //
          case INA226:                                                        //                                  //
          case INA230:                                                        //                                  //
          case INA231:                                                        //                                  //
          case INA260:                                                        //                                  //
            cvBits = readWord(INA_MASK_ENABLE_REGISTER,ina.address)&(uint16_t)8;//                                //
            break;                                                            //                                  //
          case INA3221_0:                                                     //                                  //
          case INA3221_1:                                                     //                                  //
          case INA3221_2:                                                     //                                  //
            cvBits = readWord(INA3221_MASK_REGISTER,ina.address)&(uint16_t)1; //                                  //
            break;                                                            //                                  //
          default    :cvBits = 1;                                             //                                  //
        } // of switch type                                                   //                                  //
      } // of while the conversion hasn't finished                            //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
} // of method waitForConversion()                                            //                                  //

bool INA_Class::AlertOnConversion(const bool alertState, const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method AlertOnConversion configures the INA devices which support this functionality to pull the ALERT pin low **
** when a conversion is complete. This call is ignored and returns false when called for an invalid device        **
** as the INA219 doesn't have this pin it won't work for that device.                                             **
*******************************************************************************************************************/
{                                                                             //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = false;                                                    // Assume the worst                 //
  for(uint8_t i=0;i<_DeviceCount;i++)                                         // Loop for each device found       //
  {                                                                           //                                  //
    if(deviceNumber==UINT8_MAX || deviceNumber==i )                           // If this device needs setting     //
    {                                                                         //                                  //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type)                                                       // Select appropriate device        //
      {                                                                       //                                  //
        case INA226:                                                          // Devices that have an alert pin   //
        case INA230:                                                          // Devices that have an alert pin   //
        case INA231:                                                          // Devices that have an alert pin   //
        case INA260:                                                          // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState) bitSet(alertRegister,INA_ALERT_CONVERSION_RDY_BIT); // Turn on the bit                  //
          writeWord(INA_MASK_ENABLE_REGISTER,alertRegister,ina.address);      // Write register back to device    //
          returnCode = true;                                                  //                                  //
        break;                                                                //                                  //
        default : returnCode = false;                                         //                                  //
      } // of switch type                                                     //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
  return(returnCode);                                                         // return the appropriate status    //
} // of method AlertOnConversion                                              //                                  //

bool INA_Class::AlertOnShuntOverVoltage(const bool alertState, const int32_t milliVolts, const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method AlertOnShuntOverVoltageConversion configures the INA devices which support this functionality to pull   **
** the ALERT pin low when the shunt current exceeds the value given in the parameter in millivolts. This call is  **
** ignored and returns false when called for an invalid device                                                    **
*******************************************************************************************************************/
{                                                                             //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = false;                                                    // Assume the worst                 //
  for(uint8_t i=0;i<_DeviceCount;i++)                                         // Loop for each device found       //
  {                                                                           //                                  //
    if(deviceNumber==UINT8_MAX || deviceNumber==i )                           // If this device needs setting     //
    {                                                                         //                                  //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type)                                                       // Select appropriate device        //
      {                                                                       //                                  //
        case INA226:                                                          // Devices that have an alert pin   //
        case INA230:                                                          // Devices that have an alert pin   //
        case INA231:                                                          // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState)                                                     // If true, then also set threshold //
          {                                                                   //                                  //
            bitSet(alertRegister,INA_ALERT_SHUNT_OVER_VOLT_BIT);              // Turn on the bit                  //
            uint16_t threshold = milliVolts*1000/ina.shuntVoltage_LSB;        // Compute using LSB value          //
            writeWord(INA_ALERT_LIMIT_REGISTER,threshold,ina.address);        // Write register to device         //
          } // of if we are setting a value                                   //                                  //
          writeWord(INA_MASK_ENABLE_REGISTER,alertRegister,ina.address);      // Write register back to device    //
          returnCode = true;                                                  //                                  //
          break;                                                              //                                  //
        default : returnCode = false;                                         //                                  //
      } // of switch type                                                     //                                  //
    } // of if this device needs to be set                                    //                                  //
  } // for-next each device loop                                              //                                  //
  return(returnCode);                                                         // return the appropriate status    //
} // of method AlertOnShuntOverVoltage                                        //                                  //

bool INA_Class::AlertOnShuntUnderVoltage(const bool alertState,const int32_t milliVolts, const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method AlertOnShuntUnderVoltageConversion configures the INA devices which support this functionality to pull  **
** the ALERT pin low when the shunt current goes below the value given in the parameter in millivolts. This call  **
** is ignored and returns false when called for an invalid device                                                 **
*******************************************************************************************************************/
{                                                                             //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = true;                                                     // Assume that this is successful   //
  for(uint8_t i=0;i<_DeviceCount;i++)                                         // Loop for each device found       //
  {                                                                           //                                  //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i )              // If this device needs setting     //
    {                                                                         //                                  //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type)                                                       // Select appropriate device        //
      {                                                                       //                                  //
        case INA226:                                                          // Devices that have an alert pin   //
        case INA230:                                                          // Devices that have an alert pin   //
        case INA231:                                                          // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState)                                                     // If true, then also set threshold //
          {                                                                   //                                  //
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

bool INA_Class::AlertOnBusOverVoltage(const bool alertState, const int32_t milliVolts, const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method AlertOnBusOverVoltageConversion configures the INA devices which support this functionality to pull     **
** the ALERT pin low when the bus current goes aboe the value given in the parameter in millivolts. This call is  **
** ignored and returns false when called for an invalid device                                                    **
*******************************************************************************************************************/
{                                                                             //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = true;                                                     // Assume that this is successful   //
  for(uint8_t i=0;i<_DeviceCount;i++)                                         // Loop for each device found       //
  {                                                                           //                                  //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i )              // If this device needs setting     //
    {                                                                         //                                  //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type)                                                       // Select appropriate device        //
      {                                                                       //                                  //
        case INA226:                                                          // Devices that have an alert pin   //
        case INA230:                                                          // Devices that have an alert pin   //
        case INA231:                                                          // Devices that have an alert pin   //
        case INA260:                                                          // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState)                                                     // If true, then also set threshold //
          {                                                                   //                                  //
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

bool INA_Class::AlertOnBusUnderVoltage(const bool alertState, const int32_t milliVolts, const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method AlertOnBusUnderVoltage configures the INA devices which support this functionality to pull              **
** the ALERT pin low when the bus current goes aboe the value given in the parameter in millivolts. This call is  **
** ignored and returns false when called for an invalid device                                                    **
*******************************************************************************************************************/
{                                                                             //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = true;                                                     // Assume that this is successful   //
  for(uint8_t i=0;i<_DeviceCount;i++)                                         // Loop for each device found       //
  {                                                                           //                                  //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i )              // If this device needs setting     //
    {                                                                         //                                  //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA226:                                                          // Devices that have an alert pin   //
        case INA230:                                                          // Devices that have an alert pin   //
        case INA231:                                                          // Devices that have an alert pin   //
        case INA260:                                                          // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState)                                                     // If true, then also set threshold //
          {                                                                   //                                  //
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

bool INA_Class::AlertOnPowerOverLimit(const bool alertState, const int32_t milliAmps, const uint8_t deviceNumber) 
/*******************************************************************************************************************
** Method AlertOnPowerOverLimit configures the INA devices which support this functionality to pull               **
** the ALERT pin low when the power exceeds the value set in the parameter in millivolts. This call is ignored    **
** and returns false when called for an invalid device                                                            **
*******************************************************************************************************************/
{                                                                             //                                  //
  uint16_t alertRegister;                                                     // Hold the alert register          //
  bool returnCode = true;                                                     // Assume that this is successful   //
  for(uint8_t i=0;i<_DeviceCount;i++)                                         // Loop for each device found       //
  {                                                                           //                                  //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i )              // If this device needs setting     //
    {                                                                         //                                  //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      switch (ina.type) {                                                     // Select appropriate device        //
        case INA226:                                                          // Devices that have an alert pin   //
        case INA230:                                                          // Devices that have an alert pin   //
        case INA231:                                                          // Devices that have an alert pin   //
        case INA260:                                                          // Devices that have an alert pin   //
          alertRegister = readWord(INA_MASK_ENABLE_REGISTER,ina.address);     // Get the current register         //
          alertRegister &= INA_ALERT_MASK;                                    // Mask off all bits                //
          if (alertState)                                                     // If true, then also set threshold //
          {                                                                   //                                  //
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

void INA_Class::setAveraging(const uint16_t averages, const uint8_t deviceNumber)
/*******************************************************************************************************************
** Method setAveraging sets the hardware averaging for the different devices                                      **
*******************************************************************************************************************/
{                                                                             //                                  //
  uint16_t averageIndex;                                                      // Store indexed value for register //
  int16_t  configRegister;                                                    // Configuration register contents  //
  for(uint8_t i=0;i<_DeviceCount;i++)                                         // Loop for each device found       //
  {                                                                           //                                  //
    if(deviceNumber==UINT8_MAX || deviceNumber%_DeviceCount==i )              // If this device needs setting     //
    {                                                                         //                                  //
      readInafromEEPROM(i);                                                   // Load EEPROM to ina structure     //
      configRegister = readWord(INA_CONFIGURATION_REGISTER,ina.address);      // Get the current register         //
      switch (ina.type)                                                       // Select appropriate device        //
      {                                                                       //                                  //
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
        case INA226 :                                                         //                                  //
        case INA230 :                                                         //                                  //
        case INA231 :                                                         //                                  //
        case INA3221_0:                                                       //                                  //
        case INA3221_1:                                                       //                                  //
        case INA3221_2:                                                       //                                  //
        case INA260 : if      (averages>=1024) averageIndex = 7;              // setting depending upon range     //
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
