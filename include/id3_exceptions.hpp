// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3_EXCEPTION_HPP
#define ID3_EXCEPTION_HPP

#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

// Define if the library is going to be using exceptions.
#if (!defined(__cpp_exceptions) && !defined(__EXCEPTIONS) &&                   \
     !defined(_CPPUNWIND))
#undef ID3_DISABLE_EXCEPTIONS
#define ID3_DISABLE_EXCEPTIONS
#endif

// Exception support.
#if defined(ID3_DISABLE_EXCEPTIONS)
#include <iostream>
#define ID3_THROW(_, msg)                                                      \
  {                                                                            \
    std::cerr << msg << std::endl;                                             \
    std::abort();                                                              \
  }
#else
#define ID3_THROW(exception, msg) throw exception(msg)
#endif

namespace id3 {

class audio_tag_error : public std::runtime_error {
public:
  explicit audio_tag_error(const std::string &msg)
      : std::runtime_error(msg.c_str()) {}
};

class id3_error : public audio_tag_error {
public:
  explicit id3_error(const std::string &msg) : audio_tag_error(msg.c_str()) {}
};

class ape_error : public audio_tag_error {
public:
  explicit ape_error(const std::string &msg) : audio_tag_error(msg.c_str()) {}
};

#define ID3V2_THROW(msg) ID3_THROW(id3_error, std::string("id3v2 ") + msg);

#define APE_THROW(msg) ID3_THROW(ape_error, std::string("ape ") + msg);

#ifdef ID3_ENABLE_ASSERT
#define ID3_ASSERT_MSG(expr, msg)                                              \
  if (!(expr)) {                                                               \
    ID3_THROW(std::runtime_error, std::string("Assertion error!\n") + msg +    \
                                      "\n  " + __FILE__ + '(' +                \
                                      std::to_string(__LINE__) + ")\n");       \
  }
#else
#define ID3_ASSERT_MSG(expr, msg)
#endif

#define ID3_PRECONDITION_MSG(expr, msg)                                        \
  if (!(expr)) {                                                               \
    ID3_THROW(id3_error, std::string("Precondition violation!\n") + msg +      \
                             "\n  " + __FILE__ + '(' +                         \
                             std::to_string(__LINE__) + ")\n");                \
  }

#define ID3_PRECONDITION(expr)                                                 \
  if (!(expr)) {                                                               \
    ID3_THROW(id3_error, std::string("Precondition violation!\n") + __FILE__ + \
                             '(' + std::to_string(__LINE__) + ")\n");          \
  }

} // namespace id3
#endif //
