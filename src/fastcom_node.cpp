#include "node/fastcom_node.hpp"

namespace fast_com {
namespace ipc {

SINGLETON_DEFINE(FastComNode)

FastComNode::FastComNode(const std::string &config) {
  // do nothing now
}

void FastComNode::destory_instance() {}

}  // namespace ipc
}  // namespace fast_com
