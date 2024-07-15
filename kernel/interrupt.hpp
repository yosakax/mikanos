#pragma once

#include <array>
#include <cstdint>

// desc_types
enum class DescriptorType {
  kUpper8Bytes = 0,
  kLDT = 2,
  kTSSAvailable = 9,
  kTssBusy = 11,
  kCallGate = 12,
  kInterruptGate = 14,
  kTrapGate = 15,
};
// desc_types

// descriptor_attr_struct
union InterruptDescriptorAttribute {
  uint16_t data;
  struct {
    uint16_t interrupt_stack_table : 3;
    uint16_t : 5;
    DescriptorType type : 4;
    uint16_t : 1;
    uint16_t descriptor_pribilege_level : 2;
    uint16_t present : 1;
  } __attribute__((packed)) bits;
} __attribute__((packed));
// descriptor_attr_struct

// descriptor_struct
struct InterruptDescriptor {
  uint16_t offset_low;
  uint16_t segment_selector;
  InterruptDescriptorAttribute attr;
  uint16_t offset_middle;
  uint32_t offset_high;
  uint32_t reserved;
} __attribute__((packed));
// descriptor_struct

extern std::array<InterruptDescriptor, 256> idt;

// make_idt_attr
constexpr InterruptDescriptorAttribute
MakeIDTAttr(DescriptorType type, uint8_t descriptor_pribilege_level,
            bool present = true, uint8_t interrupt_stack_table = 0) {
  InterruptDescriptorAttribute attr{};
  // 割り込みハンドラのアドレスは３つに分割していれる
  attr.bits.interrupt_stack_table = interrupt_stack_table;
  attr.bits.type = type;
  attr.bits.descriptor_pribilege_level = descriptor_pribilege_level;
  attr.bits.present = present;
  return attr;
};
// make_idt_attr

void SetIDTEntry(InterruptDescriptor &desc, InterruptDescriptorAttribute attr,
                 uint64_t offset, uint16_t segment_selector);

// vector_numbers
class InterruptVector {
public:
  enum Number {
    kXHCI = 0x40,
  };
};
// vector_numbers
// frame_struct
struct InterruptFrame {
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
};
// frame_struct

void NotifyEndOfInterrupt();
