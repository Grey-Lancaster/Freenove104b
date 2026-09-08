# Chapter 4.1 — Button Interrupt + UART

Ported from Freenove's `Sketch_04.1_Button_Interrupt_UART.ino`. Same
onboard BOOT button as chapter 3, but handled via a falling-edge GPIO
interrupt (with a busy-wait debounce inside the ISR) instead of polling
in `loop()`. Each press increments a counter that gets printed over
serial.
