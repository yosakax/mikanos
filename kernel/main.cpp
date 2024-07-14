#include "frame_buffer_config.hpp"
#include <cstddef>
#include <cstdint>

// font a
const uint8_t kFontA[16] = {
    0b00000000, //
    0b00011000, //    **
    0b00011000, //    **
    0b00011000, //    **
    0b00011000, //    **
    0b00100100, //   *  *
    0b00100100, //   *  *
    0b00100100, //   *  *
    0b00100100, //   *  *
    0b01111110, //  ******
    0b01000010, //  *    *
    0b01000010, //  *    *
    0b01000010, //  *    *
    0b11100111, // ***  ***
    0b00000000, //
    0b00000000, //
};
// font a

struct PixelColor {
  uint8_t r, g, b;
};

// pixel_writer
class PixelWriter {
public:
  PixelWriter(const FrameBufferConfig &config) : config_{config} {}
  virtual ~PixelWriter() = default;
  virtual void Write(int x, int y, const PixelColor &c) = 0;

protected:
  uint8_t *PixelAt(int x, int y) {
    return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
  }

private:
  const FrameBufferConfig &config_;
};
// pixel_writer
// derived_pixel_writer
class RGBResv8BitPerColorPixelWriter : public PixelWriter {
public:
  using PixelWriter::PixelWriter;

  virtual void Write(int x, int y, const PixelColor &c) override {
    auto p = PixelAt(x, y);
    p[0] = c.r;
    p[1] = c.b;
    p[2] = c.g;
  }
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter {
public:
  using PixelWriter::PixelWriter;

  virtual void Write(int x, int y, const PixelColor &c) override {
    auto p = PixelAt(x, y);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
  }
};
// derived_pixel_writer

// write_ascii
void WriteAscii(PixelWriter &writer, int x, int y, char c,
                const PixelColor &color) {
  if (c != 'A') {
    return;
  }
  for (int dy = 0; dy < 16; ++dy) {
    for (int dx = 0; dx < 8; ++dx) {
      if ((kFontA[dy] << dx) & 0x80u) {
        writer.Write(x + dx, y + dy, color);
      }
    }
  }
}
// write_ascii

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
  // write aa
  WriteAscii(*pixel_writer, 50, 50, 'A', {0, 0, 0});
  WriteAscii(*pixel_writer, 58, 50, 'A', {0, 0, 0});
  // write aa

  while (1) {
    __asm__("hlt");
  }
}
