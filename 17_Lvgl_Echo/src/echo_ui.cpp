// LVGL screen for the chapter 7.2 (07_Echo) mic record + playback demo --
// not one of Freenove's own 19 tutorial chapters, a new addition pairing
// 07_Echo's I2S record/playback with an LVGL UI the way 17_Lvgl_Music
// pairs with 07_Music.
#include "echo_ui.h"
#include "es8311.h"
#include "Wire.h"
#include <I2S.h>
#include "esp_heap_caps.h"

#if defined FNK0104N_3P5_320x480_ST77922
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

// es8311_codec_init() (es8311.cpp) fixes the codec's own internal sample
// rate at EXAMPLE_SAMPLE_RATE (16000Hz, see es8311.h) regardless of caller
// -- keep the I2S bus at the same rate so they stay in sync.
static const long SAMPLE_RATE = 16000;
static const int BITS_PER_SAMPLE = 16;
static const int RECORD_SECONDS = 5;

lvgl_echo_ui guider_echo_ui;

static size_t audio_buffer_bytes = 0;
static uint8_t *wav_buffer = NULL;
static size_t wav_size = 0;
static bool has_recording = false;
static TaskHandle_t echoTaskHandle = NULL;
// 0 = idle, 1 = recording, 2 = playing -- guards against re-entering while
// a background task already owns the I2S bus / recording buffer.
static volatile int echo_task_flag = 0;

static void set_status(const char *text)
{
  lv_label_set_text(guider_echo_ui.status_label, text);
}

static void set_buttons_enabled(bool enabled)
{
  if (enabled)
  {
    lv_obj_clear_state(guider_echo_ui.record_btn, LV_STATE_DISABLED);
    if (has_recording)
    {
      lv_obj_clear_state(guider_echo_ui.play_btn, LV_STATE_DISABLED);
    }
  }
  else
  {
    lv_obj_add_state(guider_echo_ui.record_btn, LV_STATE_DISABLED);
    lv_obj_add_state(guider_echo_ui.play_btn, LV_STATE_DISABLED);
  }
}

// Runs on its own pinned task (same convention as music_ui.cpp's
// loopTask_music) so the ~5-second blocking I2S read doesn't freeze LVGL's
// own screen.routine()/lv_task_handler() loop.
static void record_task(void *pvParameters)
{
  char buf[32];
  size_t recorded = 0;
  uint32_t start_ms = millis();
  int last_shown = -1;

  while (recorded < wav_size)
  {
    size_t chunk = min(audio_buffer_bytes, wav_size - recorded);
    I2S.read(wav_buffer + recorded, chunk);
    recorded += chunk;

    int elapsed = (millis() - start_ms) / 1000;
    if (elapsed != last_shown)
    {
      last_shown = elapsed;
      snprintf(buf, sizeof(buf), "Recording... %ds", elapsed);
      set_status(buf);
    }
  }

  has_recording = true;
  echo_task_flag = 0;
  set_status("Recording complete. Ready to play.");
  set_buttons_enabled(true);
  Serial.println("Recording complete.");
  vTaskDelete(NULL);
}

static void play_task(void *pvParameters)
{
  char buf[32];
  size_t played = 0;
  uint32_t start_ms = millis();
  int last_shown = -1;

  while (played < wav_size)
  {
    size_t chunk = min(audio_buffer_bytes, wav_size - played);
    I2S.write(wav_buffer + played, chunk);
    played += chunk;

    int elapsed = (millis() - start_ms) / 1000;
    if (elapsed != last_shown)
    {
      last_shown = elapsed;
      snprintf(buf, sizeof(buf), "Playing... %ds", elapsed);
      set_status(buf);
    }
  }

  echo_task_flag = 0;
  set_status("Playback complete. Ready to play again.");
  set_buttons_enabled(true);
  Serial.println("Playback complete.");
  vTaskDelete(NULL);
}

static void record_btn_event_handler(lv_event_t *e)
{
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  if (echo_task_flag != 0)
    return;

  Serial.println("Clicked the record button.");
  echo_task_flag = 1;
  set_buttons_enabled(false);
  set_status("Recording... 0s");
  xTaskCreatePinnedToCore(record_task, "echo_record", 8192, NULL, 1, &echoTaskHandle, 1);
}

static void play_btn_event_handler(lv_event_t *e)
{
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  if (echo_task_flag != 0 || !has_recording)
    return;

  Serial.println("Clicked the play button.");
  echo_task_flag = 2;
  set_buttons_enabled(false);
  set_status("Playing... 0s");
  xTaskCreatePinnedToCore(play_task, "echo_play", 8192, NULL, 1, &echoTaskHandle, 1);
}

//Parameter configuration function on the echo screen
void setup_scr_echo(lvgl_echo_ui *ui)
{
  ui->echo = lv_obj_create(NULL);
  lv_obj_clear_flag(ui->echo, LV_OBJ_FLAG_SCROLLABLE);

  static lv_style_t bg_style;
  lv_style_init(&bg_style);
  lv_style_set_bg_color(&bg_style, lv_color_hex(0xffffff));
  lv_obj_add_style(ui->echo, &bg_style, LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(ui->echo);
  lv_label_set_text(title, "Mic Record & Playback");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

  ui->record_btn = lv_btn_create(ui->echo);
  lv_obj_set_size(ui->record_btn, 160, 50);
  lv_obj_align(ui->record_btn, LV_ALIGN_CENTER, 0, -30);
  ui->record_label = lv_label_create(ui->record_btn);
  lv_label_set_text(ui->record_label, "Record (5s)");
  lv_obj_center(ui->record_label);

  ui->play_btn = lv_btn_create(ui->echo);
  lv_obj_set_size(ui->play_btn, 160, 50);
  lv_obj_align(ui->play_btn, LV_ALIGN_CENTER, 0, 40);
  lv_obj_add_state(ui->play_btn, LV_STATE_DISABLED);  // nothing recorded yet
  ui->play_label = lv_label_create(ui->play_btn);
  lv_label_set_text(ui->play_label, "Play");
  lv_obj_center(ui->play_label);

  ui->status_label = lv_label_create(ui->echo);
  lv_label_set_text(ui->status_label, "Ready to record.");
  lv_obj_set_width(ui->status_label, 220);
  lv_obj_set_style_text_align(ui->status_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(ui->status_label, LV_ALIGN_CENTER, 0, 100);

  lv_obj_add_event_cb(ui->record_btn, record_btn_event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui->play_btn, play_btn_event_handler, LV_EVENT_ALL, NULL);
}

//Bring up the codec/I2S bus and allocate the recording buffer once up front
//(reused for every take, rather than 07_Echo's malloc/free every loop pass)
int echo_iis_init(void)
{
  pinMode(AP_ENABLE, OUTPUT);
  digitalWrite(AP_ENABLE, LOW);

  Wire.begin(I2C_SDA, I2C_SCL, I2C_SPEED);

  // Freenove's original code used the newer ESP_I2S.h/I2SClass API
  // (recordWAV()/playWAV() convenience methods), which isn't present in
  // this project's pinned arduino-esp32 core (same gap 07_Echo already
  // worked around). Uses the older I2S.h global `I2S` object instead,
  // including a manual MCLK pin reach-through since I2SClass::begin() has
  // no MCLK parameter of its own.
  if (!I2S.setDuplex())
  {
    Serial.println("Failed to set I2S duplex mode!");
    return -1;
  }
  I2S.setAllPins(I2S_BCK, I2S_WS, -1, I2S_DOUT, I2S_DINT);  // sck, fs, sd(unused in duplex), outSd, inSd
  if (!I2S.begin(I2S_PHILIPS_MODE, SAMPLE_RATE, BITS_PER_SAMPLE))
  {
    Serial.println("Failed to initialize I2S bus!");
    return -1;
  }
  esp_i2s::i2s_pin_config_t mclk_pin_config = {
    .mck_io_num = I2S_MCK,
    .bck_io_num = I2S_BCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_DINT
  };
  esp_i2s::i2s_set_pin((esp_i2s::i2s_port_t)0, &mclk_pin_config);

  if (es8311_codec_init() != ESP_OK)
  {
    Serial.println("ES8311 init failed!");
    return -1;
  }

  // Legacy I2S.h drives the bus as stereo frames; there is no MONO/LEFT-slot
  // option like ESP_I2S.h exposed, so the full stereo frame is recorded and
  // played back, same as 07_Echo.
  audio_buffer_bytes = I2S.getBufferSize() * (BITS_PER_SAMPLE / 8);
  wav_size = (size_t)SAMPLE_RATE * 2 /* stereo frame */ * (BITS_PER_SAMPLE / 8) * RECORD_SECONDS;
  wav_buffer = (uint8_t *)heap_caps_malloc(wav_size, MALLOC_CAP_SPIRAM);
  if (wav_buffer == NULL)
  {
    Serial.println("Failed to allocate recording buffer!");
    return -1;
  }
  return 0;
}
