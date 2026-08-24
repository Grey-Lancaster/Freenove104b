// Freenove_WS2812_Lib_for_ESP32.h
//
// Re-implemented on top of Adafruit_NeoPixel. Freenove's original v2.0.1
// implementation called the Arduino-ESP32 core's legacy RMT API directly
// (rmtInit/rmtWrite with rmt_reserve_memsize_t), which no longer exists in
// the arduino-esp32 core version pulled in by espressif32@6.10.0 (needed for
// this board's PSRAM/TFT_eSPI setup -- see project memory). Adafruit_NeoPixel
// handles the RMT peripheral internally and stays API-compatible across core
// versions, so it's used here as the backend instead.
//
// Public class name, enum, and method signatures are kept identical to
// Freenove's original so every tutorial sketch that does
// `#include "Freenove_WS2812_Lib_for_ESP32.h"` and uses
// `Freenove_ESP32_WS2812` needs no changes.

#ifndef _FREENOVE_WS2812_LIB_FOR_ESP32_h
#define _FREENOVE_WS2812_LIB_FOR_ESP32_h

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include "WProgram.h"
#endif

#include <esp_err.h>
#include <Adafruit_NeoPixel.h>

enum LED_TYPE
{					  //R  G  B
	TYPE_RGB = 0x06,  //00 01 10
	TYPE_RBG = 0x09,  //00 10 01
	TYPE_GRB = 0x12,  //01 00 10
	TYPE_GBR = 0x21,  //10 00 01
	TYPE_BRG = 0x18,  //01 10 00
	TYPE_BGR = 0x24	  //10 01 00
};

class Freenove_ESP32_WS2812
{
protected:
	Adafruit_NeoPixel pixels;
	uint16_t ledCounts;
	uint8_t pin;
	uint8_t br;

public:
	Freenove_ESP32_WS2812(uint16_t n = 8, uint8_t pin_gpio = 2, uint8_t chn = 0, LED_TYPE t = TYPE_GRB);

	bool begin();
	void setLedCount(uint16_t n);
	void setLedType(LED_TYPE t);
	void setBrightness(uint8_t brightness);

	esp_err_t set_pixel(int index, uint8_t r, uint8_t g, uint8_t b);

	esp_err_t setLedColorData(int index, uint32_t rgb);
	esp_err_t setLedColorData(int index, uint8_t r, uint8_t g, uint8_t b);

	esp_err_t setLedColorAndBrightnessData(int index, uint32_t rgb, uint8_t brightness);
	esp_err_t setLedColorAndBrightnessData(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);

	esp_err_t setLedColor(int index, uint32_t rgb);
	esp_err_t setLedColor(int index, uint8_t r, uint8_t g, uint8_t b);

	esp_err_t setAllLedsColorData(uint32_t rgb);
	esp_err_t setAllLedsColorData(uint8_t r, uint8_t g, uint8_t b);

	esp_err_t setAllLedsColor(uint32_t rgb);
	esp_err_t setAllLedsColor(uint8_t r, uint8_t g, uint8_t b);

	esp_err_t show();

	uint32_t Wheel(uint8_t pos);
	uint32_t hsv2rgb(uint32_t h, uint32_t s, uint32_t v);
};

#endif
