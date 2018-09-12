#pragma once

#include <array>
#include <string>
#include <utility>

template<typename T>
class OnScopeEnd
{
public:
    [[gnu::always_inline]]
    OnScopeEnd(T func) : m_armed(true), m_func(std::move(func)) {}

    void disarm() { m_armed = false; }

    [[gnu::always_inline]]
    ~OnScopeEnd() { if (m_armed) m_func(); }
private:
    bool m_armed;
    T m_func;
};

template<typename T>
OnScopeEnd<T> on_scope_end(T t)
{
    return OnScopeEnd<T>{std::move(t)};
}

template <typename V, typename... T>
constexpr auto make_array(T&&... t) -> std::array <V, sizeof...(T)>
{
    return {{ std::forward<T>(t)... }};
}

namespace detail
{
    struct StringView {
        StringView(const char& s) : data(&s), length(1) {}
        StringView(const char* s) : data(s), length(std::strlen(s)) {}
        StringView(const char* begin, const char* end) : data(begin), length(end-begin) {}
        StringView(const std::string& s) : data(s.data()), length(s.size()) {}

        const char* begin() { return data; }
        const char* end() { return data + length; }

        const char* data;
        const size_t length;
    };

    template<typename T>
    std::string format_param(const T& val) { return std::to_string(val); }

    inline const char& format_param(const char& val) { return val; }
    inline const std::string& format_param(const std::string& val) { return val; }
    inline const char* format_param(const char* val) { return val; }

    inline std::string format_impl(StringView fmt, std::initializer_list<StringView> params);
}

template<typename... Types>
inline std::string format(detail::StringView fmt, Types&&... params)
{
    return detail::format_impl(fmt, { detail::format_param(std::forward<Types>(params))... });
}

