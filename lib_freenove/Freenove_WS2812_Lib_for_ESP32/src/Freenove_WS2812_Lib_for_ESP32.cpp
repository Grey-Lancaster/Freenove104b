// Freenove_WS2812_Lib_for_ESP32.cpp
// See header for why this wraps Adafruit_NeoPixel instead of Freenove's
// original direct-RMT implementation.

#include "Freenove_WS2812_Lib_for_ESP32.h"

static neoPixelType mapType(LED_TYPE t)
{
	switch (t) {
		case TYPE_RGB: return NEO_RGB + NEO_KHZ800;
		case TYPE_RBG: return NEO_RBG + NEO_KHZ800;
		case TYPE_GRB: return NEO_GRB + NEO_KHZ800;
		case TYPE_GBR: return NEO_GBR + NEO_KHZ800;
		case TYPE_BRG: return NEO_BRG + NEO_KHZ800;
		case TYPE_BGR: return NEO_BGR + NEO_KHZ800;
		default:       return NEO_GRB + NEO_KHZ800;
	}
}

Freenove_ESP32_WS2812::Freenove_ESP32_WS2812(uint16_t n /*= 8*/, uint8_t pin_gpio /*= 2*/, uint8_t chn /*= 0*/, LED_TYPE t /*= TYPE_GRB*/)
	: pixels(n, pin_gpio, mapType(t)), ledCounts(n), pin(pin_gpio), br(255)
{
	(void)chn; // RMT channel is chosen internally by Adafruit_NeoPixel
}

bool Freenove_ESP32_WS2812::begin()
{
	pixels.begin();
	pixels.clear();
	return true;
}

void Freenove_ESP32_WS2812::setLedCount(uint16_t n)
{
	ledCounts = n;
	pixels.updateLength(n);
	begin();
}

void Freenove_ESP32_WS2812::setLedType(LED_TYPE t)
{
	pixels.updateType(mapType(t));
}

void Freenove_ESP32_WS2812::setBrightness(uint8_t brightness)
{
	br = constrain(brightness, 0, 255);
}

esp_err_t Freenove_ESP32_WS2812::set_pixel(int index, uint8_t r, uint8_t g, uint8_t b)
{
	pixels.setPixelColor(index, pixels.Color(r, g, b));
	return ESP_OK;
}

esp_err_t Freenove_ESP32_WS2812::setLedColorData(int index, uint32_t rgb)
{
	return setLedColorData(index, (uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb);
}

esp_err_t Freenove_ESP32_WS2812::setLedColorData(int index, uint8_t r, uint8_t g, uint8_t b)
{
	return set_pixel(index, r * br / 255, g * br / 255, b * br / 255);
}

esp_err_t Freenove_ESP32_WS2812::setLedColorAndBrightnessData(int index, uint32_t rgb, uint8_t brightness)
{
	return setLedColorAndBrightnessData(index, (uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb, brightness);
}

esp_err_t Freenove_ESP32_WS2812::setLedColorAndBrightnessData(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
	brightness = constrain(brightness, 0, 255);
	return set_pixel(index, r * brightness / 255, g * brightness / 255, b * brightness / 255);
}

esp_err_t Freenove_ESP32_WS2812::setLedColor(int index, uint32_t rgb)
{
	return setLedColor(index, (uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb);
}

esp_err_t Freenove_ESP32_WS2812::setLedColor(int index, uint8_t r, uint8_t g, uint8_t b)
{
	setLedColorData(index, r, g, b);
	return show();
}

esp_err_t Freenove_ESP32_WS2812::setAllLedsColorData(uint32_t rgb)
{
	for (int i = 0; i < ledCounts; i++) {
		setLedColorData(i, rgb);
	}
	return ESP_OK;
}

esp_err_t Freenove_ESP32_WS2812::setAllLedsColorData(uint8_t r, uint8_t g, uint8_t b)
{
	for (int i = 0; i < ledCounts; i++) {
		setLedColorData(i, r, g, b);
	}
	return ESP_OK;
}

esp_err_t Freenove_ESP32_WS2812::setAllLedsColor(uint32_t rgb)
{
	setAllLedsColorData(rgb);
	return show();
}

esp_err_t Freenove_ESP32_WS2812::setAllLedsColor(uint8_t r, uint8_t g, uint8_t b)
{
	setAllLedsColorData(r, g, b);
	return show();
}

esp_err_t Freenove_ESP32_WS2812::show()
{
	pixels.show();
	return ESP_OK;
}

uint32_t Freenove_ESP32_WS2812::Wheel(uint8_t pos)
{
	uint32_t WheelPos = pos % 0xff;
	if (WheelPos < 85) {
		return ((255 - WheelPos * 3) << 16) | ((WheelPos * 3) << 8);
	}
	if (WheelPos < 170) {
		WheelPos -= 85;
		return (((255 - WheelPos * 3) << 8) | (WheelPos * 3));
	}
	WheelPos -= 170;
	return ((WheelPos * 3) << 16 | (255 - WheelPos * 3));
}

uint32_t Freenove_ESP32_WS2812::hsv2rgb(uint32_t h, uint32_t s, uint32_t v)
{
	uint8_t r, g, b;
	h %= 360; // h -> [0,360]
	uint32_t rgb_max = v * 2.55f;
	uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

	uint32_t i = h / 60;
	uint32_t diff = h % 60;

	// RGB adjustment amount by hue
	uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

	switch (i) {
	case 0:
		r = rgb_max;
		g = rgb_min + rgb_adj;
		b = rgb_min;
		break;
	case 1:
		r = rgb_max - rgb_adj;
		g = rgb_max;
		b = rgb_min;
		break;
	case 2:
		r = rgb_min;
		g = rgb_max;
		b = rgb_min + rgb_adj;
		break;
	case 3:
		r = rgb_min;
		g = rgb_max - rgb_adj;
		b = rgb_max;
		break;
	case 4:
		r = rgb_min + rgb_adj;
		g = rgb_min;
		b = rgb_max;
		break;
	default:
		r = rgb_max;
		g = rgb_min;
		b = rgb_max - rgb_adj;
		break;
	}
	return (uint32_t)(r << 16 | g << 8 | b);
}
