
/*====================================================================================================
   Adapted from proof of concept code published by Joel de Guzman: https://www.cycfi.com/2018/03/
   
   Copyright (c) 2018 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
====================================================================================================*/
#ifndef PITCH_DETECTOR_H
#define PITCH_DETECTOR_H


#include <array>
#include <cstdint>
#include <cmath>
#include <limits>

#define MIN_FREQUENCY 80
#define MAX_FREQUENCY 1100

template <typename T>
constexpr bool is_pow2(T n) {
    return (n & (n - 1)) == 0;
}

template <std::uint32_t N, typename T = std::uint32_t>
struct bitstream {
    static_assert(is_pow2(N), "N must be a power of 2");
    static_assert(std::is_unsigned<T>::value, "T must be unsigned");

    static constexpr auto nbits = 8 * sizeof(T);
    static constexpr auto array_size = N / nbits;

    std::array<T, array_size> bits;

    void clear() {
        bits.fill(0);
    }

    void set(std::uint32_t i, bool val) {
        auto mask = 1 << (i % nbits);
        auto& ref = bits[i / nbits];
        ref ^= (-T(val) ^ ref) & mask;
    }

    bool get(std::uint32_t i) const {
        auto mask = 1 << (i % nbits);
        return (bits[i / nbits] & mask) != 0;
    }

    static std::uint32_t count_bits(std::uint32_t i) {
        return __builtin_popcount(i);
    }

    static std::uint64_t count_bits(std::uint64_t i) {
        return __builtin_popcountll(i);
    }

    template <typename F>
    void auto_correlate(F f) {
        constexpr auto mid_array = (array_size / 2) - 1;
        constexpr auto mid_pos = N / 2;

        auto index = 0;
        auto shift = 1;
        for (auto pos = 1; pos != mid_pos; ++pos) {
            auto* p1 = bits.data();
            auto* p2 = bits.data() + index;
            auto count = 0;

            if (shift == 0) {
                for (auto i = 0; i != mid_array; ++i)
                    count += count_bits(*p1++ ^ *p2++);
            } else {
                auto shift2 = nbits - shift;
                for (auto i = 0; i != mid_array; ++i) {
                    auto v = *p2++ >> shift;
                    v |= *p2 << shift2;
                    count += count_bits(*p1++ ^ v);
                }
            }

            f(pos, count);

            ++shift;
            if (shift == nbits) {
                shift = 0;
                ++index;
            }
        }
    }
};

struct zero_cross {
    bool operator()(int16_t s) {
        if (s < -100) y = 0;
        else if (s > 0) y = 1;
        return y;
    }
    bool y = 0;
};

template <std::size_t N>
float detectFrequencyBitstream(const int16_t* samples, std::size_t length, float sampleRate) {
    if (length < N) return 0;

    bitstream<N> bin;
    bin.clear();

    zero_cross zc;
    for (std::size_t i = 0; i < N; ++i) {
        bin.set(i, zc(samples[i]));
    }

    std::size_t bestLag = 0;
    std::uint32_t minDiff = std::numeric_limits<std::uint32_t>::max();

    std::size_t minLag = static_cast<std::size_t>(sampleRate / MAX_FREQUENCY);
    std::size_t maxLag = static_cast<std::size_t>(sampleRate / MIN_FREQUENCY);

    bin.auto_correlate([&](std::size_t lag, std::uint32_t diff) {
        if (lag >= minLag && lag <= maxLag) {
            if (diff < minDiff) {
                minDiff = diff;
                bestLag = lag;
            }
        }
    });

    if (bestLag == 0) return 0;
    return sampleRate / bestLag;
}

class PitchTracker {
  public:
      PitchTracker(float sampleRate, std::size_t bufferSize, float toleranceCents = 5.0f)
          : _sampleRate(sampleRate),
            _bufferSize(bufferSize),
            _toleranceCents(toleranceCents),
            _lastPitch(0.0f),
            _smoothedPitch(0.0f),
            _stableCount(0),
            _minStableFrames(3),
            _hysteresisMargin(2.0f),
            _wasInTune(false) {}
  
      template<std::size_t N>
      float update(const int16_t* samples) {
          float newPitch = detectFrequencyBitstream<N>(samples, _bufferSize, _sampleRate);
          float centsJump = (_lastPitch > 0) ? centsDifference(newPitch, _lastPitch) : 0.0f;

          if (centsJump > 150.0f) {
              _cooldownFrames = _cooldownLength;
              _stableCount = 0;
              // Optional: skip updating smoothedPitch this frame
              return _smoothedPitch;
          }

          if (_cooldownFrames > 0) {
              _cooldownFrames--;
              return _smoothedPitch;  // Freeze output during cooldown
          }
          
          if (newPitch == 0.0f) {
              _stableCount = 0;
              return _smoothedPitch;
          }
  
          if (_lastPitch > 0 && centsDifference(newPitch, _lastPitch) <= _toleranceCents) {
              _smoothedPitch = 0.9f * _smoothedPitch + 0.1f * newPitch;
              _stableCount++;
          } else {
              _stableCount = 0;
              _smoothedPitch = newPitch;
          }
  
          _lastPitch = newPitch;
          return _smoothedPitch;
      }
  
      bool isStable() const {
          return _stableCount >= _minStableFrames;
      }
  
      float getSmoothedPitch() const {
          return _smoothedPitch;
      }
  
      float getCentsFrom(float referenceFreq) const {
          if (_smoothedPitch <= 0 || referenceFreq <= 0) return 0.0f;
          return 1200.0f * std::log2(_smoothedPitch / referenceFreq);
      }
  
      void setHysteresisMargin(float marginCents) {
          _hysteresisMargin = marginCents;
      }
  
      bool isInTune(float referenceFreq) {
          float cents = getCentsFrom(referenceFreq);
          if (_wasInTune) {
              _wasInTune = std::fabs(cents) < (_toleranceCents + _hysteresisMargin);
          } else {
              _wasInTune = std::fabs(cents) < _toleranceCents;
          }
          return _wasInTune;
      }
  
  private:
      float centsDifference(float f1, float f2) const {
          return std::fabs(1200.0f * std::log2(f1 / f2));
      }
  
      float _sampleRate;
      std::size_t _bufferSize;
      float _toleranceCents;
      float _lastPitch;
      float _smoothedPitch;
      int _stableCount;
      int _minStableFrames;
      float _hysteresisMargin;
      bool _wasInTune;
      int _cooldownFrames = 0;
      const int _cooldownLength = 3; // Number of frames to suppress after a jump

  };
  
#endif // PITCH_DETECTOR_H
