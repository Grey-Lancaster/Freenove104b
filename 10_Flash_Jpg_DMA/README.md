# Chapter 10.2 — JPEG Decode From Flash

Ported from Freenove's `Sketch_10.2_Flash_Jpg_DMA.ino`. Decodes a JPEG
baked directly into flash (`src/panda.h`, via the `TJpg_Decoder` library)
and redraws it to the TFT every 2 seconds, printing the image dimensions
and decode time over serial each time.
