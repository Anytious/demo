#pragma once

#include <errno.h>
#include <string.h>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

#include "general/designing_pattern.h"
#include "general/common_type.h"
#include "lockfree_ring/siso_ring.hpp"
#include "util/timing.h"
#include "node/topic_info.hpp"
#include "node/subscriber.hpp"
#include "node/publisher.hpp"

namespace fast_com {
namespace ipc {

class FastComNode {
 public:
  template <typename M>
  std::shared_ptr<Publisher<M>> advertise(const std::string &topic_name) {
    TopicInfo<M> *tif;

    std::lock_guard<std::mutex> lg(config_lock_);

    auto iter = database_.find(topic_name);
    if (iter == database_.end()) {
      tif = new TopicInfo<M>();
      database_.insert(std::make_pair(topic_name, (void *)tif));
    } else {
      tif = (TopicInfo<M> *)iter->second;
    }

    return std::make_shared<Publisher<M>>(tif);
  }

  template <typename M>
  std::shared_ptr<Subscriber<M>> subscribe(const std::string &topic_name,
                                           ReceiveCallback<M> cb,
                                           uint32_t max_delay_ms_limit = 10,
                                           uint32_t queue_size = 16) {
    TopicInfo<M> *tif;

    std::lock_guard<std::mutex> lg(config_lock_);

    auto iter = database_.find(topic_name);
    if (iter == database_.end()) {
      tif = new TopicInfo<M>();
      database_.insert(std::make_pair(topic_name, (void *)tif));
    } else {
      tif = (TopicInfo<M> *)iter->second;
    }

    SISOFreeRingPtr<M> ring = tif->add_subscriber(queue_size);

    auto sub = std::make_shared<Subscriber<M>>(ring, cb, max_delay_ms_limit);

    return sub;
  }

 private:
  std::mutex config_lock_;
  std::map<std::string, TopicInfoPtr> database_;

  SINGLETON_DECLARE(FastComNode)
};

using FastComNodePtr = std::shared_ptr<FastComNode>;

}  // namespace ipc
}  // namespace fast_com