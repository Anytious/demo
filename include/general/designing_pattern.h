/* -*- mode:c -*- */
#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unistd.h>

#define SINGLETON_DECLARE(classname)                          \
                                                              \
 public:                                                      \
  static classname *get_instance(const std::string &config) { \
    construct_lock.lock();                                    \
    if (getpid() != pid_ || __instance_ == nullptr) {         \
      __instance_ = new classname(config);                    \
      pid_ = getpid();                                        \
    }                                                         \
    construct_lock.unlock();                                  \
    return __instance_;                                       \
  }                                                           \
  static classname *get_instance(void *args = nullptr) {      \
    construct_lock.lock();                                    \
    if (getpid() != pid_ || __instance_ == nullptr) {         \
      __instance_ = new classname(args);                      \
      pid_ = getpid();                                        \
    }                                                         \
    construct_lock.unlock();                                  \
    return __instance_;                                       \
  }                                                           \
  virtual ~classname() { destory_instance(); }                \
                                                              \
 private:                                                     \
  explicit classname(const std::string &config);      \
  explicit classname(void *args);                     \
  void destory_instance();                            \
  static classname *__instance_;                      \
  static pid_t pid_;                                  \
  static std::mutex construct_lock;

#define SINGLETON_DEFINE(classname)            \
  classname *classname::__instance_ = nullptr; \
  pid_t classname::pid_ = -1;                  \
  std::mutex classname::construct_lock;
