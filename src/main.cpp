#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include "display/TunerDisplay.h"

#define TFT_DC   21
#define TFT_CS   15
#define TFT_SCLK 16
#define TFT_MOSI 17

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, -1);
TunerDisplay tuner(tft);

void setup() {
  tuner.begin();
  tuner.drawStaticUI();
  tuner.drawNote(GLYPH_G, GLYPH_FLAT);
  tuner.drawPitch(192, -20);
  tuner.drawNeedle(0);
  delay(1000);
  tuner.drawNeedle(-20);
  delay(1000);
  tuner.drawNeedle(20);
}

void loop() {}