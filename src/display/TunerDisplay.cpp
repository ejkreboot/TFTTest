#include "TunerDisplay.h"
#include "Instrument_Sans_10.h"
#include <cmath>
#include <pgmspace.h>

extern const uint16_t sprite_sheet[];
extern const Sprite tunerSprites[];

TunerDisplay::TunerDisplay(Adafruit_ILI9341& d) : tft(d) {}

void TunerDisplay::begin() {
  tft.begin();
  tft.setRotation(0);
  tft.setFont(&InstrumentSans_Regular10pt8b);
  tft.setTextSize(1);
  tft.setTextColor(0xFFFF);
}

void TunerDisplay::drawStaticUI() {
  drawSprite(ARC_X, ARC_Y, TUNER_ARC);
  drawSprite(TARGET_X, TARGET_Y, TUNING_TARGET);
  drawSprite(SIGNAL_CAPTION_X, SIGNAL_CAPTION_Y, SIGNAL_CAPTION);
}

void TunerDisplay::drawNote(GlyphSymbol symbol, GlyphSymbol accidental) {
  drawGlyph(symbol, AMBER, NOTE_GLYPH_X, NOTE_GLYPH_Y);
  drawGlyph(accidental, AMBER, NOTE_GLYPH_X + 42, NOTE_GLYPH_Y);
}

void TunerDisplay::drawPitch(float hz, int cents) {
  tft.setCursor(HZ_LABEL_X, TEXT_Y);
  tft.print(hz, 0);
  tft.print("hz");

  tft.setCursor(CENTS_LABEL_X, TEXT_Y);
  tft.print(cents);
  tft.print((char)0xA2); // ¢
}

void TunerDisplay::drawGlyph(GlyphSymbol symbol, GlyphColor color, int x, int y) {
  const GlyphSheet sheet(Unica_allArray[color], 4, 3, 50, 60);
  int index = static_cast<int>(symbol);
  int col = index % sheet.sheetWidth;
  int row = index / sheet.sheetWidth;
  const int sheetRowWidth = sheet.sheetWidth * sheet.glyphWidth;

  static uint16_t lineBuffer[50];

  for (int i = 0; i < sheet.glyphHeight; ++i) {
    for (int j = 0; j < sheet.glyphWidth; ++j) {
      int offset = ((row * sheet.glyphHeight + i) * sheetRowWidth) + (col * sheet.glyphWidth + j);
      lineBuffer[j] = pgm_read_word(&sheet.bitmap[offset]);
    }
    tft.drawRGBBitmap(x, y + i, lineBuffer, sheet.glyphWidth, 1);
  }
}

void TunerDisplay::drawSprite(int x, int y, TunerSpriteID id) {
  const Sprite& sprite = tunerSprites[id];
  const int sheetRowWidth = 200;
  static uint16_t lineBuffer[200];

  for (int i = 0; i < sprite.height; ++i) {
    for (int j = 0; j < sprite.width; ++j) {
      int offset = ((sprite.y + i) * sheetRowWidth) + (sprite.x + j);
      lineBuffer[j] = pgm_read_word(&sprite_sheet[offset]);
    }
    tft.drawRGBBitmap(x, y + i, lineBuffer, sprite.width, 1);
  }
}

void TunerDisplay::drawNeedle(float cents) {
  constexpr float MAX_DEFLECTION = 50.0;
  constexpr float MIN_ANGLE = -45.0;
  constexpr float MAX_ANGLE = 45.0;

  cents = constrain(cents, -MAX_DEFLECTION, MAX_DEFLECTION);
  float angle = map(cents, -MAX_DEFLECTION, MAX_DEFLECTION, MIN_ANGLE, MAX_ANGLE);

  auto drawRotated = [&](float angleDeg, bool erase) {
    const Sprite& sprite = tunerSprites[TUNER_NEEDLE];
    float angleRad = angleDeg * M_PI / 180.0;
    int px = CENTER_X + NEEDLE_LEN * sin(angleRad);
    int py = CENTER_Y - NEEDLE_LEN * cos(angleRad);
    float cxSprite = sprite.width / 2.0;
    float cySprite = 0;
    const int sheetRowWidth = 200;

    for (int i = 0; i < sprite.height; ++i) {
      for (int j = 0; j < sprite.width; ++j) {
        float dx = j - cxSprite;
        float dy = i - cySprite;
        float rx = dx * cos(angleRad) - dy * sin(angleRad);
        float ry = dx * sin(angleRad) + dy * cos(angleRad);
        int screenX = px + round(rx);
        int screenY = py + round(ry);
        int offset = (sprite.y + i) * sheetRowWidth + (sprite.x + j);
        uint16_t color = pgm_read_word(&sprite_sheet[offset]);
        if (color != 0x0000) {
          tft.drawPixel(screenX, screenY, erase ? ILI9341_BLACK : color);
        }
      }
    }
  };

  if (!isnan(previousCents))
    drawRotated(map(previousCents, -50, 50, -45, 45), true);

  drawRotated(angle, false);
  previousCents = cents;
}
