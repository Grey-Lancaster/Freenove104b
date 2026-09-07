// Converted from Sketch_07.1_Music.ino (Freenove FNK0104B tutorial, chapter 7.1)
// Plays .mp3 files from the root of an SD card (SD_MMC) if one is present
// with any; otherwise falls back to a single embedded track (Olive.mp3,
// baked into the firmware via olive_mp3.h) written to SPIFFS on first boot.
#include <Arduino.h>
#include "FS.h"
#include "SD_MMC.h"
#include "SPI.h"
#include "es8311.h"
#include "Audio.h"
#include "Wire.h"
#include "demo_music.h"

#define FNK0104AB_2P8_240x320_ILI9341
//#define FNK0104N_3P5_320x480_ST77922
//#define FNK0104S_4P0_320x480_ST7796

#ifdef FNK0104N_3P5_320x480_ST77922
 //ESP32-S3 IO Pin define
 #define SD_SCK 5
 #define SD_CMD 4
 #define SD_D0 6
 #define SD_D1 7
 #define SD_D2 2
 #define SD_D3 3

 //I2S IO Pin define
 #define I2S_MCK 17
 #define I2S_BCK 18
 #define I2S_DINT 16
 #define I2S_DOUT 15
 #define I2S_WS 21
 #define AP_ENABLE 1
 #define I2C_SCL 39        /*!< GPIO number used for I2C master clock */
 #define I2C_SDA 38        /*!< GPIO number used for I2C master data  */
 #define I2C_SPEED 400000  /*!< I2C master clock frequency */
#else
 //ESP32-S3 IO Pin define
 #define SD_SCK 38
 #define SD_CMD 40
 #define SD_D0 39
 #define SD_D1 41
 #define SD_D2 48
 #define SD_D3 47

 //I2S IO Pin define
 #define I2S_MCK 4
 #define I2S_BCK 5
 #define I2S_DINT 6
 #define I2S_DOUT 8
 #define I2S_WS 7
 #define AP_ENABLE 1
 #define I2C_SCL 15        /*!< GPIO number used for I2C master clock */
 #define I2C_SDA 16        /*!< GPIO number used for I2C master data  */
 #define I2C_SPEED 400000  /*!< I2C master clock frequency */
#endif

Audio audio;

void driver_es8311_init(void) {
  pinMode(AP_ENABLE, OUTPUT);
  digitalWrite(AP_ENABLE, LOW);

  Wire.begin(I2C_SDA, I2C_SCL, I2C_SPEED);

  // Freenove's original code brought up a temporary I2S peripheral here via
  // the newer ESP_I2S.h/I2SClass API purely to give the ES8311 a live
  // BCLK/WS/MCLK signal while its I2C-based register init (es8311_codec_init,
  // below) runs. ESP_I2S.h isn't present in this project's pinned
  // arduino-esp32 core (same gap already hit and worked around in the
  // sibling `translate` project on this same codec). Arduino's older I2S.h
  // wrapper can't be used here either -- Audio.h (above) already pulls in
  // the raw ESP-IDF driver/i2s.h at global scope, and I2S.h's own
  // namespace-wrapped re-inclusion trick only works if it's the first to
  // include that header, so the two collide. Using the raw ESP-IDF i2s
  // driver directly instead sidesteps the conflict entirely; the Audio
  // library takes over/reinstalls this same I2S0 peripheral itself in
  // audio.setPinout() below once the codec's I2C init has run.
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false
  };
  if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
    Serial.println("Failed to initialize I2S bus!");
  }
  i2s_pin_config_t pin_config = {
    .mck_io_num = I2S_MCK,
    .bck_io_num = I2S_BCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_DINT
  };
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void setup() {
  Serial.begin(115200);

  //SD card init -- bounded retry, not infinite: if no card is present at
  // all, we still want to fall through to the embedded SPIFFS fallback
  // track in demo_music() rather than hang here forever.
  if (!SD_MMC.setPins(SD_SCK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3)) {
    Serial.println("Pin change failed!");
    return;
  }
  bool sd_ready = false;
  for (int attempt = 0; attempt < 10 && !sd_ready; attempt++) {
    sd_ready = SD_MMC.begin();
    if (!sd_ready) {
      Serial.println("SD card does not exist, please insert SD card.");
      delay(100);
    }
  }
  if (!sd_ready) {
    Serial.println("No SD card found after retrying -- continuing without one.");
  }

  driver_es8311_init();
  if (es8311_codec_init() != ESP_OK) {
    Serial.println("ES8311 init failed!");
    return;
  }
  //audio init
  // setPinout's 4th positional parameter is DIN (mic input), not MCK -- see
  // Audio.h: setPinout(BCLK, LRC, DOUT, DIN = I2S_PIN_NO_CHANGE, MCK =
  // I2S_PIN_NO_CHANGE). Passing I2S_MCK positionally as the 4th arg (as
  // Freenove's original tutorial code does) actually leaves the real MCK at
  // its default of "no change", so the codec never gets a real MCLK
  // signal. The sibling `translate` project independently confirmed this
  // exact codec needs a real MCLK on this pin for the speaker (mic still
  // works without it, which is why codec init reports success even when
  // the speaker is silent). DIN isn't wired up via this library here, so
  // it's left at I2S_PIN_NO_CHANGE and MCK is passed explicitly instead.
  audio.setPinout(I2S_BCK, I2S_WS, I2S_DOUT, I2S_PIN_NO_CHANGE, I2S_MCK);
  audio.setVolume(21); // max (volumetable maxes out at index 21 -> gain 64)
  if (!demo_music()) {
    return;
  }

  //play music
  demo_music_play(0);
}

void loop() {
  audio.loop();
}
