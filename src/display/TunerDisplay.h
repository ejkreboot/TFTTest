#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <MusicGlyphs.h>
#include <TunerSprites.h>

class TunerDisplay {
public:
  TunerDisplay(Adafruit_ILI9341& display);

  void begin();
  void drawStaticUI();
  void drawNeedle(float cents);
  void drawNote(GlyphSymbol symbol, GlyphSymbol accidental);
  void drawPitch(float hz, int cents);

private:
  void drawGlyph(GlyphSymbol symbol, GlyphColor color, int x, int y);
  void drawSprite(int x, int y, TunerSpriteID id);

  Adafruit_ILI9341& tft;
  float previousCents = NAN;
};

// Display geometry (240x240)
constexpr int CENTER_X = 120;
constexpr int CENTER_Y = 134;
constexpr int NEEDLE_LEN = 90;

// Static sprite positions
constexpr int ARC_X = 20;
constexpr int ARC_Y = 34;
constexpr int TARGET_X = 100;
constexpr int TARGET_Y = 15;
constexpr int SIGNAL_CAPTION_X = 20;
constexpr int SIGNAL_CAPTION_Y = 215;

// Note glyph position
constexpr int NOTE_GLYPH_X = 78;
constexpr int NOTE_GLYPH_Y = 100;

// Text positions
constexpr int HZ_LABEL_X = 65;
constexpr int CENTS_LABEL_X = 130;
constexpr int TEXT_Y = 200;
