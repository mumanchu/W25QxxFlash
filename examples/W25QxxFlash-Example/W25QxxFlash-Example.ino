/////////////////////////////////////////////////////////////////////
// Test sketch for the 32-bit W25QxxxFlash Library
// Copyright (C) mumanchu and muman.ch, 2026.10.31 (Halloween Edition)
// If you re-use this code, please include the above copyright notice
// 
// https://github/mumanchu/W25QxxFlash
// https://muman.ch/muman/index.htm?muman-matts-blog.htm
/*
This example initializes SPI and the flash chip then calls the 
comprehensive flash test method which may take a few minutes, 
depending on the memory chip size.

Modify the W25Q_xxx #defines according to your SPI pins and the 
flash chip you are using.

NOTES
- Only 32-bit 3.3V MCUs can be used!
- Tested on STM32 (Nucleo-64), SAMD51 (Arduino Zero) and ESP32 
  (ESP32-C3-DevKitC) boards.
- If you need to change something for a different board, please let 
  me know and I'll update the code.
- If the onboard LED is on the same pin as SPI_SCL (D13), then you 
  *cannot* use the onboard LED and SPI at the same time!
*/

#include <SPI.h>


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// In DEBUG mode, detect and log errors
#include "MumanchuDebug.h"

#ifdef DEBUG
// if using a hardware debugger, disable all GCC compiler optimisations
//#pragma GCC optimize ("-O0")

// Shared error logging function
void LogError(const char* msg, const char* filePath, uint line)
{
	char buf[256];
	const char* fname = strrchr(filePath, '\\');
	fname = fname ? fname + 1 : filePath;
	sprintf(buf, "ERROR: %s : %s(%u)", msg, fname, line);
	Serial.println(buf);
	Serial.flush();
}
#endif
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// SPI Definitions
// - define W25Q_SCK/MOSI/MISO if non-standard SPI pins are used
// - define W25Q_CS, W25Q_MBITS your falsh chip
// - define W25Q_SPEED accoring to your MCU baord, default is 4MHz
//   e.g. 18MHz for STM32F103, 12MHz for Arduino Zero, 42MHz for 
//   STM32F407, 50MHz for ESP32-C3, etc
//   (each SPI device on the bus can have a different speed)

#ifdef STM32F407xx
// STM32F407ZGT6 with onboard W25Q16JV
// https://stm32-base.org/boards/STM32F407ZGT6-STM32F4XX.html
// Serial TX is on PA0
#define W25Q_SCK	PB3
#define W25Q_MISO	PB4
#define W25Q_MOSI	PB5
#define W25Q_CS		PB14		// SPI chip select for the flash chip
#define W25Q_MBITS	16			// W25Q16JV
#define W25Q_SPEED	42000000	// 42MHz
#endif

#ifdef ARDUINO_NUCLEO_64
#define W25Q_SCK	D13
#define W25Q_MISO	D12
#define W25Q_MOSI	D11
#define W25Q_CS		D8
#define W25Q_SPEED	18000000	// 18MHz
#define W25Q_MBITS	64			// W25Q64 used for tests
#endif

// this is for the ESP32-C3-DevKitC-02U
// change if for other ESP32 boards
#ifdef ARDUINO_ARCH_ESP32
#define W25Q_SCK	6
#define W25Q_MISO	5
#define W25Q_MOSI	7
#define W25Q_CS		8
#define W25Q_SPEED	50000000	// 50MHz
#define W25Q_MBITS	64			// W25Q64 used for tests
#endif

#ifdef ARDUINO_SAMD_ZERO
#define W25Q_SPEED	12000000	// 12MHz
#endif

#ifndef W25Q_SPEED
#define W25Q_SPEED	4000000		// default is 4MHz
#endif

#ifndef W25Q_CS
#define W25Q_CS		8			// SPI chip select pin for the flash chip
#endif

#ifndef W25Q_MBITS
#define W25Q_MBITS	64			// W25Qxxx
#endif

// W25Qxx flash
#define W25Q_ENABLE_FLASH_TEST	// enables the test() method
#include "W25QxxFlash.h"
W25QxxFlash flash;

// Some data from the flash
byte flashData[256];


#if defined(ARDUINO_SAMD_ZERO)
// New SPI channel for the Arduino Zero so it uses the standard SPI pins 
// else SPI is on the ICSP connector and the flash test shield does not work
// https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/creating-a-new-spi
#include "wiring_private.h"  // for pinPeripheral() function
static SPIClassSAMD mySPI(&sercom1, 12, 13, 11, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_3);
#endif


// Startup
void setup() 
{
	// use different pins for Serial logging
	// this is only for the (really nice) STM32 Nucleo-64 boards
	#ifdef ARDUINO_NUCLEO_64
	Serial.setTx(PC_10);
	Serial.setRx(PC_11);
	#endif

	Serial.begin(115200);
	delay(3000);

	// PuTTY clear screen and scrollback
	// comment out if not using PuTTY
	Serial.print("\033[2J\033[H\033[3J");

	Serial.println("\n\rW25QxxFlash Started\n\r");
	Serial.flush();

	// initialize the SPI channel, depending on your MCU type

	#if defined(ARDUINO_ARCH_ESP32)
	SPI.begin(W25Q_SCK, W25Q_MISO, W25Q_MOSI);

	#elif defined(ARDUINO_SAMD_ZERO)
	// on the Zero, override SPI so we can use the standard SPI pins 11/12/13
	// else SPI is on the ICSP connector, and the flash test shield doesn't work
	mySPI.begin();
	SPI = mySPI;
	// assign pins 11, 12, 13 to SERCOM functionality
	pinPeripheral(11, PIO_SERCOM);
	pinPeripheral(12, PIO_SERCOM);
	pinPeripheral(13, PIO_SERCOM);

	#elif defined(ARDUINO_ARCH_STM32) && defined(W25Q_MISO)
	SPI.setMISO(W25Q_MISO);
	SPI.setMOSI(W25Q_MOSI);
	SPI.setSCLK(W25Q_SCK);

	#else
	// use default SPI
	SPI.begin();
	#endif

	// set the SPI speed, if errors occur it is probably too fast for your MCU
	flash.spiSettings = SPISettings(W25Q_SPEED, MSBFIRST, SPI_MODE0);

	// initialize the chip
	// set 'sizeMBits' to the size of your chip, e.g. 64 for 25Q64
	if (!flash.begin(&SPI, W25Q_CS, W25Q_MBITS)) {
		Serial.println("flash.begin() failed");
		Serial.flush();
		while (1) yield();
	}

	char buf[64];
	sprintf(buf, "Flash %uMbits %uMbytes", W25Q_MBITS, W25Q_MBITS / 8);
	Serial.println(buf);
	Serial.flush();

	// each chip should have a unique 64-bit ID
	uint64_t uniqueId;
	if (!flash.readUniqueId(&uniqueId)) {
		Serial.println("flash.readUniqueId() failed");
		Serial.flush();
	}

	// run the lengthy flash test
	// the test may take several minutes, depending on the chip size
	if (!flash.test()) {
		Serial.println("flash.test() failed");
		Serial.flush();
		while (1) yield();
	}
	Serial.println("flash.test() successful");

	// read some data from the flash
	// will be all FFs because the flash.test() erased it
	if (!flash.readData(0, flashData, sizeof(flashData))) {
		Serial.println("flash.readData() failed");
		Serial.flush();
		while (1) yield();
	}
}

void loop() 
{
	// this function intentionally left blank
}

