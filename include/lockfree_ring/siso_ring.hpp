#pragma once

#include <stdint.h>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace fast_com {
namespace ipc {

template <typename M>
class SISOFreeRing {
  using ItemPtr = std::shared_ptr<M>;

 public:
  explicit SISOFreeRing(uint32_t ring_size) {
    ring_size_ = ring_size;
    ring_buffer_.resize(ring_size_);

    /* prod_tail_ == cons_head_, empty
     * (prod_tail_ + 1) % ring_size_ == cons_head_, full
     */
    prod_head_ = 0;
    prod_tail_.store(0);

    cons_head_ = 0;
    cons_tail_.store(0);
  }

  void publish(ItemPtr item) { ring_sp_enqueue(item); }

  ItemPtr read() { return ring_sp_dequeue(); }

  ItemPtr read_latest() { return ring_sp_dequeue_latest_and_clear(); }

 private:
  bool is_empty() { return cons_tail_.load() == prod_tail_.load(); }

  bool is_full() {
    return ((prod_tail_.load() + 1) % ring_size_) == cons_tail_.load();
  }

  void add_index(uint32_t *now_index) {
    *now_index = (*now_index + 1) % ring_size_;
  }

  bool ring_sp_enqueue(ItemPtr pitem) {
    if (is_full()) {
      return false;
    }

    add_index(&prod_head_);
    ring_buffer_[prod_head_] = pitem;

    prod_tail_.store(prod_head_, std::memory_order_release);

    return true;
  }

  ItemPtr ring_sp_dequeue() {
    if (is_empty()) {
      return nullptr;
    }

    add_index(&cons_head_);
    ItemPtr pitem = ring_buffer_[cons_head_];

    cons_tail_.store(cons_head_, std::memory_order_release);

    return pitem;
  }

  ItemPtr ring_sp_dequeue_latest_and_clear() {
    if (is_empty()) {
      return nullptr;
    }

    cons_head_ = prod_tail_.load();
    ItemPtr pitem = ring_buffer_[cons_head_];

    cons_tail_.store(cons_head_, std::memory_order_release);

    return pitem;
  }

 private:
  uint32_t ring_size_;
  std::vector<ItemPtr> ring_buffer_;

  /*provider*/
  uint32_t prod_head_;
  volatile std::atomic<uint32_t> prod_tail_;

  /*consumer*/
  uint32_t cons_head_;
  volatile std::atomic<uint32_t> cons_tail_;
};

template <typename M>
using SISOFreeRingPtr = std::shared_ptr<SISOFreeRing<M>>;

}  // namespace ipc
}  // namespace fast_com
