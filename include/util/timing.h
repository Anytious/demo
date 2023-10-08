#pragma once

#include <stdint.h>
#include <chrono>

namespace fast_com {
namespace util {

class TimerHelper {
 public:
  static void sleep_ms(const double &sleep_time);

  static inline double transform_from_us_to_sec(
      const uint64_t &time_us) {
    return static_cast<double>(time_us) / 1000000;
  }
};

}  // namespace util
}  // namespace fast_com
