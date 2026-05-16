/////////////////////////////////////////////////////////////////////
// Test sketch for W25QxxxFlash Library
// mumanchu and muman.ch, 2026.05.16
// 
// https://github/mumanchu/W25QxxFlash
// https://muman.ch/muman/index.htm?muman-matts-blog.htm
/*
It doesn't do much. It initializes SPI then calls the comprehensive 
flash test method which takes about 4 minutes 22 seconds.

This code was developed for a nice STM32F407ZGT6 board which I bought 
on Aliexpress for CHF11.50 (ECBuying). This board is recommended.
It has a 16Mbit W25Q16 flash chip, and a full SWD/JTAG debug connector.

Here are some hard-to-find technical details of this board:
https://stm32-base.org/boards/STM32F407ZGT6-STM32F4XX.html
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


// SPI pins
#define SPI_SCK			PB_3
#define SPI_MISO		PB_4
#define SPI_MOSI		PB_5

// Chip select for 16Mbit Flash chip
#define FLASH_CS		PB_14

// Onboard LED
#define LED_BUILTIN		PF_9

// W25Q16 16Mbit flash chip
#include "W25QxxFlash.h"
W25QxxFlash flash;

// Some data from the flash
byte flashData[1204];


// Startup
void setup() 
{
	Serial.begin(115200);
	delay(3000);
	Serial.println("\n\rStarted...\n\r");
	Serial.flush();

	pinMode(LED_BUILTIN, OUTPUT);

	// initialize the SPI channel
	SPI.setMISO(SPI_MISO);
	SPI.setMOSI(SPI_MOSI);
	SPI.setSCLK(SPI_SCK);
	SPI.begin();

	// start the 16Mbit flash
	if (!flash.begin(&SPI, FLASH_CS, 16)) {
		Serial.println("flash.begin() failed");
		Serial.flush();
		while (1) yield();
	}

	// run the lengthy flash test
	// the test takes 4 minutes 22 seconds
	if (!flash.test()) {
		Serial.println("flash.test() failed");
		Serial.flush();
		while (1) yield();
	}
	Serial.println("flash.test() successful");

	// read some data from the flash
	// will be all FFs because the test erased it
	if (!flash.readData(0, flashData, 1024)) {
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

		// flash the onboard LED
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
	}
}
