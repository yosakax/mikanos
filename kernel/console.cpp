#include <cstring>

#include "console.hpp"
#include "font.hpp"
#include "graphics.hpp"
#include "layer.hpp"

// constructor
Console::Console(const PixelColor &fg_color, const PixelColor &bg_color)
    : writer_{nullptr}, fg_color_{fg_color}, bg_color_{bg_color}, buffer_{},
      cursor_row_{0}, cursor_column_{0} {}
// constructor

// put_string
void Console::PutString(const char *s) {
  while (*s) {
    if (*s == '\n') {
      Newline();
    } else if (cursor_column_ < kColumns - 1) {
      WriteAscii(*writer_, Vector2D<int>{cursor_column_ * 8, 16 * cursor_row_},
                 *s, fg_color_);
      buffer_[cursor_row_][cursor_column_] = *s;
      cursor_column_++;
    }
    ++s;
  }
  if (layer_manager) {
    layer_manager->Draw();
  }
}
// put_string

void Console::SetWriter(PixelWriter *writer) {
  if (writer == writer_) {
    return;
  }
  writer_ = writer;
  Refresh();
}

// newline
void Console::Newline() {
  cursor_column_ = 0;
  if (cursor_row_ < kRows - 1) {
    cursor_row_++;
  } else {
    // 背景色に塗りつぶし
    for (int y = 0; y < 16 * kRows; ++y) {
      for (int x = 0; x < 8 * kColumns; ++x) {
        writer_->Write(Vector2D<int>{x, y}, bg_color_);
      }
    }
    // 改行した文字列を書き込む
    for (int row = 0; row < kRows - 1; ++row) {
      // memcpy(dst, src, size):  srcの戦闘からsizeバイトをdestにこぴーする　
      memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
      WriteString(*writer_, Vector2D<int>{0, 16 * row}, buffer_[row],
                  fg_color_);
    }
    // memset(s, c, n): sをnバイト分cで埋める
    memset(buffer_[kRows - 1], 0, kColumns + 1);
  }
}
// newline

// console_refresh
void Console::Refresh() {
  for (int row = 0; row < kRows; ++row) {
    WriteString(*writer_, Vector2D<int>{0, 16 * row}, buffer_[row], fg_color_);
  }
}
// console_refresh
