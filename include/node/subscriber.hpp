#include <thread>
#include <string>
#include <memory>
#include <functional>
#include "general/common_type.h"
#include "lockfree_ring/siso_ring.hpp"

namespace fast_com {
namespace ipc {

#define THREAD_NAME_MAXLEN 15
#define THREAD_NAME_MODULENAME_LEN 4

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

  // fastcom-name: fastcom_topic-name_mapmoud-name
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
    const std::string thread_name_header = "fastcom_";

    // printf("thread_name: %s_%s_%s\n", thread_name_header.data(),
    // _topic_name,
    //        _module_name);

    TOPIC_NAME_LEN = THREAD_NAME_MAXLEN - thread_name_header.length() -
                      THREAD_NAME_MODULENAME_LEN;

    topicname_t = _topic_name;  // char* to string
    modulename_t = _module_name;

    topicname_t = topicname_t.substr(0, TOPIC_NAME_LEN);
    // thread_name: fastcom/module_name
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

}  // namespace ipc
}  // namespace fast_com