#include "console.hpp"
#include "error.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "logger.hpp"
#include "mouse.hpp"
#include "pci.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <vector>

// void operator delete(void *obj) noexcept {}

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

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

  console->PutString(s);
  return result;
}
// printk

// mouse_observer
char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor *mouse_cursor;

void MouseObserver(std::int8_t displacement_x, int8_t displacement_y) {
  mouse_cursor->MoveRelative({displacement_x, displacement_y});
}
// mouse_observer

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

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config) {
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

  const int kFrameWidth = frame_buffer_config.horizontal_resolution;
  const int kFrameHeight = frame_buffer_config.vertical_resolution;

  // draw_desktop
  FillRectangle(*pixel_writer, {0, 0}, {kFrameWidth, kFrameHeight - 50},
                kDesktopBGColor);
  FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth, 50},
                {1, 8, 17});

  FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth / 5, 50},
                {80, 80, 80});
  FillRectangle(*pixel_writer, {10, kFrameHeight - 40}, {30, 30},
                {160, 160, 160});
  // draw_desktop

  console = new (console_buf)
      Console{*pixel_writer, kDesktopFGColor, kDesktopBGColor};

  printk("Welcome to MikanOS!\n");
  SetLogLevel(kWarn);

  // mouse_cursor
  mouse_cursor = new (mouse_cursor_buf)
      MouseCursor{pixel_writer, kDesktopBGColor, {300, 200}};
  // mouse_cursor

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
    Log(kInfo, "xhc is nullptr\n");
  }
  // find_xhc

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

  // configure port
  usb::HIDMouseDriver::default_observer = MouseObserver;

  for (int i = 1; i < xhc.MaxPorts(); ++i) {
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
  // configure port

  // receive_event
  while (1) {
    if (auto err = usb::xhci::ProcessEvent(xhc)) {
      Log(kError, "Error while ProcessEvent: %s at %s:%d\n", err.Name(),
          err.File(), err.Line());
    }
  }
  // receive_event

  while (1) {
    __asm__("hlt");
  }
}

extern "C" void __cxa_pure_virtual() {
  while (1)
    __asm__("hlt");
}
