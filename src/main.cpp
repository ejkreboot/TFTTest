#include <Arduino.h>
#include <TFT_eSPI.h> 

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");
  Serial.println(TFT_WIDTH);
  Serial.println(TFT_HEIGHT);
  Serial.println(SPI_FREQUENCY);
  tft.init();
  tft.setRotation(1);
  tft.setCursor(10,10);
  tft.print("Hello");
}

void loop() {}
