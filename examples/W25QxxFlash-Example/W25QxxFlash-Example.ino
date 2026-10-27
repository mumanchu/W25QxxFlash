/////////////////////////////////////////////////////////////////////
// Test sketch for the 32-bit W25QxxxFlash Library
// mumanchu and muman.ch, 2026.10.27
// 
// https://github/mumanchu/W25QxxFlash
// https://muman.ch/muman/index.htm?muman-matts-blog.htm
/*
It doesn't do much. It initializes SPI then calls the comprehensive 
flash test method which takes several mnutes depending on the memory 
size.

This code was developed for a nice STM32F407ZGT6 board which I bought 
on Aliexpress for CHF11.50 (ECBuying). This board is recommended.
It has a 16Mbit W25Q16 flash chip, and a full SWD/JTAG debug connector.

Here are some hard-to-find technical details of this board:
https://stm32-base.org/boards/STM32F407ZGT6-STM32F4XX.html
*/

#include <SPI.h>

typedef unsigned long ulong;
typedef unsigned int uint;


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


// SPI pins
// these definitions are not needed if you're using the standard pins
#define SPI_SCK			D13
#define SPI_MISO		D12
#define SPI_MOSI		D11

// SPI chip select for the flash chip
#define FLASH_CS		D8

// Onboard LED
// TAKE CARE! The LED is often on the same pin as SPI_SCL (D13)
// you cannot use the LED and SPI at the same time!
//#define LED_BUILTIN		D13
#undef LED_BUILTIN

// W25Q16 16Mbit flash chip
#define W25Q_ENABLE_FLASH_TEST		// enables the test() method
#include "W25QxxFlash.h"
W25QxxFlash flash;

// Some data from the flash
byte flashData[1024];


// Startup
void setup() 
{
	// Use different pins for Serial logging
	// this is only for the STM32 Nucleo-64 boards
	#ifdef ARDUINO_NUCLEO_64
	Serial.setTx(PC_10);
	Serial.setRx(PC_11);
	#endif

	Serial.begin(115200);
	delay(3000);

	// PuTTY clear screen and scrollback
	// comment out if not using PuTTY
	Serial.print("\033[2J\033[H\033[3J");

	Serial.println("\n\rStarted\n\r");
	Serial.flush();

	// 'int' must be 32 bits
	if (sizeof(int) != 4) {
		LOGERROR("requires 32-bit MCU");
		while(1) yield();
	}

	// TAKE CARE! The LED is often on the same pin as SPI_SCL (D13)
	// you cannot use the LED and SPI at the same time!
	#ifdef LED_BUILTIN
	pinMode(LED_BUILTIN, OUTPUT);
	#endif

	// initialize the SPI channel
	// uncomment these if you want to use different pins for SPI
	// this depends on your MCU model
	//SPI.setMISO(SPI_MISO);
	//SPI.setMOSI(SPI_MOSI);
	//SPI.setSCLK(SPI_SCK);
	// some platforms use this
	//SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
	SPI.begin();

	// initialize the chip
	// set 'sizeMBits' to the size of your chip, e.g. 64 for 25Q64
	if (!flash.begin(&SPI, FLASH_CS, 64)) {
		Serial.println("flash.begin() failed");
		Serial.flush();
		while (1) yield();
	}

	// each chip should have a unique 64-bit ID
	uint64_t uniqueId;
	if (!flash.readUniqueId(&uniqueId)) {
		Serial.println("flash.readUniqueId() failed");
		Serial.flush();
	}

	// run the lengthy flash test
	// the test takes several minutes depending on the chip size
	if (!flash.test()) {
		Serial.println("flash.test() failed");
		Serial.flush();
		while (1) yield();
	}
	Serial.println("flash.test() successful");

	// read some data from the flash
	// will be all FFs because the test erased it
	if (!flash.readData(0, flashData, sizeof(flashData))) {
		Serial.println("flash.readData() failed");
		Serial.flush();
		while (1) yield();
	}
}

// The loop just flashes the LED
void loop() 
{
	// 200ms scheduler
	ulong t = millis();
	static ulong t1 = 0;
	if (t - t1 >= 200) {
		t1 = t;

		#ifdef LED_BUILTIN
		// flash the onboard LED
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
		#endif
	}
}
