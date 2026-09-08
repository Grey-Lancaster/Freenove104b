// Converted from Sketch_07.2_Echo.ino (Freenove FNK0104B tutorial, chapter 7.2)
// Records 5 seconds of audio from the ES8311's mic input, then plays it back.
#include <Arduino.h>
#include "FS.h"
#include "SD_MMC.h"
#include "SPI.h"
#include "es8311.h"
#include "Wire.h"
#include <I2S.h>
#include "esp_heap_caps.h"

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

// es8311_codec_init() (es8311.cpp) fixes the codec's own internal sample rate
// at EXAMPLE_SAMPLE_RATE (16000Hz, see es8311.h) regardless of caller -- keep
// the I2S bus at the same rate so they stay in sync.
static const long SAMPLE_RATE = 16000;
static const int BITS_PER_SAMPLE = 16;
static const int RECORD_SECONDS = 5;

size_t audioBufferBytes;
uint8_t *audioBuffer = NULL;

void driver_es8311_init(void) {
  pinMode(AP_ENABLE, OUTPUT);
  digitalWrite(AP_ENABLE, LOW);

  Wire.begin(I2C_SDA, I2C_SCL, I2C_SPEED);

  // Freenove's original code used the newer ESP_I2S.h/I2SClass API
  // (recordWAV()/playWAV() convenience methods), which isn't present in this
  // project's pinned arduino-esp32 core (see project memory). Ported to the
  // older I2S.h global `I2S` object instead -- same pattern already
  // hardware-validated in the sibling `translate` project on this exact
  // codec/I2S pins, including the manual MCLK pin reach-through since
  // Arduino's I2SClass::begin() has no MCLK parameter.
  if (!I2S.setDuplex()) {
    Serial.println("Failed to set I2S duplex mode!");
    return;
  }
  I2S.setAllPins(I2S_BCK, I2S_WS, -1, I2S_DOUT, I2S_DINT); // sck, fs, sd(unused in duplex), outSd, inSd
  // Must be called before begin() -- sizes the DMA/ring buffers used by
  // every I2S.read()/write() call. The library's default (128 samples ->
  // 256 bytes at 16-bit) is tiny: each read/write call has a few ms of
  // fixed overhead (task/ring-buffer bookkeeping) on top of however long
  // the actual audio transfer takes, and at 256 bytes/call that overhead
  // is comparable to the ~4ms of real audio being requested -- consistently
  // falling behind real-time by roughly 2x, losing about every other DMA
  // buffer's worth of audio to ring-buffer overflow between calls. That's
  // what made recordings play back at ~2x speed. 1024 (the library's max)
  // makes each call request ~32ms of audio at once, making that fixed
  // per-call overhead a small fraction of the transfer instead of roughly
  // doubling it.
  I2S.setBufferSize(1024);
  if (!I2S.begin(I2S_PHILIPS_MODE, SAMPLE_RATE, BITS_PER_SAMPLE)) {
    Serial.println("Failed to initialize I2S bus!");
    return;
  }
  esp_i2s::i2s_pin_config_t mclk_pin_config = {
    .mck_io_num = I2S_MCK,
    .bck_io_num = I2S_BCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_DINT
  };
  esp_i2s::i2s_set_pin((esp_i2s::i2s_port_t)0, &mclk_pin_config);
}

void setup()
{
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("Start initializing the audio device...");
  driver_es8311_init();
  if (es8311_codec_init() != ESP_OK) {
    Serial.println("ES8311 init failed!");
    return;
  }
  delay(3000);
  Serial.println("Initialization completed.");

  // Legacy I2S.h drives the bus as stereo frames; there is no MONO/LEFT-slot
  // option like ESP_I2S.h exposed, so the full stereo frame is recorded and
  // played back instead.
  audioBufferBytes = I2S.getBufferSize() * (BITS_PER_SAMPLE / 8);
}

void loop()
{
  size_t wav_size = (size_t)SAMPLE_RATE * 2 /* stereo frame */ * (BITS_PER_SAMPLE / 8) * RECORD_SECONDS;
  uint8_t *wav_buffer = (uint8_t *)heap_caps_malloc(wav_size, MALLOC_CAP_SPIRAM);
  if (wav_buffer == NULL) {
    Serial.println("Failed to allocate recording buffer!");
    delay(1000);
    return;
  }

  // Record 5 seconds of audio data
  Serial.println("Start recording for 5 seconds...");
  // Stops by real wall-clock time, not a fixed nominal byte target. The
  // Arduino-ESP32 I2S library (libraries/I2S/src/I2S.cpp) has a real bug in
  // its receive path: _rx_done_routine()'s per-DMA-event drain size omits
  // the *2-for-stereo factor its _tx_done_routine() counterpart has (found
  // by reading the library source directly), so it only pulls about half
  // of each stereo DMA transfer into the ring buffer -- roughly every
  // other segment of real audio never makes it in. Read throughput ends up
  // at about half the nominal rate regardless of chunk size, so waiting
  // for a fixed byte count made this silently take ~10s for a "5 second"
  // recording, and playing that undersized-for-its-nominal-length buffer
  // back at the (correctly paced) write side made it sound sped up. Can't
  // fix the library from here, so accept whatever real bytes I2S.read()
  // delivers in RECORD_SECONDS of real time and play back that actual
  // count instead.
  size_t recorded = 0;
  uint32_t record_start_ms = millis();
  while (millis() - record_start_ms < (uint32_t)RECORD_SECONDS * 1000 && recorded < wav_size) {
    size_t chunk = min(audioBufferBytes, wav_size - recorded);
    // I2S.read() returns how many bytes it actually delivered -- can be
    // less than requested. Only count what was actually received.
    int got = I2S.read(wav_buffer + recorded, chunk);
    if (got > 0) {
      recorded += (size_t)got;
    }
  }
  Serial.printf("Recording completed: %u bytes in %lums.\n", (unsigned)recorded, (unsigned long)(millis() - record_start_ms));
  delay(1000);

  Serial.println("Start playing the recording...");
  size_t played = 0;
  while (played < recorded) {
    size_t chunk = min(audioBufferBytes, recorded - played);
    // Plain I2S.write() resolves to write_nonblocking(), which SILENTLY
    // DROPS (not queues) anything that doesn't fit in the ~8KB output ring
    // buffer right now, returning immediately either way -- this loop was
    // completing almost instantly and producing static/silence instead of
    // the recording, because most of each chunk was thrown away rather
    // than actually sent to the codec. write_blocking() genuinely blocks
    // until the chunk is queued, which both fixes that and (since it's
    // sized to fit one DMA buffer) paces this loop to real playback speed.
    // The sibling `translate` project independently hit and documented
    // this same bug on this same I2S library.
    I2S.write_blocking(wav_buffer + played, chunk);
    played += chunk;
  }
  Serial.println("Playback has been completed.");
  free(wav_buffer);
  delay(1000);
}
