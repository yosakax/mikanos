#include "window.hpp"
#include "frame_buffer.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "logger.hpp"

// window_ctor
Window::Window(int width, int height, PixelFormat shadow_format)
    : width_{width}, height_{height} {
  data_.resize(height);
  for (int y = 0; y < height; ++y) {
    data_[y].resize(width);
  }

  FrameBufferConfig config{};
  config.frame_buffer = nullptr;
  config.horizontal_resolution = width;
  config.vertical_resolution = height;
  config.pixel_format = shadow_format;

  if (auto err = shadow_buffer_.Initialize(config)) {
    Log(kError, "failed to initialize shadow buffer: %s at %s:%d\n", err.Name(),
        err.File(), err.Line());
  }
}
// window_ctor

// window_drawto
void Window::DrawTo(FrameBuffer &dst, Vector2D<int> position) {
  if (!transparent_color_) {
    dst.Copy(position, shadow_buffer_);
    return;
  }

  const auto tc = transparent_color_.value();
  auto &writer = dst.Writer();
  for (int y = 0; y < Height(); ++y) {
    for (int x = 0; x < Width(); ++x) {
      const auto c = At(Vector2D<int>{x, y});
      if (c != tc) {
        writer.Write(position + Vector2D<int>{x, y}, c);
      }
    }
  }
}
// window_drawto

// window_settc
void Window::SetTransparentColor(std::optional<PixelColor> c) {
  transparent_color_ = c;
}
// window_settc

Window::WindowWriter *Window::Writer() { return &writer_; }

const PixelColor &Window::At(Vector2D<int> pos) const {
  return data_[pos.y][pos.x];
}

void Window::Write(Vector2D<int> pos, PixelColor c) {
  data_[pos.y][pos.x] = c;
  shadow_buffer_.Writer().Write(pos, c);
}

int Window::Width() const { return width_; }

int Window::Height() const { return height_; }
