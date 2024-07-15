#include "interrupt.hpp"

// idt_array
std::array<InterruptDescriptor, 256> idt;
// idt_array

// set_idt_entry
void SetIDTEntry(InterruptDescriptor &desc, InterruptDescriptorAttribute attr,
                 uint64_t offset, uint16_t segment_selector) {
  desc.attr = attr;
  desc.offset_low = offset & 0xffffu;
  desc.offset_middle = (offset >> 16) & 0xffffu;
  desc.offset_high = offset >> 32;
  desc.segment_selector = segment_selector;
}
// set_idt_entry

// notify_end of interrupt
void NotifyEndOfInterrupt() {
  volatile auto end_of_interrupt = reinterpret_cast<uint32_t *>(0xfee000b0);
  *end_of_interrupt = 0;
}
// notify_end of interrupt
