# Chapter 12.1 — Touch-Driven Drawing

Ported from Freenove's `Sketch_12.1_TFT_Touch_Draw.ino`. A simple
finger-paint app built on raw `TFT_eSPI` + FT6336U touch (no LVGL): a
color-swatch strip along the top of the screen picks the current color,
and dragging a finger anywhere else draws a line that follows it. Tapping
the palette area again clears the canvas.
