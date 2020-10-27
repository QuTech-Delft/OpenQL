#include "exception.h"

#include <cstring>

namespace ql {

static std::string make_message(
    const std::string &msg,
    bool system = false
) noexcept {
    if (system) {
        return msg + ": " + std::strerror(errno);
    } else {
        return msg;
    }
}

exception::exception(
    const std::string &msg,
    bool system
) noexcept : std::runtime_error(make_message(msg, system)) {
}

} // namespace ql
