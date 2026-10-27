#pragma once

///////////////////////////////////////////////////////////////////////////////
// Winbond W25Qxxx SPI Serial Flash Memory
// Copyright (C) https://muman.ch + https://github.com/mumanchu, 2026.10.27
// If you re-use this software, please include the above copyright notice
// 
// https://github/mumanchu/W25QxxFlash
/*
Library for all W25Qxxx SPI Winbond chips, 1..512Mbits.

W25Q10		1Mbit		128Kbytes
W25Q20		2Mbit		256Kbytes
W25Q40		4Mbit		512Kbytes
W25Q16		16Mbit		2Mbytes
W25Q32		32Mbit		4Mbytes
W25Q64		64Mbit		8Mbytes
W25Q128		128Mbit		16Mbytes
W25Q256		256Mbit		32Mbytes
W25Q512		512Mbit		64Mbytes

WINBOND CHIP RANGE
https://www.winbond.com/productResource-files/DA05-0006.pdf

TYPICAL DATA SHEET, W25Q512 = 64Mbytes
https://github.com/mumanchu/mumanchu/blob/main/assets/W25QxxFlash/W25Q512-data-sheet.pdf

W25Q16JV 16Mbit = 2Mbytes
https://stm32-base.org/assets/pdf/devices/W25Q16JV.pdf

Other data sheets can be found on the Winbond website
https://www.winbond.com/hq/support/documentation
*/


class W25QxxFlash
{
	uint numBytes;
	uint numMBits;
	SPIClass* spi;
	uint csPin;

public:
	// check your SPI speed, max is 18MHz for STM32F103
	SPISettings spiSettings = SPISettings(18000000, MSBFIRST, SPI_MODE0);

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

protected:
	bool writeEnable(bool enable = true);
	bool doErase(byte cmd, uint address);
	void sendAndReceive(const byte* tx, byte* rx, uint length);
};


// Call this from setup()
bool W25QxxFlash::begin(SPIClass* spi, uint spiCsPin, uint sizeMbits)
{
	// size in megabits
	switch (sizeMbits) {
	case 1:				// 1..512Mbits
	case 2:
	case 4:
	case 8:
	case 16:
	case 32:
	case 64:
	case 128:
	case 256:
	case 512:
		numBytes = (1024 * 1024 * sizeMbits) / 8;
		break;
	default:
		LOGERROR("invalid size");
		return false;
	}
	this->numMBits = sizeMbits;
	this->spi = spi;

	// chip IDs are 0x11..0x20 for 1..512Mbits
	uint chipId = 0x10;
	for (uint bits = sizeMbits; bits; bits >>= 1)
		++chipId;

	// SPI chip select pin
	csPin = spiCsPin;
	pinMode(csPin, OUTPUT);
	digitalWrite(csPin, 1);

	// check the chip is responding and is the expected size
	uint manufacturer, memType, id;
	if (!readChipInfo(&manufacturer, &memType, &id)) {
		LOGERROR("no response");
		return false;
	}
	if (manufacturer != 0xef || id != chipId) {
		LOGERROR("wrong flash chip");
		return false;
	}
	return true;
}

// Soft Reset
bool W25QxxFlash::reset()
{
	// reset cannot be done while busy or suspended
	bool busy;
	if (!readBusyBit(&busy))
		return false;
	if (busy) {
		LOGERROR("Cannot reset while busy");
		return false;
	}
	byte sr2;
	if (!readRegister(RDSTATUS2, &sr2))
		return false;
	if (sr2 & 0x80) {
		LOGERROR("Cannot reset while suspended");
		return false;
	}

	// send two commands to do the reset
	const byte enableReset = 0x66;
	const byte resetDevice = 0x99;
	sendAndReceive(&enableReset, NULL, 1);
	sendAndReceive(&resetDevice, NULL, 1);

	delayMicroseconds(100);
	return true;
}


/////////////////////////////////////////////////////////////////////
// Info Methods

// Read Chip Information
// manufacturer : JEDEC manufacturer ID, 0xEF for Winbond chips
// memType      : see data sheets
// chipId       : 0x11..0x20 for 1..512Mbits
bool W25QxxFlash::readChipInfo(uint* manufacturer, uint* memType, uint* chipId)
{
	*manufacturer = 0;
	*memType = 0;
	*chipId = 0;

	// read JEDEC ID
	static const byte tx[4] = { 0x9f, 0xff, 0xff, 0xff };
	byte rx[4];
	sendAndReceive(tx, rx, 4);

	// no response
	if (*(uint32_t*)rx == 0xffffffff || *(uint32_t*)rx == 0)
		return false;

	*manufacturer = rx[1];
	*memType = rx[2];
	*chipId = rx[3];
	return true;
}

// Read the 64-bit unique chip id
bool W25QxxFlash::readUniqueId(uint64_t* uniqueId)
{
	*uniqueId = 0;

	static const byte tx1[13] = { 0x4b, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	byte rx1[13] = { 0 };
	sendAndReceive(tx1, rx1, 13);

	// ms byte is first, reverse the bytes
	byte* in = rx1 + 12;
	uint64_t uid;
	byte* out = (byte*)&uid;
	for (int i = 0; i < 8; ++i)
		*out++ = *in--;

	// all 0xff = no response
	if (uid == 0xffffffffffffffff)
		return false;
	*uniqueId = uid;

	return true;
}


/////////////////////////////////////////////////////////////////////
// Read/Write Methods

// Read data bytes, can read across sector and block boundaries
bool W25QxxFlash::readData(uint address, byte* data, uint length)
{
	ASSERT((address + length <= numBytes) && (length != 0));

	// all 0xff if it fails
	memset(data, 0xff, length);

	// fast/normal mode message is different
	byte tx[5];
	bool fastMode = spiSettings.getClockFreq() > 50000000;
	if (fastMode) {
		// fast mode for 100MHz clock
		//tx[5] = { 0x0B, address >> 16, address >> 8, address, 0xff };
		tx[0] = 0x0b;
		tx[4] = 0xff;
	}
	else {
		// normal mode 50MHz
		//tx[4] = { 0x03, address >> 16, address >> 8, address };
		tx[0] = 0x03;
	}
	tx[1] = (byte)(address >> 16);
	tx[2] = (byte)(address >> 8);
	tx[3] = (byte)address;

	const uint maxLength = 512;		// max message length
	while (length) {
		uint bytesToRead = length > maxLength ? maxLength : length;

		spi->beginTransaction(spiSettings);
		digitalWrite(csPin, 0);
		spi->transfer(tx, fastMode ? 5 : 4);
		spi->transfer(data, bytesToRead);
		digitalWrite(csPin, 1);
		spi->endTransaction();

		data += bytesToRead;
		length -= bytesToRead;
	}
	return true;
}

// Write data to flash, obeying 256 byte page boundaries to avoid wrapping
// The area to be written must be erased first, existing bytes must be 0xff
// Waits until data has been written (blocking function)
bool W25QxxFlash::writeData(uint address, const byte* data, uint length)
{
	ASSERT((address + length <= numBytes) && (length != 0));

	while (length) {
		int pageOffset = address % 256;
		int bytesToWrite = 256 - pageOffset;
		if (bytesToWrite > length)
			bytesToWrite = length;

		byte tx[4] = { 0x02, address >> 16, address >> 8, address };

		// automatic write disable after erase or program
		writeEnable();

		spi->beginTransaction(spiSettings);
		digitalWrite(csPin, 0);
		spi->transfer(tx, 4);
		spi->transfer(data, NULL, bytesToWrite);
		digitalWrite(csPin, 1);
		spi->endTransaction();

		if (!waitWhileBusy(1000)) {
			LOGERROR("writeData() timeout");
			return false;
		}

		address += bytesToWrite;
		data += bytesToWrite;
		length -= bytesToWrite;
	}
	return true;
}


/////////////////////////////////////////////////////////////////////
// Erase Methods
// After each call, poll with readBusyBit() or waitWhileBusy() 
// until erase is complete. See data sheet for worst-case timeouts.

// Erase a 4KByte sector
bool W25QxxFlash::eraseSector(uint sector)
{
	ASSERT(sector < (numBytes / 4096));
	return doErase(0x20, sector * 4096);
}

// Erase a 32KByte block
bool W25QxxFlash::eraseBlock32(uint block)
{
	ASSERT(block < (numBytes / 32768));
	return doErase(0x52, block * 32768);
}

// Erase a 64KByte block
bool W25QxxFlash::eraseBlock64(uint block)
{
	ASSERT(block < (numBytes / 65536));
	return doErase(0xd8, block * 65536);
}

// Erase the entire chip
// call waitBusy(msEraseTimeout) to find out when it's finished 
bool W25QxxFlash::eraseChip()
{
	const byte tx = 0xc7;	// or 0x60
	writeEnable();
	sendAndReceive(&tx, NULL, 1);
	return true;
}

// Is the chip all FFs?
bool W25QxxFlash::isErased()
{
	byte buf[512];
	for (uint adds = 0; adds < numBytes; adds += sizeof(buf)) {
		if (!readData(adds, buf, sizeof(buf)))
			return false;
		for (uint i = 0; i < sizeof(buf); ++i) {
			if (buf[i] != 0xff)
				return false;
		}
	}
	return true;
}

// Shared private method
bool W25QxxFlash::doErase(byte cmd, uint address)
{
	byte tx[4] = { cmd, address >> 16, address >> 8, address };
	writeEnable();
	sendAndReceive(tx, NULL, 4);
	return true;
}


/////////////////////////////////////////////////////////////////////
// Misc Methods

// Erase or write in progress? 
// Reads the BUSY bit 1 of Status Register 1
bool W25QxxFlash::readBusyBit(bool* busyBit)
{
	*busyBit = false;
	byte status1;
	if (!readRegister(RDSTATUS1, &status1))
		return false;
	*busyBit = status1 & 0x01;
	return true;
}

// Wait until erase or write is complete, with timeout in milliseconds
bool W25QxxFlash::waitWhileBusy(ulong msTimeout)
{
	ulong ticks = millis();

	while (1) {
		byte status1;
		if (!readRegister(RDSTATUS1, &status1))
			return false;
		if ((status1 & 0x01) == 0) {
			// not busy
			// WEL bit (write enable) should have been reset
			ASSERT((status1 & 0x02) == 0);
			break;
		}
		if ((millis() - ticks) > msTimeout) {
			LOGERROR("busy timeout");
			return false;
		}
		delayMicroseconds(100);
	}
	return true;
}

// Set/clear Write Enable bit WEL
// The WEL bit must be set prior to every Page Program, Sector Erase, 
// Block Erase, Chip Erase, Write Status Register and Erase/Program 
// Security Registers command.
// It is reset automatically when the command completes.
bool W25QxxFlash::writeEnable(bool enable /*=true*/)
{
	byte tx = enable ? 0x06 : 0x04;
	sendAndReceive(&tx, NULL, 1);
	return true;
}

// Read a status register
bool W25QxxFlash::readRegister(W25Q_RDREG reg, byte* data)
{
	byte tx[2] = { (byte)reg, 0xff };
	byte rx[2];
	sendAndReceive(tx, rx, 2);
	*data = rx[1];
	return true;
}

// Write a status register
bool W25QxxFlash::writeRegister(W25Q_WRREG reg, byte data)
{
	byte tx[2] = { (byte)reg, data };
	sendAndReceive(tx, NULL, 2);
	return true;
}

// Send data and get the optional returned bytes
// rx = NULL to ignore the response
void W25QxxFlash::sendAndReceive(const byte* tx, byte* rx, uint length)
{
	spi->beginTransaction(spiSettings);
	digitalWrite(csPin, 0);
	spi->transfer(tx, rx, length);
	digitalWrite(csPin, 1);
	spi->endTransaction();
}


// Patched out by default, you only need to do this once
#ifdef W25Q_ENABLE_FLASH_TEST

// THIS TEST OVERWRITES *ALL* THE DATA IN THE FLASH!
// The flash is first erased to all FFs, and verified it's all FFs.
// A random byte is written to every location. 
// Each byte is read back and verified it is correct.
// The flash is erased again and verified that it's all FFs.
// This can take a several minutes to run, depending on the chip size.
bool W25QxxFlash::test()
{
	// worst case erase time in milliseconds
	uint msEraseTimeout = (numMBits >= 256 ? 400 : numMBits * 2) * 1000;

	// data buffer, length must be power-of-2
	byte buf[512];
	const uint bufSize = sizeof(buf);

	// is the chip already erased?
	if (!isErased()) {

		#ifdef DEBUG
		Serial.println("Erasing...");
		Serial.flush();
		#endif

		// erase the entire ship
		if (!eraseChip())
			return false;
		if (!waitWhileBusy(msEraseTimeout))
			return false;
		// verify it's all FFs
		if (!isErased()) {
			LOGERROR("erase failed 1");
			return false;
		}
	}

	// write random data, buf[] bytes at a time
	#ifdef DEBUG
	Serial.println("Writing...");
	Serial.flush();
	#endif

	// use a different random number sequence every time
	uint seed = (uint)micros();

	srandom(seed);
	for (uint adds = 0; adds < numBytes; adds += bufSize) {
		#ifdef DEBUG
		sprintf((char*)buf, "%08x\r", adds);
		Serial.print((char*)buf);
		Serial.flush();
		#endif

		for (uint i = 0; i < bufSize; ++i)
			buf[i] = (byte)random();
		if (!writeData(adds, buf, bufSize))
			return false;
	}

	// read back and verify all the data
	#ifdef DEBUG
	Serial.println("Verifying...");
	Serial.flush();
	#endif

	srandom(seed);
	for (uint adds = 0; adds < numBytes; adds += bufSize) {
		#ifdef DEBUG
		sprintf((char*)buf, "%08x\r", adds);
		Serial.print((char*)buf);
		Serial.flush();
		#endif

		if (!readData(adds, buf, bufSize))
			return false;
		for (uint i = 0; i < bufSize; ++i) {
			byte b = (byte)random();
			if (buf[i] != b) {
				#ifdef DEBUG
				char s[128];
				sprintf(s, "verify failed : adds=0x%08x (%u), was %02x, sb %02x", 
					adds + i, adds + i, buf[i], b);
				LOGERROR(s);
				#endif
				return false;
			}
		}
	}

	// erase the chip again
	#ifdef DEBUG
	Serial.println("Erasing...");
	Serial.flush();
	#endif

	if (!eraseChip())
		return false;
	if (!waitWhileBusy(msEraseTimeout))
		return false;

	// verify it's all FFs
	if (!isErased()) {
		LOGERROR("erase failed 2");
		return false;
	}

	// it worked!
	return true;
}
#endif	// #ifdef W25Q_ENABLE_FLASH_TEST
