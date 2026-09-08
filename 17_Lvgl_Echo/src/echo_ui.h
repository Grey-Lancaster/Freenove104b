#ifndef __ECHO_UI_H
#define __ECHO_UI_H

#include "lvgl.h"

typedef struct lvgl_echo
{
  lv_obj_t *echo;
  lv_obj_t *record_btn;
  lv_obj_t *record_label;
  lv_obj_t *play_btn;
  lv_obj_t *play_label;
  lv_obj_t *status_label;
} lvgl_echo_ui;

extern lvgl_echo_ui guider_echo_ui;

void setup_scr_echo(lvgl_echo_ui *ui);  //Parameter configuration function on the echo screen
int echo_iis_init(void);                //Bring up the codec/I2S bus and allocate the recording buffer; 0 on success

#endif
