#ifndef MY_MUDUO_DEFINE
#define MY_MUDUO_DEFINE

#include <cstddef>

constexpr int max_listen_fd = 8;
constexpr size_t signal_length = 8;
constexpr size_t buffer_prepend_size = 8;
constexpr size_t buffer_initial_size = 128;
constexpr size_t extra_buffer_size = 65536;  // the same with the kernel buffer
constexpr int max_events = 512;

#endif