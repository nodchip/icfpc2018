#pragma once

#include <iostream>

#define LOG()         std::cerr << __FILE__ << "(" << __LINE__ << "): "
#define LOG_IF(cond)  if ((cond)) LOG() << #cond << " failed\n"

#define ASSERT_RETURN(cond, value)                      \
  do {                                                  \
    if (!(cond)) {                                      \
      LOG() << #cond << " failed\n";                    \
      return (value);                                   \
    }                                                   \
  } while(false)
#define ASSERT(cond)                                    \
  do {                                                  \
    if (!(cond)) {                                      \
      LOG() << #cond << " failed\n";                    \
      exit(0);                                          \
    }                                                   \
  } while(false)

#define ASSERT_COND(cond) do { LOG_IF(cond); } while(false)
#define ASSERT_NE(a, b)                             \
  do {                                              \
    LOG_IF((a) == (b))                              \
      << #a << ": " << (a) << "\n"                  \
      << #b << ": " << (b) << "\n";                 \
  } while(false)
#define ASSERT_EQ(a, b)                             \
  do {                                              \
    LOG_IF((a) != (b))                              \
      << #a << ": " << (a) << "\n"                  \
      << #b << ": " << (b) << "\n";                 \
  } while(false)
#undef ASSERT_COND
