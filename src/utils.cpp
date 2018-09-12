#include "utils.hpp"

namespace detail {
    std::string format_impl(StringView fmt, std::initializer_list<StringView> params)
    {
        // approximate size needed
        size_t size = fmt.length;
        for (auto& s : params)
            size += s.length;
        std::string res;
        res.reserve(size);

        size_t implicitIndex = 0;
        for (auto it = fmt.begin(), end = fmt.end(); it != end;) {
            auto current = it;
            while (true) {
                if (current == end) {
                    res.append(it, current);
                    break;
                } else if (*current == '\\') {
                    res.append(it, current);
                    ++current; // points to the escaped char
                    if (current == end) 
                        break;
                    res += (*current);
                    ++current; // points to the rest
                    break;
                } else if (*current == '{') {
                    res.append(it, current);
                    auto closing = std::find(current, end, '}');
                    if (closing == end)
                        throw std::runtime_error("format string error, unclosed '{'");

                    size_t index;
                    if (closing == current + 1) 
                        index = implicitIndex++;
                    else 
                        index = std::stoi(std::string{current + 1, closing});

                    if (index >= params.size())
                        throw std::runtime_error("format string parameter index too big");

                    const auto& param = params.begin()[index];
                    res.append(param.data, param.length);
                    current = closing + 1; // points to the rest
                    break;
                }
                ++current;
            }
            it = current;
        }
        return res;
    }
}
