#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <numeric>
#include <vector>

#include "asmfunc.h"
#include "console.hpp"
#include "error.hpp"
#include "font.hpp"
#include "frame_buffer.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "interrupt.hpp"
#include "layer.hpp"
#include "logger.hpp"
#include "memory_manager.hpp"
#include "memory_map.hpp"
#include "mouse.hpp"
#include "paging.hpp"
#include "pci.hpp"
#include "queue.hpp"
#include "segment.hpp"
#include "timer.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"

// void operator delete(void *obj) noexcept {}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

// console_buf
char console_buf[sizeof(Console)];
Console *console;
// console_buf

// printk
int printk(const char *format, ...) {
  va_list ap;
  int result;
  char s[1024];

  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);

  // StartLAPICTimer();
  console->PutString(s);
  // auto elapsed = LAPICTimerElapsed();
  // StopLAPICTimer();
  // sprintf(s, "[%09d]", elapsed);
  // console->PutString(s);
  return result;
}
// printk

// memman_buf
char memory_manager_buf[sizeof(BitMapMemoryManager)];
BitMapMemoryManager *memory_manager;
// memman_buf

// layermgr_mouse_observer
unsigned int mouse_layer_id;
Vector2D<int> screen_size;
Vector2D<int> mouse_position;

void MouseObserver(std::int8_t displacement_x, int8_t displacement_y) {
  auto newpos = mouse_position + Vector2D<int>{displacement_x, displacement_y};
  // 右端対策
  newpos = ElementMin(newpos, screen_size + Vector2D<int>{-kMouseCursorWidth,
                                                          -kMouseCursorHeight});
  // 左端対策
  mouse_position = ElementMax(newpos, Vector2D<int>{0, 0});

  layer_manager->Move(mouse_layer_id, mouse_position);
  // StartLAPICTimer();
  layer_manager->Draw();
  // auto elapsed = LAPICTimerElapsed();
  // StopLAPICTimer();
  // printk("MouseObserver: elapsed = %u\n", elapsed);
}
// layermgr_mouse_observer

// switch_echi2xhci
void SwitchEhci2Xhci(const pci::Device &xhc_dev) {
  bool intel_ehc_exist = false;
  for (int i = 0; i < pci::num_device; ++i) {
    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) /*EHCI*/
        && 0x8086 == pci::ReadVendorId(pci::devices[i])) {
      intel_ehc_exist = true;
      break;
    }
  }
  if (!intel_ehc_exist) {
    return;
  }

  uint32_t superspeed_ports = pci::ReadConfigReg(xhc_dev, 0xdc); // USB3PRM
  pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports);            // USB3_PSSEN
  uint32_t ehci2xhci_ports = pci::ReadConfigReg(xhc_dev, 0xd4);  // XUS2PRM
  pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports);             // XUSB2PR
  Log(kDebug, "SwitchEhci2Xhci: SS = %02, xHCI = %02x\n", superspeed_ports,
      ehci2xhci_ports);
}
// switch_echi2xhci

// queue_message
struct Message {
  enum Type {
    kInterruptXHCI,
  } type;
};

ArrayQueue<Message> *main_queue;
// queue_message

// xhci_handler
usb::xhci::Controller *xhc;

__attribute__((interrupt)) void IntHandlerXHCI(InterruptFrame *frame) {
  main_queue->Push(Message{Message::kInterruptXHCI});
  NotifyEndOfInterrupt();
}
// xhci_handler

// main_new_stack

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void
KernelMainNewStack(const FrameBufferConfig &frame_buffer_config_ref,
                   const MemoryMap &memory_map_ref) {
  FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
  MemoryMap memory_map{memory_map_ref};
  // main_new_stack

  // reinterpret_cast<T>はただの(T)val と同じ型のキャストだが，
  // 整数からポインタへの型変換であることを明示することができてよい
  switch (frame_buffer_config.pixel_format) {
  case kPixelRGBResv8BitPerColor:
    pixel_writer = new (pixel_writer_buf)
        RGBResv8BitPerColorPixelWriter{frame_buffer_config};
    break;
  case kPixelBGRResv8BitPerColor:
    pixel_writer = new (pixel_writer_buf)
        BGRResv8BitPerColorPixelWriter{frame_buffer_config};
    break;
  }

  console = new (console_buf) Console{kDesktopFGColor, kDesktopBGColor};
  console->SetWriter(pixel_writer);

  printk("Welcome to MikanOS!\n");
  SetLogLevel(kWarn);
  InitializeLAPICTimer();

  // setup_segments_and_page
  SetupSegments();

  const uint16_t kernel_cs = 1 << 3;
  const uint16_t kernel_ss = 2 << 3;
  SetDSAll(0);
  SetCSSS(kernel_cs, kernel_ss);

  SetupIdentityPageTable();
  // setup_segments_and_page

  // mark_allocated
  ::memory_manager = new (memory_manager_buf) BitMapMemoryManager;

  // print_memory_map
  const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
  uintptr_t available_end = 0;

  for (uintptr_t iter = memory_map_base;
       iter < memory_map_base + memory_map.map_size;
       iter += memory_map.descriptor_size) {
    auto desc = reinterpret_cast<MemoryDescriptor *>(iter);
    if (available_end < desc->physical_start) {
      memory_manager->MarkAllocated(FrameID{available_end / kBytesPerFrame},
                                    (desc->physical_start - available_end) /
                                        kBytesPerFrame);
    }

    const auto physical_end =
        desc->physical_start + desc->number_of_pages * kUEFIPageSize;
    if (IsAvailable(static_cast<MemoryType>(desc->type))) {
      available_end = physical_end;
      printk("type = %u, phys = %08lx - %08lx, pages = %lu, attr = %08lx\n",
             desc->type, desc->physical_start,
             desc->physical_start + desc->number_of_pages * 4096 - 1,
             desc->number_of_pages, desc->attribute);
    } else {
      memory_manager->MarkAllocated(
          FrameID{desc->physical_start / kBytesPerFrame},
          desc->number_of_pages * kUEFIPageSize / kBytesPerFrame);
    }
  }
  memory_manager->SetMemoryRange(FrameID{1},
                                 FrameID{available_end / kBytesPerFrame});
  // mark_allocated

  // initialize_heap
  if (auto err = InitializeHeap(*memory_manager)) {
    Log(kError, "failed to allocate pages: %s at %s: %d\n", err.Name(),
        err.File(), err.Line());
    exit(1);
  }
  // initialize_heap

  std::array<Message, 32> main_queue_data;
  ArrayQueue<Message> main_queue{main_queue_data};
  ::main_queue = &main_queue;

  // show_devices
  auto err = pci::ScanAllBus();
  printk("ScanAllBus: %s\n", err.Name());

  for (int i = 0; i < pci::num_device; ++i) {
    const auto &dev = pci::devices[i];
    auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
    auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    printk("%d.%d.%d: vend %04x, class %08x, head %02x\n", dev.bus, dev.device,
           dev.function, vendor_id, class_code, dev.header_type);
  }
  // show_devices

  // find_xhc
  // Intel製を優先してxHCを探す
  pci::Device *xhc_dev = nullptr;
  for (int i = 0; i < pci::num_device; ++i) {
    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
      xhc_dev = &pci::devices[i];
      // Log(kDebug, "vendorid : %d\n", pci::ReadVendorId(*xhc_dev));

      if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
        break;
      }
    }
  }

  if (xhc_dev) {
    Log(kInfo, "xHC has been found: %d.%d.%d\n", xhc_dev->bus, xhc_dev->device,
        xhc_dev->function);
  } else {
    Log(kError, "xhc is nullptr\n");
  }
  // find_xhc

  // load_idt
  SetIDTEntry(idt[InterruptVector::kXHCI],
              MakeIDTAttr(DescriptorType::kInterruptGate, 0),
              reinterpret_cast<uint64_t>(IntHandlerXHCI), kernel_cs);
  LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
  // load_idt

  // configure_msi
  const uint8_t bsp_local_apic_id =
      *reinterpret_cast<const uint32_t *>(0xfee00020) >> 24;
  pci::ConfigureMSIFixedDestination(
      *xhc_dev, bsp_local_apic_id, pci::MSITriggerMode::kLevel,
      pci::MSIDeliveryMode::kFixed, InterruptVector::kXHCI, 0);
  // configure_msi

  // read_bar
  const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
  Log(kDebug, "ReadBar:%s\n", xhc_bar.error.Name());
  const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
  Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);
  // read_bar

  // init_xhc
  usb::xhci::Controller xhc{xhc_mmio_base};

  if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
    SwitchEhci2Xhci(*xhc_dev);
  }

  {
    auto err = xhc.Initialize();
    Log(kDebug, "xhc.Initialize: %s\n", err.Name());
  }

  Log(kInfo, "xHC starting!\n");
  xhc.Run();
  // init_xhc

  ::xhc = &xhc;
  __asm__("sti");

  // configure port
  usb::HIDMouseDriver::default_observer = MouseObserver;

  for (int i = 1; i <= xhc.MaxPorts(); ++i) {
    auto port = xhc.PortAt(i);
    Log(kDebug, "Port %d: IsConnected = %d\n", i, port.IsConnected());

    if (port.IsConnected()) {
      if (auto err = usb::xhci::ConfigurePort(xhc, port)) {
        Log(kError, "failed to configure port: %s at %s:%d\n", err.Name(),
            err.File(), err.Line());
        continue;
      }
    }
  }
  printk("kokomade kita!\n");
  // configure port
  // main_window
  screen_size.x = frame_buffer_config.horizontal_resolution;
  screen_size.y = frame_buffer_config.vertical_resolution;

  auto bgwindow = std::make_shared<Window>(screen_size.x, screen_size.y,
                                           frame_buffer_config.pixel_format);

  auto bgwriter = bgwindow->Writer();

  DrawDesktop(*bgwriter);
  console->SetWindow(bgwindow);
  console->SetWriter(bgwriter);

  auto mouse_window = std::make_shared<Window>(
      kMouseCursorWidth, kMouseCursorHeight, frame_buffer_config.pixel_format);
  mouse_window->SetTransparentColor(kMouseTransparentColor);
  DrawMouseCursor(mouse_window->Writer(), {0, 0});
  mouse_position = {200, 200};

  // make_window
  auto main_window =
      std::make_shared<Window>(160, 52, frame_buffer_config.pixel_format);
  DrawWindow(*main_window->Writer(), "Hellow_window");
  // make_window

  // create_screen
  FrameBuffer screen;
  if (auto err = screen.Initialize(frame_buffer_config)) {
    Log(kError, "failed to initialize frame buffer: %s at %s:%d\n", err.Name(),
        err.File(), err.Line());
  }
  // create_screen

  layer_manager = new LayerManager;
  layer_manager->SetWriter(&screen);

  auto bglayer_id =
      layer_manager->NewLayer().SetWindow(bgwindow).Move({0, 0}).ID();
  mouse_layer_id = layer_manager->NewLayer()
                       .SetWindow(mouse_window)
                       .Move(mouse_position)
                       .ID();
  auto main_window_layer_id =
      layer_manager->NewLayer().SetWindow(main_window).Move({300, 100}).ID();

  layer_manager->UpDown(bglayer_id, 0);
  layer_manager->UpDown(mouse_layer_id, 1);
  layer_manager->UpDown(main_window_layer_id, 1);
  layer_manager->Draw();

  // main_window
  printk("kokomade kita!\n");

  char str[128];
  unsigned int count = 0;

  while (true) {

    ++count;
    count %= 100000;
    sprintf(str, "%010u", count);
    FillRectangle(*main_window->Writer(), {24, 28}, {8 * 10, 16},
                  {0xc6, 0xc6, 0xc6});
    WriteString(*main_window->Writer(), {24, 28}, str, {0, 0, 0});
    layer_manager->Draw();

    __asm__("cli");

    if (main_queue.Count() == 0) {
      __asm__("sti\n\thlt");
      continue;
    }

    Message msg = main_queue.Front();
    main_queue.Pop();
    __asm__("sti");

    // get_front_message
    switch (msg.type) {
    case Message::kInterruptXHCI:
      while (xhc.PrimaryEventRing()->HasFront()) {
        if (auto err = ProcessEvent(xhc)) {
          Log(kError, "Error while ProcessEvent: %s at %s:%d\n", err.Name(),
              err.File(), err.Line());
        }
      }
      break;
    default:
      Log(kError, "Unknown message type: %d\n", msg.type);
      // break;
    }
    // get_front_message
  }
}

extern "C" void __cxa_pure_virtual() {
  while (1)
    __asm__("hlt");
}
