#include "topic_info.hpp"

namespace fast_com {
namespace ipc {

template <typename M>
class Publisher {
  public:
  Publisher(TopicInfo<M> *tif_ptr) { tif_ptr_ = tif_ptr; }

  void publish(const M &data) { tif_ptr_->publish(data); }

  void publish(std::shared_ptr<M> data) { tif_ptr_->publish(data); }

  private:
  TopicInfo<M> *tif_ptr_;
};

}  // namespace ipc
}  // namespace fast_com