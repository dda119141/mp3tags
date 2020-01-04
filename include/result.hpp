/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TAGBASE_RESULT_HPP
#define TAGBASE_RESULT_HPP

#include <optional>
#include <variant>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
 
/*
 * Result is a type which represents value or an error, and values and errors
 * are template parametrized. Working with value wrapped in result is done using
 * match() function, which accepts 2 functions: for value and error cases. No
 * accessor functions are provided.
 */

namespace tagBase {

    /*
     * Value and error types can be constructed from any value or error, if
     * underlying types are constructible. Example:
     *
     * @code
     * Value<std::string> v = Value<const char *>("hello");
     * @nocode
     */

    template <typename T>
    struct Value {
      T value;
      template <typename V>
      operator Value<V>() {
        return {value};
      }
    };

    template <>
    struct Value<void> {};

    template <typename E>
    struct Error {
      E error;
      template <typename V>
      operator Error<V>() {
        return {error};
      }
    };

    template <>
    struct Error<void> {};

#if 0
    /**
     * Result is a specialization of a variant type with value or error
     * semantics.
     * @tparam V type of value
     * @tparam E error type
     */
    template <typename V, typename E>
    class Result: public std::variant<Value<V>, Error<E>> {

     public:
      using ValueType = Value<V>;
      using ErrorType = Error<E>;
    };
#endif
    template <typename V, typename E>
    using Result = std::variant<Value<V>, Error<E>>;

    template <typename ResultType>
    using ValueOf = typename ResultType::ValueType;
    template <typename ResultType>
    using ErrorOf = typename ResultType::ErrorType;

    // Factory methods for avoiding type specification
    template <typename T>
    Value<T> makeValue(T &&value) {
      return Value<T>{std::forward<T>(value)};
    }

    template <typename E>
    Error<E> makeError(E &&error) {
      return Error<E>{std::forward<E>(error)};
    }

    /**
     * Bind operator allows chaining several functions which return result. If
     * result contains error, it returns this error, if it contains value,
     * function f is called.
     * @param f function which return type must be compatible with original
     * result
     */
    template <typename T, typename E, typename Transform>
    constexpr auto operator|(Result<T, E> r, Transform &&f) ->
        typename std::enable_if<
            not std::is_same<decltype(f(std::declval<T>())), void>::value,
            decltype(f(std::declval<T>()))>::type {
      using return_type = decltype(f(std::declval<T>()));
      return std::visit(overloaded {
          [&f](const Value<T> &v) { return f(v.value); },
          [](const Error<E> &e) { return return_type(makeError(e.error)); } 
          }, r);
    }

    /**
     * Bind operator overload for functions which do not accept anything as a
     * parameter. Allows execution of a sequence of unrelated functions, given
     * that all of them return Result
     * @param f function which accepts no parameters and returns result
     */
    template <typename T, typename E, typename Procedure>
    constexpr auto operator|(Result<T, E> r, Procedure f) ->
        typename std::enable_if<not std::is_same<decltype(f()), void>::value,
                                decltype(f())>::type {
      using return_type = decltype(f());
      return std::visit(overloaded {
          [&f](const Value<T> &v) { return f(); },
          [](const Error<E> &e) { return return_type(makeError(e.error)); }
      }, r);
    }

}  // namespace tagBase
#endif  // TAGBASE_RESULT_HPP
