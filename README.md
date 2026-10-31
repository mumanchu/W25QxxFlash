# Arduino Library for SPI Flash Memory Chips

> [!IMPORTANT]
> **This library only works with 3.3V 32-bit processors. The flash chips require 3.3V, they will be damaged if connected to 5V!**

![W25Qxx Breakout Board](https://github.com/mumanchu/mumanchu/blob/main/assets/W25QxxFlash/w25qxx-flash.jpg)


## Description

Library for 3.3V SPI Flash memory chips, 1..512Mbits. These chips all connect to the MCU via an SPI interface. 

The library has been tested on STM32 Nucleo, Espressif ESP32 and Arduino Zero SAMD51 boards. But each MCU board has different idiosyncrasies, so please let me know if you find a problem and I'll fix it ASAP.

Here are the WinBond chips, but it should work with SPI flash memories from most manufacturers.

- W25Q10		1Mbit		128Kbytes
- W25Q20		2Mbit		256Kbytes
- W25Q40		4Mbit		512Kbytes
- W25Q16		16Mbit		2Mbytes
- W25Q32		32Mbit		4Mbytes
- W25Q64		64Mbit		8Mbytes
- W25Q128		128Mbit		16Mbytes
- W25Q256		256Mbit		32Mbytes
- W25Q512		512Mbit		64Mbytes

All the chips use the same API.

## Class Reference

For more information, open the [source file](src/W25QxxFlash.h) and view the comments for each method.

```cpp
class W25QxxFlash
{
public:
    // use the max. SPI speed for your MCU
    SPISettings spiSettings = SPISettings(4000000, MSBFIRST, SPI_MODE0);

    bool begin(SPIClass* spi, uint spiCsPin, uint sizeMbits);
    bool reset();

    bool readChipInfo(uint* manufacturer, uint* memType, uint* chipId);
    bool readUniqueId(uint64_t* uniqueId);
    bool readBusyBit(bool* busyBit);
    bool waitWhileBusy(ulong msTimeout);

    bool readData(uint address, byte* data, uint length);
    bool writeData(uint address, const byte* data, uint length);

    bool eraseSector(uint sector);
    bool eraseBlock32(uint block);
    bool eraseBlock64(uint block);
    bool eraseChip();
    bool isErased();

    enum W25Q_RDREG { RDSTATUS1 = 0x05, RDSTATUS2 = 0x35, RDSTATUS3 = 0x15 };
    enum W25Q_WRREG { WRSTATUS1 = 0x01, WRSTATUS2 = 0x31, WRSTATUS3 = 0x11 };
    bool readRegister(W25Q_RDREG reg, byte* data);
    bool writeRegister(W25Q_WRREG reg, byte data);

    #ifdef W25Q_ENABLE_FLASH_TEST
    bool test();
    #endif
};
```

The `test()` method is only available if you add `#define W25Q_ENABLE_FLASH_TEST` before `#include "W25QxxFlash.h"`. See the example sketch.

## Example Sketch

To run the example sketch you will need a board with a fitted W25Qxx chip, or a breakout board with a Winbond chip connected to SPI's W25Q_CS, W25Q_SCK, W25Q_MOSI and W25Q_MISO pins (plus 3.3V and GND). 

The SPI channel must be configured for the flash memory chip and the SPI channel pins. These are different for each board. The example sketch contains definitions for some common baords. See the `#define W25Q_xxx` definitions.

The library has been tested with SMT32 Nucleo-64, ESP32-C3 and Arduino Zero SAMD51 boards.

These all worked well, but SPI speeds are limited by the MCU. _If the example sketch fails, reduce the SPI clock speed in `spiSettings` and try again._

The example sketch doesn't do much. It initializes SPI then calls the comprehensive flash test method which takes several minutes, depending on your chip size.

> [!NOTE]
> If using the Arduino Serial Monitor, '\r' will always do a line feed so you will see the addresses scrolling up instead of being refreshed on the same line. That's why I use the antique 'PuTTY' serial monitor.

For the original tests, I used a very nice STM32F407ZGT6 board which I bought on AliExpress for CHF11.50 (ECBuying store). This STM32 board is recommended. It has a nice STM32F407ZGT6 MCU (32-bit Cortex M4 with FPU), a 16Mbit W25Q16 SPI flash chip, an SD card which is also on the SPI bus, a battery-backed RTC, and a full SWD/JTAG debug connector :-) 

Here are some hard-to-find technical details of this board, including a schematic, \
https://stm32-base.org/boards/STM32F407ZGT6-STM32F4XX.html

With the STM32F407ZGT6 board, debugging was done with the ST-LINK V2.1 half of a Nucleo-64 board connected via the SWD pins. This is a great combination.

![STM32F407ZGT6 and ST-LINK](https://github.com/mumanchu/mumanchu/blob/main/assets/W25QxxFlash/stm32f407zgt6-debug.jpg)


## Fast Read Modes

Many of these chips support enhanced SPI communications called Dual and Quad modes, which can increase the read data rate by up to 6 times. This library does not support these modes. However, it does support 'fast read' mode which runs SPI at up to 100MHz (only for STM32 and ESP32, see the `readData()` method). 

The _write_ speeds are limited by the chip itself, see `waitWhileBusy()`, so having ultra-fast read rates is only significant if you are streaming video or fast audio data. For that you may need a different library.


## Using an SPI library with the Arduino Zero

By default, the SPI pins (MISO, MOSI and SCK) on an Arduino zero are on the ICSP connector. The standard Arduino SPI pins do not work. This means that SPI shields _will not work_ on the Arduino Zero, including the Arduino shield I made to test the flash chips.

However, you can configure the Zero to use the standard Arduino SPI pins (SCK=13, MOSI=12, MISO=11) by creating a new SPI channel. You will see this code in the example sketch.

Add these lines after `#include <SPI.h>`,

```cpp
#if defined(ARDUINO_SAMD_ZERO)
// New SPI channel for the Arduino Zero so it uses the standard SPI pins 
// else SPI is on the ICSP connector and the flash test shield does not work
// https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/creating-a-new-spi
#include "wiring_private.h"  // for pinPeripheral() function
static SPIClassSAMD mySPI(&sercom1, 12, 13, 11, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_3);
#endif
```

Add these lines to `setup()`, before `flash.begin()` is called,

```cpp
  #if defined(ARDUINO_SAMD_ZERO)
  // on the Zero, override SPI so we can use the standard SPI pins 11/12/13
  // else SPI is on the ICSP connector, and the flash test shield doesn't work
  mySPI.begin();
  SPI = mySPI;
  // assign pins 11, 12, 13 to SERCOM functionality
  pinPeripheral(11, PIO_SERCOM);
  pinPeripheral(12, PIO_SERCOM);
  pinPeripheral(13, PIO_SERCOM);
  #endif
```

## Future Updates

Some enhancements may be made to this library, if required by future applications. (Or you can do this for homework :-)

* Non-blocking write data and erase procedures.
* 'Verify' option for `writeData()` and `erase()` methods. 


## Data Sheets

WINBOND CHIP RANGE \
https://www.winbond.com/productResource-files/DA05-0006.pdf

TYPICAL DATA SHEET, W25Q512 = 512Mbits = 64Mbytes \
https://github.com/mumanchu/mumanchu/blob/main/assets/W25QxxFlash/W25Q512JV.pdf

W25Q16 16Mbit = 2Mbytes \
https://stm32-base.org/assets/pdf/devices/W25Q16JV.pdf

Other data sheets can be found on the Winbond website \
https://www.winbond.com/hq/support/documentation

If you want to learn about SPI, this is a great introduction, \
https://www.analog.com/en/resources/analog-dialogue/articles/introduction-to-spi-interface.html


# Revision History

| Date       | Version  | Details |
|:---------- |:---------|:----------- |
| 2026.10.31 | 1.0.4    | Updated for ESP32s, improved example sketch |
| 2026.10.27 | 1.0.3    | Improvements after testing with new chips, see github Releases comment |
| 2026.07.17 | 1.0.2    | Cosmetic updates (Title, README, example comments) |
| 2026.05.16 | 1.0.1	| Correct include file name in library.properties |
| 2026.05.16 | 1.0.0	| First release |

<br/>


## Joke of the Week

Humans need at least 8 hours of sleep a day, and as much as they can get at night as well.
