#include "Macros.h"

#include <spdlog/spdlog.h>

namespace Vixen {
    void log_error(std::string function, std::string file, int line, std::string message) {
        spdlog::error("ERROR: {}\n   {} ({}:{})", message, function, file, line);
    }
}
