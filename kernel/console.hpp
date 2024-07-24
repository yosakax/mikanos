#pragma once

#include <memory>

#include "graphics.hpp"
#include "window.hpp"

class Console {
public:
  /*
   * NOTE: クラス内でstatic<type>
   * で定義した変数はメモリに乗ってから終了するまで存在する静的な変数になる
   * */
  static const int kRows = 25, kColumns = 80;
  Console(const PixelColor &fg_color, const PixelColor &bg_color);

  void PutString(const char *s);
  void SetWriter(PixelWriter *writer);
  void SetWindow(const std::shared_ptr<Window> &window);

private:
  void Newline();
  void Refresh();

  PixelWriter *writer_;
  std::shared_ptr<Window> window_;
  const PixelColor fg_color_, bg_color_;
  char buffer_[kRows][kColumns];
  int cursor_row_, cursor_column_;
};
