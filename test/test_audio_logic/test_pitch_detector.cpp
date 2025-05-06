#include <unity.h>
#include <PitchDetector.h>
#include "testSignal.h"

void test_streaming_pitch_tracking() {
  constexpr float sampleRate = 16000.0f;
  constexpr std::size_t bufferSize = 1024;
  constexpr std::size_t hopSize = bufferSize / 4;
  constexpr float referenceFreq = 110.0f;  // Open A

  PitchTracker tracker(sampleRate, bufferSize, 10.0f);
  tracker.setHysteresisMargin(2.0f);

  int stableCount = 0;
  int inTuneCount = 0;

  printf("Streaming test signal in blocks...\n");

  for (std::size_t offset = 0; offset + bufferSize <= testSignalLength; offset += hopSize) {
    const int16_t* chunk = testSignal + offset;

    float freq = tracker.update<bufferSize>(chunk);
    float cents = tracker.getCentsFrom(referenceFreq);

    printf("Block @ sample %5zu: Pitch = %7.4f Hz", offset, freq);
    if (tracker.isStable()) {
      printf(" ✓ stable");
      stableCount++;
    }
    printf(" (%+.2f cents)", cents);

    if (tracker.isInTune(referenceFreq)) {
      printf(" 🎯 in tune");
      inTuneCount++;
    }

    printf("\n");
  }

  printf("Stable frames: %d\n", stableCount);
  printf("In tune frames: %d\n", inTuneCount);

  TEST_ASSERT_GREATER_THAN(80, stableCount);       // Was the pitch stable at all?
  TEST_ASSERT_LESS_THAN(100, stableCount);       // Was the pitch ever not stable?
  TEST_ASSERT_GREATER_THAN(100, inTuneCount);       // Was it ever "in tune"?
  TEST_ASSERT_LESS_THAN(150, inTuneCount);       // Was the pitch ever not "in tune"?
}


void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_streaming_pitch_tracking);
  UNITY_END();
}

#ifndef ARDUINO
int main(int argc, char** argv) {

  setup();
  return 0;
}
#endif

void loop() {
  // not used
}
