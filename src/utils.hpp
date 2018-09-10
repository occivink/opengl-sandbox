#pragma once

#include <array>
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
