#pragma once

#include <array>
#include <cstddef>
#include <limits>

#include "error.hpp"

// frame_id

namespace {
constexpr unsigned long long operator""_KiB(unsigned long long kib) {
  return kib * 1024;
}

constexpr unsigned long long operator""_MiB(unsigned long long mib) {
  return mib * 1024_KiB;
}
constexpr unsigned long long operator""_GiB(unsigned long long gib) {
  return gib * 1024_MiB;
}
} // namespace

/** @brief 物理メモリフレーム1つの大きさ（バイト単位） */
static const auto kBytesPerFrame{4_KiB};

class FrameID {
public:
  explicit FrameID(size_t id) : id_{id} {}
  size_t ID() const { return id_; }
  void *Frame() const { return reinterpret_cast<void *>(id_ * kBytesPerFrame); }

private:
  size_t id_;
};

static const FrameID kNullFrame(std::numeric_limits<size_t>::max());
// frame_id

/** @brief ビットマップ配列を用いてフレーム単位でメモリ管理を行うクラス
 * 1bit を1フレームに対応させて，ビットマップにより秋フレームを管理する
 * 配列alloc_mapの各ビットがフレームに対応し，0なら空き，1なら使用中。
 * alloc_map[n]のmビット目が対応する物理アドレスは次の式で求めることができる
 * kFrameBytes * (n * kBitsPerMapLine + m)
 * */

// bitmap_memory_manager
class BitMapMemoryManager {
public:
  /** @brief このメモリ管理クラスで扱える最大の物理メモリ量(バイト単位) */
  static const auto kMaxPhysicalMemoryBytes{128_GiB};
  /** @brief kMaxPhysicalMemoryBytes
   * までの物理メモリを扱うために必要なフレーム数 */
  static const auto kFrameCount{kMaxPhysicalMemoryBytes / kBytesPerFrame};

  /** @brief ビットマップ配列の要素型 */
  using MapLineType = unsigned long;
  /** @brief ビットマップ配列の1つのようそのビット数==フレーム数 */
  static const size_t kBitsPerMapLine{8 * sizeof(MapLineType)};

  /** @brief インスタンスを初期化する */
  BitMapMemoryManager();

  /** @brief 要求されたフレーム数の領域を確保して先頭のフレームIDを返す */
  WithError<FrameID> Allocate(size_t num_frames);
  Error Free(FrameID start_frame, size_t num_frames);
  void MarkAllocated(FrameID start_frame, size_t num_frames);

  /** @brief このメモリマネージャで扱うメモリ範囲を設定する
   * この呼び出し以降，Allocateによるメモリ割り当ては設定された範囲内でのみ行われるo
   *
   * @param range_begin_ メモリ範囲の始点
   * @param range_end_ メモリ範囲の終点 最終フレームの次のフレーム
   *
   * */
  void SetMemoryRange(FrameID range_begin, FrameID range_end);

private:
  std::array<MapLineType, kFrameCount / kBitsPerMapLine> alloc_map_;
  /** @brief このメモリマネージャで扱うメモリ範囲の始点 */
  FrameID range_begin_;

  /** @brief このメモリマネージャで扱うメモリ範囲の終点 */
  FrameID range_end_;

  bool GetBit(FrameID frame) const;
  void SetBit(FrameID frame, bool allocated);
};
// bitmap_memory_manager

Error InitializeHeap(BitMapMemoryManager &memory_manager);
