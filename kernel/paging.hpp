#pragma once

#include <cstddef>

// 1つのページディレクトリには512個の2MiBページが設定できることから，
// kPageDirectoryCount * 1GiB の仮想アドレスがマッピングされることになる
const size_t kPageDirectoryCount = 64;

/** @brief 仮想アドレスと物理アドレスが一致するようにページテーブルを設定する
 * 最終的にはCR3レジスタが正しく設定されたページテーブルを指すようになる
 * */
void SetupIdentityPageTable();
