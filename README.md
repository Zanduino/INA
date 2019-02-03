<img src="images/horizontal_narrow.png" alt="INA" align="right" height="40px">

# INA2*xx* Devices<br>[![Build Status](https://travis-ci.org/SV-Zanshin/INA.svg?branch=master)](https://travis-ci.org/SV-Zanshin/INA) [![DOI](https://zenodo.org/badge/136879464.svg)](https://zenodo.org/badge/latestdoi/136879464) [![arduino-library-badge](https://www.ardu-badge.com/badge/INA2xx.svg?)](https://www.ardu-badge.com/INA2xx)
_Arduino_ library to access multiple INA2xx High-Side/Low-Side Bi-Directional I2C Current and Power Monitors at the same time.  Details of the library methods and example programs are to be found [at the INA wiki pages](https://github.com/SV-Zanshin/INA/wiki).

Texas Instruments produces this family of power monitors and the library supports the following devices:

| Device                                      | Max V | Package   | Shunt mV | Description | Tested |
| ------------------------------------------- | ------| --------- | -------- |------------ | ------ |
| [INA219](http://www.ti.com/product/INA219) ([datasheet](http://www.ti.com/lit/ds/symlink/ina219.pdf))  | 26V   | SOT-23 8p | ±40,±80,±160,±320mV |            | Yes |
| [INA220](http://www.ti.com/product/INA220) ([datasheet](http://www.ti.com/lit/ds/symlink/ina220.pdf)) | 26V   | VSSOP 10p | ±40,±80,±160,±320mV | identical to INA219  | - |
| [INA220-Q1](http://www.ti.com/product/INA220-Q1) ([datasheet](http://www.ti.com/lit/ds/symlink/ina220-Q1.pdf)) | 26V   | VSSOP 10p | ±40,±80,±160,±320mV | Identical to INA219 | - |
| [INA226](http://www.ti.com/product/INA226) ([datasheet](http://www.ti.com/lit/ds/symlink/ina226.pdf)) | 36V | VSSOP 10p | ±81.92mV |            | Yes |
| [INA230](http://www.ti.com/product/INA230) ([datasheet](http://www.ti.com/lit/ds/symlink/ina230.pdf)) | 28V | QFN 16p | ±81.92mV | Identical to INA226 | - |
| [INA231](http://www.ti.com/product/INA231) ([datasheet](http://www.ti.com/lit/ds/symlink/ina231.pdf)) | 28V | DSBGA-12 | ±81.92mV | Identical to INA226 | - |
| [INA260](http://www.ti.com/product/INA260) ([datasheet](http://www.ti.com/lit/ds/symlink/ina260.pdf)) | 36V | TSSOP 16p | n.a. | 2 mΩ shunt, ±15A             | Yes |
| [INA3221](http://www.ti.com/product/INA3221) ([datasheet](http://www.ti.com/lit/ds/symlink/ina3221.pdf)) | 26V | VQFN(16) | ±163.8mV | 3 concurrent circuits | Yes |

Texas Instruments has a document which documents and details additional differences between the various INA-devices, this PDF document can be read at [Digital Interfaces for Current Sensing Devices](http://www.ti.com/lit/an/sboa203/sboa203.pdf)
## Hardware layout
<img src="https://github.com/SV-Zanshin/INA/blob/master/images/INA226.jpg" width="175" align="left"/>The packages are small and a lot of work to solder, but fortunately there are now several sources for breakout boards for the various devices which are worth it in time savings. My first test with a INA226 involved a blank breakout board, some solder paste, a frying pan, desoldering braid, a magnifying glass and quite a bit of time to set up the first breadboard.
## Library description
The library locates all INA2xx devices on the I2C chain. Each unit can be individually configured with 2 setup parameters describing the expected maximum voltage, shunt/resistor values which then set the internal configuration registers is ready to begin accurate measurements.  The details of how to setup the library along with all of the publicly available methods can be found on the [INA wiki pages](https://github.com/SV-Zanshin/INA/wiki).

Great lengths have been taken to avoid the use of floating point in the library. This keeps the library size down because floating point support doesn't have to be compiled into the code. The results are returned as 32-bit integersTo keep the original level of precision without loss but to allow the full range of voltages and amperes to be returned the amperage .

Since the functionality differs between the supported devices there are some functions which will only work for certain devices.

## Documentation
The documentation has been done using Doxygen and can be found at [doxygen documentation](https://sv-zanshin.github.io/INA/html/index.html)

<img src="https://github.com/SV-Zanshin/INA/blob/master/images/horizontal_narrow.png" alt="INA" align="left" height="75px">
<img src="https://www.sv-zanshin.com/r/images/site/gif/zanshintext.gif" align="right" width="75"/><img src="https://www.sv-zanshin.com/r/images/site/gif/zanshinkanjitiny.gif" align="right">
