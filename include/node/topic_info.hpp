#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include "lockfree_ring/siso_ring.hpp"

namespace fast_com {
namespace ipc {

template <typename M>
class TopicInfo {
  public:
  TopicInfo() {
    sub_seq_.store(-1);
    ring_vec_.resize(MAX_SUBCRIBER_NUMBER_);

    for (uint32_t i = 0; i < ring_vec_.size(); i++) {
      ring_vec_[i] = nullptr;
    }
  }

  /*the data ptr will free by shared_ptr,
    *caller won't free it
    */
  void publish(const M &data) {
    auto data_ptr = std::make_shared<M>(data);

    for (uint32_t i = 0; i < ring_vec_.size(); i++) {
      if (!ring_vec_[i]) {
        break;
      }

      ring_vec_[i]->publish(data_ptr);
    }
  }

  void publish(std::shared_ptr<M> data_ptr) {
    for (uint32_t i = 0; i < ring_vec_.size(); i++) {
      if (!ring_vec_[i]) {
        break;
      }
      ring_vec_[i]->publish(data_ptr);
    }
  }

  SISOFreeRingPtr<M> add_subscriber(uint32_t queue_size) {
    int sub_id;
    do {
      sub_id = sub_seq_.load();
    } while (!sub_seq_.compare_exchange_weak(sub_id, sub_id + 1,
                                              std::memory_order_acq_rel));

    sub_id += 1;

    auto ring = std::make_shared<SISOFreeRing<M>>(queue_size);
    ring_vec_[sub_id] = ring;

    return ring;
  }

  private:
  const int MAX_SUBCRIBER_NUMBER_ = 8;
  std::vector<SISOFreeRingPtr<M>> ring_vec_;
  std::atomic<int32_t> sub_seq_;
};

using TopicInfoPtr = void *;

}  // namespace ipc
}  // namespace fast_com