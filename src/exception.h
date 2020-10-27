#pragma once

#include <string>
#include <stdexcept>

namespace ql {

class exception : public std::runtime_error {
public:
    explicit exception(
        const std::string &msg,
        bool system = false
    ) noexcept;
};

} // namespace ql
