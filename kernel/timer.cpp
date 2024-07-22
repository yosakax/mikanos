#include "timer.hpp"

namespace {
const uint32_t kCountMax = 0xffffffff;
volatile uint32_t &lvt_timer = *reinterpret_cast<uint32_t *>(0xfee00320);
volatile uint32_t &initial_count = *reinterpret_cast<uint32_t *>(0xfee00380);
volatile uint32_t &current_count = *reinterpret_cast<uint32_t *>(0xfee00390);
volatile uint32_t &devide_config = *reinterpret_cast<uint32_t *>(0xfee003e0);
} // namespace

void InitializeLAPICTimer() {
  devide_config = 0b1011;         // 分周比１
  lvt_timer = (0b001 << 16) | 32; // masked,. one-shot
}

void StartLAPICTimer() { initial_count = kCountMax; }

uint32_t LAPICTimerElapsed() { return kCountMax - current_count; }

void StopLAPICTimer() { initial_count = 0; }
