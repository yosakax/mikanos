#pragma once

#include "error.hpp"
#include "logger.hpp"
#include <array>
#include <cstdint>
#include <type_traits>

namespace pci {
// condig_addr
// CONFIG_ADDRESS レジスタのIOポートアドレス
const uint16_t kConfigAddress = 0x0cf8;
// CONDIF_DATAレジスタのIOポートアドレス
const uint16_t kConfigData = 0x0cfc;
// condig_addr

// class_code
/** @brief PCIデバイスのクラスコード */
struct ClassCode {
  uint8_t base, sub, interface;

  /** @brief ベースクラスが等しい場合にTrueを返す */
  bool Match(uint8_t b) {
    // Log(kDebug, "hit base");
    return b == base;
  }
  /** @brief ベースクラスとサブクラスが等しい場合にTrueを返す */
  bool Match(uint8_t b, uint8_t s) {
    // Log(kDebug, "hit sub");
    return Match(b) && s == sub;
  }
  /** @brief ベースクラスとサブクラスとインターフェース等しい場合にTrueを返す */
  bool Match(uint8_t b, uint8_t s, uint8_t i) {
    // Log(kDebug, "hit interface!\n");
    return Match(b, s) && i == interface;
  }
};
// class_code

/** @brief PCIデバイスを操作するための基礎データを格納する
 *  バス番号，デバイス番号，ファンクション番号はデバイスを特定するために必須
 *  その他の情報は単に利便性のために加えてある
 * */
struct Device {
  uint8_t bus, device, function, header_type;
  ClassCode class_code;
};

/** @brief CONFIG_ADDRESSに指定された整数を書き込む */
void WriteAddress(uint32_t address);

/** @brief CONDIF_DATAに指定された整数を書き込む */
void WriteData(uint32_t value);

/** @brief CONFIG_DATAから32ビット整数を読み込む */
uint32_t ReadData();

/** @brief デバイスIDレジスタを読み取る（全ヘッダタイプ共通） */
uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);
/** @brief ヘッダタイプIDレジスタを読み取る（全ヘッダタイプ共通） */
uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);

/** @brief クラスコードレジスタを読み取る(全ヘッダタイプ共通) */
ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);

/** @brief ベンダIDレジスタを読み取る（全ヘッダタイプ共通） */
uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);

inline uint16_t ReadVendorId(const Device &dev) {
  return ReadVendorId(dev.bus, dev.device, dev.function);
}

uint32_t ReadConfigReg(const Device &dev, uint8_t reg_addr);

void WriteConfReg(const Device &dev, uint8_t reg_addr, uint32_t value);

/** @brief バス番号レジスタを読み取る(ヘッダタイプ１用)
 * 返される32ビット整数の構造は次の通り
 * - 23:16: sub ordinate bus number
 * - 15:8: secondary bus number
 * - 7:0 revision
 * */
uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

/** @brief 単一ファンクションの場合にTrueを返す*/
bool IsSingleFunctionDevice(uint8_t header_type);

// var_devices
/** @brief ScanAllBus()により発見されたPCIデバイスの一覧*/
inline std::array<Device, 32> devices;
/** @brief devicesの有効な要素の数*/
inline int num_device;

/** @brief PCIデバイスをすべて探索し，devicesに格納する
 *  バス０から再帰的にPCIデバイスを探索し，devicesの先頭から詰めて書き込む
 *  発見したデバイスの数をnum_deviceに設定する
 * */
Error ScanAllBus();
// var_devices

constexpr uint8_t CalcBarAddress(unsigned int bar_index) {
  return 0x10 + 4 * bar_index;
}

WithError<uint64_t> ReadBar(Device &device, unsigned int bar_index);

}; // namespace pci
