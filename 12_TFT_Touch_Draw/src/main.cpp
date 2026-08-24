// Converted from Sketch_12.1_TFT_Touch_Draw.ino (Freenove FNK0104B tutorial, chapter 12.1)
#include <Arduino.h>
#include <TFT_eSPI.h>           // TFT display library
#include "FT6336U.h"

// Invoke custom TFT driver library
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define I2C_SDA 16
#define I2C_SCL 15
#define RST_N_PIN 18
#define INT_N_PIN 17

/* Create an instance of the touch screen library */
FT6336U ft6336u(I2C_SDA, I2C_SCL, RST_N_PIN, INT_N_PIN);
FT6336U_TouchPointType tp;

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

const uint8_t ColorPalette = 21;      // Height of the color palette area
const uint8_t BrushSize = 2;          // Brush thickness (radius)
const uint8_t LineSize = 5;           // Line thickness (radius)

const uint8_t color_num = 12;
int current_color = TFT_WHITE;

unsigned int colors[color_num] = {
  TFT_RED, TFT_PINK, TFT_GREEN, TFT_BLUE, TFT_OLIVE,
  TFT_CYAN, TFT_YELLOW, TFT_WHITE, TFT_MAGENTA,
  TFT_ORANGE, TFT_BLACK, TFT_SILVER
};

// Define coordinate variables
uint16_t x, y;
uint16_t last_x = 0, last_y = 0;

void drawUI() {
    for (int i = 0; i < color_num; i++) {
        tft.fillRect(i * ColorPalette, 0, ColorPalette, ColorPalette, colors[i]);
    }
    // Draw clear screen button
    tft.setCursor(260 , 3, 2);
    tft.setTextColor(TFT_WHITE);
    tft.print("Clear");

    // Display current color
    tft.fillRect(300, 5, 12, 12, current_color);

    // Add palette border/outline
    tft.drawRect(0, 0, SCREEN_HEIGHT, ColorPalette, TFT_WHITE);
}

void setup() {
    Serial.begin(115200);
    ft6336u.begin();
    Serial.print("FT6336U Firmware Version: ");
    Serial.println(ft6336u.read_firmware_id());
    Serial.print("FT6336U Device Mode: ");
    Serial.println(ft6336u.read_device_mode());
    tft.init();

    // Set the TFT and touch screen to landscape orientation
    tft.setRotation(1);

    tft.fillScreen(TFT_BLACK);
    drawUI();
}

void loop() {
    // Check if the touch screen is currently pressed
    // Raw and coordinate values are stored within library at this instant
    // for later retrieval by GetRaw and GetCoord functions.
    tp = ft6336u.scan();
    if (tp.touch_count!=0) // Note this function updates coordinates stored within library variables
    {
      // Read the current X and Y axis as co-ordinates at the last touch time
      // The values were captured when Pressed() was called!
      x = tp.tp[0].y;
      y = 240-tp.tp[0].x;
      Serial.print(x); Serial.print(","); Serial.println(y);
        /* Color palette area logic */
        if (y < ColorPalette + 3) {
            // Enter color selection area, stroke interrupted
            last_x = 0;
            last_y = 0;
            /* Determine stylus click area */
            // Clicked color palette
            if (x >= color_num * ColorPalette) {

              tft.fillRect(0, ColorPalette + 1, tft.width(), tft.height() - ColorPalette - 1, current_color);
            }
            // Clicked Clear button
            else {
              current_color = colors[x / ColorPalette];

              tft.fillRect(tft.width() - 39, 6, 18, 18, current_color);
            }
            return;
        }

        if (last_x == 0 && last_y == 0) {
            // Record current position
            last_x = x;
            last_y = y;
        }

        tft.drawWideLine(last_x, last_y, x, y, LineSize, current_color, current_color);

        // Update coordinates
        last_x = x;
        last_y = y;
    }
    else {
        // Reset coordinates when stylus leaves the screen
        last_x = 0;
        last_y = 0;
    }
}
