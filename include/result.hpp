// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef TAGBASE_RESULT_HPP
#define TAGBASE_RESULT_HPP

#include <functional>
#include <optional>
#include <type_traits>

template <typename T>
const auto getPayload(const std::string &filename)
{
  return [&filename](const meta_entry &entry) {
    const T handle{filename};
    return handle.getFramePayload(entry);
  };
}

namespace expected
{

using uChar = std::vector<unsigned char>;

template <typename T>
struct Result {
public:
  Result() noexcept = default;

  constexpr Result(T &&t) noexcept : mOptional{std::forward<T>(t)} {}

  void operator()(const std::string &error) { mError = error; }

  template <typename E,
            typename = std::enable_if_t<
                (std::is_same<std::decay_t<E>, std::string>::value ||
                 std::is_same<std::decay_t<E>, const char *>::value ||
                 std::is_same<std::decay_t<E>, char *>::value)>>
  void register_error(E &&error)
  {
    mError = std::forward<E>(error);
  }

  std::string error() const { return mError; }

  constexpr bool has_value() const noexcept { return mOptional.has_value(); }

  constexpr bool invalid() const noexcept { return !has_value(); }

  const auto value() const { return mOptional.value(); }

private:
  std::string mError = {};
  std::optional<T> mOptional = {};
};

template <typename T,
          typename = std::enable_if_t<(
              std::is_same<std::decay_t<T>, std::string>::value ||
              std::is_same<std::decay_t<T>, char *>::value)>,
          typename E = Result<T>>
struct print_t {
  auto operator<<(const E &obj)
  {
    std::stringstream ss;
    if (obj.has_value())
      ss << obj.value();
    else
      ss << "Empty Object";
  }
};

template <typename E, typename T,
          typename = std::enable_if_t<
              (std::is_same<std::decay_t<T>, std::string>::value ||
               std::is_same<std::decay_t<T>, const char *>::value)>>
Result<E> makeError(T &&error)
{
  auto obj = Result<E>{};
  obj.register_error(error);
  return obj;
}

template <typename T>
Result<T> makeValue(T value)
{
  return Result{std::forward<T>(value)};
}

template <typename T, typename Transform,
          typename Ret = typename std::result_of<Transform(T)>::type>
auto operator|(const expected::Result<T> &r, Transform f)
{
  auto fuc = std::forward<Transform>(f);

  if (r.has_value()) {
    return fuc(r.value());
  } else {
    return Ret();
  }
}

auto testResultClass(const std::string &FileName, uint32_t num)
{
  std::ifstream fil(FileName);
  std::vector<unsigned char> buffer(num, '0');

  if (!fil.good()) {
    return makeError<uChar>(std::string("mp3 UUU file could not be read"));
  }

  fil.read(reinterpret_cast<char *>(buffer.data()), num);

  return makeValue<uChar>(buffer);
}

} // namespace expected

#endif // TAGBASE_RESULT_HPP
