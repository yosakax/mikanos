#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include <cstddef>

// placement_new
void *operator new(size_t size, void *buf) { return buf; }

void operator delete(void *obj) noexcept {}

// placement_new
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

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

  for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
    for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
      pixel_writer->Write(x, y, {255, 255, 255});
    }
  }

  for (int x = 100; x < 300; ++x) {
    for (int y = 100; y < 200; ++y) {
      pixel_writer->Write(x, y, {0, 255, 0});
    }
  }

  // write font
  int i = 0;
  for (char c = '!'; c <= '~'; ++c, ++i) {
    WriteAscii(*pixel_writer, 8 * i, 50, c, {0, 0, 0});
  }
  // write font

  while (1) {
    __asm__("hlt");
  }
}
