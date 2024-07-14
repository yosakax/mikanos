#include "mouse.hpp"

#include "graphics.hpp"

namespace {

// #@@range_begin(mosue_cursor_shape)
const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
// clang-format off
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
  "@              ",
  "@@             ",
  "@.@            ",
  "@..@           ",
  "@...@          ",
  "@....@         ",
  "@.....@        ",
  "@......@       ",
  "@.......@      ",
  "@........@     ",
  "@.........@    ",
  "@..........@   ",
  "@...........@  ",
  "@............@ ",
  "@......@@@@@@@@",
  "@......@       ",
  "@....@@.@      ",
  "@...@ @.@      ",
  "@..@   @.@     ",
  "@.@    @.@     ",
  "@@      @.@    ",
  "@       @.@    ",
  "         @.@   ",
  "         @@@   ",
};
// clang-format on
// #@@range_end(mosue_cursor_shape)
}; // namespace

void DrawMouseCursor(PixelWriter *pixel_writer, Vector2D<int> position) {
  for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cursor_shape[dy][dx] == '@') {
        pixel_writer->Write(position.x + dx, position.y + dy, {0, 0, 0});
      } else if (mouse_cursor_shape[dy][dx] == '.') {
        pixel_writer->Write(position.x + dx, position.y + dy, {255, 255, 255});
      }
    }
  }
}

void EraseMouseCursor(PixelWriter *pixel_writer, Vector2D<int> position,
                      PixelColor erase_color) {
  for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cursor_shape[dy][dx] != ' ') {
        pixel_writer->Write(position.x + dx, position.y + dy, erase_color);
      }
    }
  }
}

// mouse_class
MouseCursor::MouseCursor(PixelWriter *writer, PixelColor erase_color,
                         Vector2D<int> initial_position)
    // clang-format off
    : pixel_writer_{writer}, 
      erase_color_{erase_color}, 
      position_{initial_position} {
  // clang-format on
  DrawMouseCursor(pixel_writer_, position_);
}

void MouseCursor::MoveRelative(Vector2D<int> displacement) {
  EraseMouseCursor(pixel_writer_, position_, erase_color_);
  position_ += displacement;
  DrawMouseCursor(pixel_writer_, position_);
}

// mouse_class
