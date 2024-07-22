#pragma once
#include <map>
#include <memory>
#include <vector>

#include "frame_buffer.hpp"
#include "graphics.hpp"
#include "window.hpp"

// layer
/** @brief Layerは1つの層を表す
 * 現状では1つのウィンドウしか持てない設計だが，
 * 将来的には複数のウィンドウを持ちうる
 * */
class Layer {
public:
  /** @brief 指定されたIDを持つレイヤを生成する */
  Layer(unsigned int id = 0);
  /** @brief このインスタンスのIDを返す */
  unsigned int ID() const;

  /** @brief ウィンドウを設定する 既存のウィンドウはこのレイヤから外れる*/
  Layer &SetWindow(const std::shared_ptr<Window> &window);
  /** @brief 設定されたウィンドウを返す*/
  std::shared_ptr<Window> GetWindow() const;

  /** @brief レイヤの位置情報を指定された絶対座標へと更新する。再描画はしない */
  Layer &Move(Vector2D<int> pos);

  /** @brief レイヤの位置情報を指定された相対座標へと更新する。再描画はしない */
  Layer &MoveRelative(Vector2D<int> pos_diff);

  /** @brief 指定された描画先にウィンドウの内容を描画する */
  void DrawTo(FrameBuffer &screen) const;

private:
  unsigned int id_;
  Vector2D<int> pos_;
  std::shared_ptr<Window> window_;
};
// layer

// layer_manager
/** brief LayerManagerは複数のLayerを管理する */
class LayerManager {
public:
  /** @brief Drawメソッドなどで描画する際の描画先を設定する */
  void SetWriter(FrameBuffer *screen);

  /** @brief 新しいレイヤを生成して参照を返す
   * 新しく生成されたレイヤの実態はLayerManager内部のコンテナで保持される
   * */
  Layer &NewLayer();

  /** @brief 現在表示状態にあるレイヤを描画する*/
  void Draw() const;

  /** @brief レイヤの位置情報を指定された絶対場へと更新する 再描画はしない */
  void Move(unsigned int id, Vector2D<int> new_position);

  /** @brief レイヤの位置情報を指定された相対場へと更新する 再描画はしない */
  void MoveRelative(unsigned int id, Vector2D<int> pos_diff);

  /** @brief レイヤの高さ方向の位置を指定された位置に移動する
   *
   * new_heightに負の高さを指定するとレイヤは非表示となり，
   * 0以上を指定するとその高さになる
   * 現在のレイヤ数以上の数値を指定した場合は最前面のレイヤとなる
   * */
  void UpDown(unsigned int id, int new_height);
  /** @brief レイヤを非表示にする */
  void Hide(unsigned int id);

private:
  FrameBuffer *screen_{nullptr};
  std::vector<std::unique_ptr<Layer>> layers_{};
  std::vector<Layer *> layer_stack_{};
  unsigned int latest_id_{0};

  Layer *FindLayer(unsigned int id);
};

extern LayerManager *layer_manager;

// layer_manager
