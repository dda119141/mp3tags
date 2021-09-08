// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef TAGBASE_RESULT_HPP
#define TAGBASE_RESULT_HPP

#include <type_traits>
#include <functional>
#include <optional>

namespace expected {

    using uChar = std::vector<unsigned char>;
    template <typename T, typename E>
        class Result_t {
            public:
                Result_t()
                {
                }

                auto moveError(const E& error) {
                    //m_error = std::move(E(std::forward<E>(error)));
                    m_error = std::move(error);
                    m_value = {};

                    return *this;
                }

                auto moveValue(const T& value) {
                    m_value = std::move(value);
                    m_error = {};

                    return *this;
                }

                Result_t<T, E> operator()(const E& error)
                {
                    m_error = std::move(error);
                    m_value = {};
                    return *this;
                }

               bool has_value() const { return m_value.has_value(); }

                bool has_error() const { return m_error.has_value(); }

                T value() const {
                    if (m_value.has_value())
                        return m_value.value();
                    else
                        return {};
                }

                E error() const {
                    if (m_error.has_value())
                        return m_error.value();
                    else
                        return {};
                }

                template <typename V>
                auto operator << (const V& error)
                {
                    std::stringstream ss;
                    ss << error;
                    if(m_error.has_value())
                         m_error = m_error.value() +  ss.str();
                    else
                        m_error = ss.str();
                    return *this;
                }

            private:
                std::optional<E> m_error;
                std::optional<T> m_value;
        };

    template <typename T>
        using Result = Result_t<T, std::string>;

 template <typename T, typename E>
        Result_t<T, E> makeError(const E& error) {
            Result_t<T, E> obj;
            obj.moveError(error);
            return obj;
        }

  template <typename T>
        Result<T> makeError(const char* error) {
            Result_t<T, std::string> obj;
            obj.moveError(std::string(error));
            return obj;
        }

  template <typename T>
        Result<T> makeError() {
            Result_t<T, std::string> obj;
            obj.moveError(std::string("Error: "));
            return obj;
        }

    template <typename T>
        auto makeError(std::string_view error)
        -> decltype(makeError<T, std::string_view>(std::forward<std::string_view>(error)))
        {
            return makeError<T, std::string_view>(std::forward<std::string_view>(error));
        }

    template <typename T, typename E>
        Result_t<T, E> makeValue(T&& value) {
            Result_t<T, E> obj;
            obj.moveValue(std::forward<T>(value));
            return obj;
        }

  template <typename T>
        Result<T> makeValue(const T& value) {
            Result_t<T, std::string> obj;
            obj.moveValue(value);
            return obj;
        }


    template <typename T, typename E>
        std::string getOutput(const Result_t<T, E>& obj)
        {
            std::stringstream str_obj;
            if(auto ret1 = std::get_if<T>(&obj)){
                str_obj << *ret1;
            }
            else if(auto ret2 = std::get_if<E>(&obj)){
                str_obj << *ret2;
            }

            str_obj << "\n";

            return str_obj.str();
        }

#if 0
    template <typename T, typename E, typename Transform,
            typename Ret = typename std::result_of<Transform(T)>::type>
        Ret operator | (expected::Result_t<T, E>&& r, Transform f)
       // -> decltype(f(std::forward<expected::Result_t<T, E>>(r).value()))
    {
            auto fuc = std::forward<Transform>(f);
            auto obj = std::forward<expected::Result_t<T, E> >(r);

            if (obj.has_value()) {
                return fuc(obj.value());
            } else {
                return Ret()(obj.error());
               // return Ret();
            }
    }
#endif

    template <typename T, typename E, typename Transform,
            typename Ret = typename std::result_of<Transform(T)>::type>
        auto operator | (const expected::Result_t<T, E>& r, Transform f)
            -> typename std::enable_if_t<not std::is_integral<Ret>::value && not std::is_void<Ret>::value, Ret>
    {
            auto fuc = std::forward<Transform>(f);

            if (r.has_value()) {
                return fuc(r.value());
            } else {
               // return Ret()(r.error());
                return Ret();
            }
     }

 template <typename T, typename E, typename Transform,
            typename Ret = typename std::result_of<Transform(T)>::type>
        auto operator | (const expected::Result_t<T, E>& r, Transform f)
            -> typename std::enable_if_t<std::is_integral<Ret>::value, Ret>
    {
            auto fuc = std::forward<Transform>(f);

            if (r.has_value()) {
                return fuc(r.value());
            } else {
                return 0;
            }
     }

 template <typename T, typename E, typename Transform,
            typename Ret = typename std::result_of<Transform(T)>::type>
        auto operator | (const expected::Result_t<T, E>& r, Transform f)
            -> typename std::enable_if_t<std::is_void<Ret>::value, Ret>
    {
            auto fuc = std::forward<Transform>(f);

            if (r.has_value()) {
                return fuc(r.value());
            } else {
                return;
            }
     }

}  // namespace expected


namespace expected
{
    using uChar = std::vector<unsigned char>;

    Result<uChar> GetStringFromFile1(const std::string& FileName, uint32_t num )
    {
        std::ifstream fil(FileName);
        std::vector<unsigned char> buffer(num, '0');

        if(!fil.good()){
            //std::cerr << "mp3 file could not be read\n";
            //return {};
            return makeError<uChar, std::string>("mp3 UUU file could not be read");
        }

        fil.read(reinterpret_cast<char*>(buffer.data()), num);

        //return makeValue<uChar, std::string>(buffer);
        return makeValue<uChar>(buffer);
    }

    std::optional<uChar> GetTagHeader1(const std::string& FileName )
//    std::string GetTagHeader1(const std::string& FileName )
    {
        auto ret =
            GetStringFromFile1(FileName, 10) | [&]( const uChar& obj )
        {
            std::cerr << "error " << "GetTagHeader1" << std::endl;

            //return makeValue<bool, std::string>(true);
           // return makeError<bool, std::string>(" ******* ");
            //return makeError<uChar>(" ******* ");
            return makeValue<uChar>(obj);
            //return true;
        };

       return std::make_optional(ret.value());
        //return ret.error();
       // return "OK";
    }
}

#endif  // TAGBASE_RESULT_HPP
