// LVGL UI for the chapter 7.2 (07_Echo) mic record + playback demo -- not
// one of Freenove's own 19 tutorial chapters, a new addition pairing
// 07_Echo's I2S record/playback with an LVGL screen (Record/Play buttons,
// status label) the way 17_Lvgl_Music pairs with 07_Music.
#include <Arduino.h>
#include "display.h"
#include "echo_ui.h"

Display screen;

void setup() {
  Serial.begin(115200);

  screen.init();

  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.println(LVGL_Arduino);
  Serial.println("I am LVGL_Arduino");

  setup_scr_echo(&guider_echo_ui);
  lv_scr_load(guider_echo_ui.echo);

  if (echo_iis_init() != 0) {
    lv_label_set_text(guider_echo_ui.status_label, "Audio init failed -- check serial log.");
    lv_obj_add_state(guider_echo_ui.record_btn, LV_STATE_DISABLED);
  }

  Serial.println("Setup done");
}

void loop() {
  screen.routine(); /* let the GUI do its work */
  delay(5);
}
