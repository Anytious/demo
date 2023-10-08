
#include <chrono>
#include <thread>

#include "util/timing.h"

namespace fast_com {
namespace util {

void TimerHelper::sleep_ms(const double &sleep_time) {
  std::this_thread::sleep_for(
      std::chrono::duration<double, std::milli>(sleep_time));
}

}  // namespace util
}  // namespace fast_com
