# Chapter 13.1 — LVGL (baseline setup)

Ported from Freenove's `Sketch_13.1_LVGL.ino`. The simplest possible LVGL
bring-up on this board: initializes the TFT_eSPI display driver and LVGL
itself, then shows a single centered label reading "Hello ESP32-S3!"
along with the LVGL version in use.

Touch input is registered with LVGL (`my_touchpad_read`) but left as an
empty stub — this chapter doesn't actually read the touch controller, it
just establishes the display pipeline that every later LVGL chapter
(14 through 19) builds on.
