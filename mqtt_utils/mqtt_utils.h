#include <chrono>

namespace mqtt {
  inline int64_t milliseconds_since_epoch() {
    return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
  }
}