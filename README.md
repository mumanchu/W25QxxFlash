# Library for all Winbond W25Qxxx SPI Flash Memory Chips

**This version of the library is only suitable for 32-bit processors. It has been tested on the STM32.**

## Description

Library for all W25Qxxx Winbond Flash memory chips, 1..512Mbits. \
These chips all connect to the MCU via the SPI interface.

- W25Q10		1Mbit		128Kbytes
- W25Q20		2Mbit		256Kbytes
- W25Q40		4Mbit		512Kbytes
- W25Q16		16Mbit		2Mbytes
- W25Q32		32Mbit		4Mbytes
- W25Q64		64Mbit		8Mbytes
- W25Q128		128Mbit		16Mbytes
- W25Q256		256Mbit		32Mbytes
- W25Q512		512Mbit		64Mbytes

All the chips have the same API.

These are the methods:
```cpp
class W25QxxFlash
{
	bool begin(SPIClass* spi, uint spiCsPin, uint sizeMbits);
	bool reset();

	bool readChipInfo(uint* manufacturer, uint* memType, uint* id);
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

	bool test();
};
```
Refer to the commented source code for details. The `test()` method can be used as an example.

## Example Sketch

To run the example sketch you will need a board with a fitted W25Qxx chip. I used a very nice STM32F407ZGT6 board which I bought on Aliexpress for CHF11.50 (ECBuying store). This STM32 board is recommended. It has a 16Mbit W25Q16 SPI flash chip, an SD card which is also on the SPI bus, a battery-backed RTC, and a full SWD/JTAG debug connector :-)

Here are some hard-to-find technical details of this board, including a schematic \
https://stm32-base.org/boards/STM32F407ZGT6-STM32F4XX.html

The example sketch doesn't do much. It initializes SPI then calls the comprehensive flash test method which takes about 4 minutes 22 seconds.

Debugging was done with the ST-LINK V2.1 half of a Nucleo-64 board connected via the SWD pins. This is a great combination.

![STM32F407ZGT6 and ST-LINK](https://github.com/mumanchu/mumanchu/blob/main/assets/W25QxxFlash/stm32f407zgt6-debug.jpg)


## Data Sheets

WINBOND CHIP RANGE \
https://www.winbond.com/productResource-files/DA05-0006.pdf

TYPICAL DATA SHEET, W25Q512 = 64Mbytes \
https://github.com/mumanchu/mumanchu/blob/main/assets/W25QxxFlash/W25Q512JV.pdf

W25Q16 16Mbit = 2Mbytes \
https://stm32-base.org/assets/pdf/devices/W25Q16JV.pdf

Other data sheets can be found on the Winbond website \
https://www.winbond.com/hq/support/documentation


# Revision History

| Date       | Version  | Details |
|:---------- |:---------|:----------- |
| 2026.05.16 | 1.0.0	| First release |
| 2026.05.16 | 1.0.1	| Correct include file name in library.properties |

<br/>


## Joke of the Week

Humans need at least 8 hours of sleep a day, and as much as they can get at night as well.


