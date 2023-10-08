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
#include "lockfree_ring/siso_ring.hpp"
#include "util/timing.h"

#define THREAD_NAME_MAXLEN 15
#define THREAD_NAME_MODULENAME_LEN 4

#define errExitEN(en, msg) \
  do {                     \
    errno = en;            \
    exit(EXIT_FAILURE);    \
  } while (0)

namespace fast_com {
namespace ipc {

class InpcomSIMO {
 public:
  template <typename M>
  using ReceiveCallback = std::function<void(std::shared_ptr<M>)>;

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

  template <typename M>
  class Subscriber {
   public:
    Subscriber(SISOFreeRingPtr<M> ring_ptr, ReceiveCallback<M> cb,
               uint32_t max_delay_ms_limit) {
      ring_ptr_ = ring_ptr;
      is_start_ = true;
      cb_ = cb;
      max_delay_ms_limit_ = max_delay_ms_limit;

      receiver_ = std::make_shared<std::thread>(&Subscriber::run, this);
    }

    ~Subscriber() {
      is_start_ = false;

      if (receiver_->joinable()) {
        receiver_->join();
      }
    }

    // inpcom-name: inpcom_topic-name_mapmoud-name
    //                |         |           |
    //           default index  |           |
    //                    subscriber topic  |
    //                               default:unknow;
    int set_thread_name(pthread_t tid, const char *_topic_name,
                        const char *_module_name = "unknown") {
      int rc = 0;
      char theName[THREAD_NAME_MAXLEN + 1] = {0};  // name space size must be
                                                   // larger than the
                                                   // thread_name limitation 15
      int TOPIC_NAME_LEN = 0;
      std::string topicname_t, modulename_t;
      const std::string thread_name_header = "ipm_";

      // printf("thread_name: %s_%s_%s\n", thread_name_header.data(),
      // _topic_name,
      //        _module_name);

      TOPIC_NAME_LEN = THREAD_NAME_MAXLEN - thread_name_header.length() -
                       THREAD_NAME_MODULENAME_LEN;

      topicname_t = _topic_name;  // char* to string
      modulename_t = _module_name;

      topicname_t = topicname_t.substr(0, TOPIC_NAME_LEN);
      // thread_name: ipm_mfr/module_name
      modulename_t = modulename_t.substr(0, THREAD_NAME_MODULENAME_LEN);
      std::string _name = thread_name_header + topicname_t + "_" + modulename_t;

      _name = _name.substr(0, THREAD_NAME_MAXLEN);  //名字最长为15个字符
      rc = pthread_setname_np(tid, _name.data());

      if (0 == rc) {
        rc = pthread_getname_np(tid, theName, THREAD_NAME_MAXLEN + 1);  // 9

      } else {
        errExitEN(rc, "pthread_getname_np");
      }

      return (rc);
    }

   private:
    void run() {
      auto tid = pthread_self();
      std::string stid = std::to_string(tid);

      set_thread_name(tid, stid.data());

      while (is_start_) {
        std::shared_ptr<M> new_item = ring_ptr_->read_latest();
        if (!new_item) {
          util::TimerHelper::sleep_ms(max_delay_ms_limit_);
          continue;
        }
        cb_(new_item);
      }
    }

   private:
    uint32_t max_delay_ms_limit_;
    ReceiveCallback<M> cb_;
    std::shared_ptr<std::thread> receiver_;
    bool is_start_;
    SISOFreeRingPtr<M> ring_ptr_;
  };

  template <typename M>
  class Publisher {
   public:
    Publisher(TopicInfo<M> *tif_ptr) { tif_ptr_ = tif_ptr; }

    void publish(const M &data) { tif_ptr_->publish(data); }

    void publish(std::shared_ptr<M> data) { tif_ptr_->publish(data); }

   private:
    TopicInfo<M> *tif_ptr_;
  };

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

  SINGLETON_DECLARE(InpcomSIMO)
};

using InpcomSIMOPtr = std::shared_ptr<InpcomSIMO>;

}  // namespace ipc
}  // namespace fast_com