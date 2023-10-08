/* -*- mode:c -*- */
#pragma once

#include <functional>
template <typename M>
using ReceiveCallback = std::function<void(std::shared_ptr<M>)>;

#define errExitEN(en, msg) \
  do {                     \
    errno = en;            \
    exit(EXIT_FAILURE);    \
  } while (0)
