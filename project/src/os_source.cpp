#include "os_source.h"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace nano_edr {

namespace {
constexpr std::uint32_t kWaitMs = 100;
}

OsSource::OsSource(const std::string& config_path) : handle_(config_path) {}

void OsSource::Run(Agent* agent) {
    handle_.Subscribe(&Agent::Trampoline, agent);
    handle_.Start();

    os_status status = handle_.Wait(kWaitMs);
    while (status == OS_TIMEOUT) {
        status = handle_.Wait(kWaitMs);
    }

    if (status != OS_OK) {
        throw std::runtime_error(std::string("источник остановился: ") +
                                 os_status_str(status));
    }
}

}  // namespace nano_edr