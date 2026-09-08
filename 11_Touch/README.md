# Chapter 11.1 — FT6336U Capacitive Touch (raw)

Ported from Freenove's `Sketch_11.1_Touch.ino`. No display output at all
— just polls the FT6336U touch controller directly over I2C and prints
the raw touch-point count and coordinates over serial, 10 times a second.
