# Library for all Winbond W25Qxxx SPI Flash Memory Chips

Another library from _mumanchu_.

## *** PRELIMINARY ***

_If it doesn't work, please let us know..._ \
The library will be released when the example Sketch is ready.


## Description

These chips all connect to the MCU via the SPI interface.

Library for all W25Qxxx Winbond Flash memory chips, 1..512Mbits.

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

	enum RDREG { RDSTATUS1 = 0x05, RDSTATUS2 = 0x35, RDSTATUS3 = 0x15 };
	enum WRREG { WRSTATUS1 = 0x01, WRSTATUS2 = 0x31, WRSTATUS3 = 0x11 };
	bool readRegister(RDREG reg, byte* data);
	bool writeRegister(WRREG reg, byte data);

	bool test();
};
```
Refer to the commented source code for details.


## Data Sheets

WINBOND CHIP RANGE \
https://www.winbond.com/productResource-files/DA05-0006.pdf

TYPICAL DATA SHEET, W25Q512 = 64Mbytes \
https://github.com/mumanchu/mumanchu/blob/main/assets/W25QxxFlash/W25Q512-data-sheet.pdf

W25Q16 16Mbit = 2Mbytes \
https://stm32-base.org/assets/pdf/devices/W25Q16JV.pdf

Other data sheets can be found on the Winbond website \
https://www.winbond.com/hq/support/documentation


# Revision History

| Date       | Version  | Details |
|:---------- |:---------|:----------- |
| 2026.05.12 | 0.0.0	| Preliminary |

<br/>


## Joke of the Week

Humans need at least 8 hours of sleep a day, and as much as you can get at night as well.


