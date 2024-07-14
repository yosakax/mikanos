#pragma once
#include "graphics.hpp"

class Console {
public:
  /*
   * NOTE: クラス内でstatic<type>
   * で定義した変数はメモリに乗ってから終了するまで存在する静的な変数になる
   * */
  static const int kRows = 25, kColumns = 80;

  Console(PixelWriter &writer, const PixelColor &fg_color,
          const PixelColor &bg_color);

  void PutString(const char *s);

private:
  void Newline();

  PixelWriter &writer_;
  const PixelColor fg_color_, bg_color_;
  char buffer_[kRows][kColumns];
  int cursor_row_, cursor_column_;
};
