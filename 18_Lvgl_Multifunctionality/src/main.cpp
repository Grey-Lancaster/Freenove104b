// Converted from Sketch_18.1_Lvgl_Multifunctionality.ino (Freenove FNK0104B tutorial, chapter 18.1)
#include <Arduino.h>
#include "public.h"
#include "driver/i2s.h"

Display screen;

void fix_image_color_u8(uint8_t * data, uint32_t len_bytes) {
    for(uint32_t i = 0; i < len_bytes; i += 2) {
        uint8_t temp = data[i];
        data[i] = data[i+1];
        data[i+1] = temp;
    }
}

void driver_es8311_init(void) {
  pinMode(AP_ENABLE, OUTPUT);
  digitalWrite(AP_ENABLE, LOW);

  Wire.begin(I2C_SDA, I2C_SCL, I2C_SPEED);

  // Freenove's original code brought up a temporary I2S peripheral here via
  // the newer ESP_I2S.h/I2SClass API purely to give the ES8311 a live
  // BCLK/WS/MCLK signal while its I2C-based register init (es8311_codec_init,
  // below) runs. ESP_I2S.h isn't present in this project's pinned
  // arduino-esp32 core -- same fix as sketches/07_Music and 17_Lvgl_Music:
  // bring up the raw ESP-IDF i2s driver directly instead. The Audio library
  // (music_ui.cpp's music_iis_init() -> audio.setPinout()) takes over this
  // same I2S0 peripheral once the codec's I2C init has run.
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
  /* prepare for possible serial debug */
  Serial.begin(115200);
  /*** Init drivers ***/
  sdmmc_init(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0, SD_MMC_D1, SD_MMC_D2, SD_MMC_D3);  //Initialize the SD module
  driver_es8311_init();
  if (es8311_codec_init() != ESP_OK) {
    Serial.println("ES8311 init failed!");
    return;
  }
  screen.init();

  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.println(LVGL_Arduino);
  Serial.println("I am LVGL_Arduino");

  setup_scr_main(&guider_main_ui);
  lv_scr_load(guider_main_ui.main);

  Serial.println("Setup done");
}

void loop() {
  screen.routine(); /* let the GUI do its work */
  delay(5);
}
