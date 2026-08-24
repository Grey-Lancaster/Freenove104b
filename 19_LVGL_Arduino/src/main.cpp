/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */
// Converted from Sketch_19.1_LVGL_Arduino.ino (Freenove FNK0104B tutorial, chapter 19.1)
#include <Arduino.h>
#include <lvgl.h>
#include "demos/lv_demos.h"
#include <TFT_eSPI.h>

/* 0/2 = portrait, 1/3 = landscape */
#define TFT_DIRECTION 0

#define TFT_SCREEN_WIDTH 240
#define TFT_SCREEN_HEIGHT 320

#if (TFT_DIRECTION == 1) || (TFT_DIRECTION == 3)
  static const uint16_t screenWidth = TFT_SCREEN_HEIGHT;
  static const uint16_t screenHeight = TFT_SCREEN_WIDTH;
#else
  static const uint16_t screenWidth = TFT_SCREEN_WIDTH;
  static const uint16_t screenHeight = TFT_SCREEN_HEIGHT;
#endif

#include "FT6336U.h"

#define I2C_SCL 15
#define I2C_SDA 16
#define INT_N_PIN 17
#define RST_N_PIN 18

static lv_color_t buf[ screenWidth * 40 ];

TFT_eSPI tft = TFT_eSPI(TFT_SCREEN_WIDTH, TFT_SCREEN_HEIGHT); /* TFT instance */
FT6336U ft6336u(I2C_SDA, I2C_SCL, RST_N_PIN, INT_N_PIN);
FT6336U_TouchPointType tp;

static lv_disp_draw_buf_t draw_buf;

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors((uint16_t*)&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    tp = ft6336u.scan();
    int touched = tp.touch_count;

    if( !touched )
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        int x = tp.tp[0].x;
        int y = tp.tp[0].y;
        #if (TFT_DIRECTION == 1)
          x = tp.tp[0].y;
          y = TFT_SCREEN_WIDTH - tp.tp[0].x;
        #elif (TFT_DIRECTION == 3)
          x = TFT_SCREEN_HEIGHT - tp.tp[0].y;
          y = tp.tp[0].x;
        #elif (TFT_DIRECTION == 2)
          x = TFT_SCREEN_WIDTH - tp.tp[0].x;
          y = TFT_SCREEN_HEIGHT - tp.tp[0].y;
        #endif
        if(x >= 0 && x < (int)screenWidth && y >= 0 && y < (int)screenHeight)
        {
            data->state = LV_INDEV_STATE_PR;
            data->point.x = x;
            data->point.y = y;
        }
    }
}

void setup()
{
    Serial.begin( 115200 ); /* prepare for possible serial debug */

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println( LVGL_Arduino );
    Serial.println( "I am LVGL_Arduino" );

    ft6336u.begin();
    lv_init();
    tft.begin();          /* TFT init */
    tft.setRotation( TFT_DIRECTION );
    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * 40 );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register( &disp_drv );

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register( &indev_drv );

    /*Or try out a demo. Don't forget to enable the demos in lv_conf.h. E.g. LV_USE_DEMOS_WIDGETS*/
    lv_demo_widgets();
    // lv_demo_benchmark();
    // lv_demo_keypad_encoder();
    // lv_demo_music();
    // lv_demo_printer();
    // lv_demo_stress();

    Serial.println( "Setup done" );
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay( 5 );
}
