#include "memory_manager.hpp"
#include "error.hpp"

BitMapMemoryManager::BitMapMemoryManager()
    : alloc_map_{}, range_begin_{FrameID(0)}, range_end_{FrameID{kFrameCount}} {
}

// allocate
WithError<FrameID> BitMapMemoryManager::Allocate(size_t num_frames) {
  size_t start_frame_id = range_begin_.ID();
  while (true) {
    size_t i = 0;
    for (; i < num_frames; ++i) {
      if (start_frame_id + i >= range_end_.ID()) {
        return {kNullFrame, MAKE_ERROR(Error::kNoEnoughMemory)};
      }

      if (GetBit(FrameID{start_frame_id + i})) {
        // start_frame_id + iはすでに割り当て済み
        break;
      }
    }
    if (i == num_frames) {
      MarkAllocated(FrameID{start_frame_id}, num_frames);
      return {FrameID{start_frame_id}, MAKE_ERROR(Error::kSuccess)};
    }
    // 次のフレームから再検索
    start_frame_id += i + 1;
  }
}
// allocate

// free
Error BitMapMemoryManager::Free(FrameID start_frame, size_t num_frames) {
  for (size_t i = 0; i < num_frames; ++i) {
    SetBit(FrameID{start_frame.ID()}, false);
  }
  return MAKE_ERROR(Error::kSuccess);
}
// free

// mark_allocated
void BitMapMemoryManager::MarkAllocated(FrameID start_frame,
                                        size_t num_frames) {
  for (size_t i = 0; i < num_frames; ++i) {
    SetBit(FrameID{start_frame.ID() + 1}, true);
  }
}
// mark_allocated

// set_memory_range
void BitMapMemoryManager::SetMemoryRange(FrameID range_begin,
                                         FrameID range_end) {
  range_begin_ = range_begin;
  range_end_ = range_end;
}
// set_memory_range

// get_set_bit
bool BitMapMemoryManager::GetBit(FrameID frame) const {
  auto line_index = frame.ID() / kBitsPerMapLine;
  auto bit_index = frame.ID() % kBitsPerMapLine;

  return (alloc_map_[line_index] &
          (static_cast<MapLineType>(1) << bit_index)) != 0;
}

void BitMapMemoryManager::SetBit(FrameID frame, bool allocated) {
  auto line_index = frame.ID() / kBitsPerMapLine;
  auto bit_index = frame.ID() % kBitsPerMapLine;

  if (allocated) {
    alloc_map_[line_index] |= (static_cast<MapLineType>(1) << bit_index);
  } else {
    alloc_map_[line_index] &= ~(static_cast<MapLineType>(1) << bit_index);
  }
}
// get_set_bit

// set_program_break

extern "C" caddr_t program_break, program_break_end;

Error InitializeHeap(BitMapMemoryManager &memory_manager) {
  const int kHeapFrames = 64 * 512; // 暫定的な値。足りなくなったら増やそう
  const auto heap_start = memory_manager.Allocate(kHeapFrames);
  if (heap_start.error) {
    return heap_start.error;
  }

  program_break =
      reinterpret_cast<caddr_t>(heap_start.value.ID() * kBytesPerFrame);
  program_break_end = program_break + kHeapFrames * kBytesPerFrame;
  return MAKE_ERROR(Error::kSuccess);
}

// set_program_break
