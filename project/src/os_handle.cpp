#include "os_handle.h"
#include <stdexcept>
#include <string>

namespace nano_edr {

namespace {
void ThrowIfFailed(const char* call, os_status status) {
    if (status != OS_OK) {
        throw std::runtime_error(std::string(call) + " завершилась с ошибкой: " +os_status_str(status));
    }
}
}

OsHandle::OsHandle(const std::string& config_path) {
    const os_status status = os_init(config_path.c_str(), &handle_);
    if (status != OS_OK) {
        throw std::runtime_error("не удалось инициализировать os.h для '" +
                                 config_path + "': " + os_status_str(status));
    }
}

OsHandle::~OsHandle() {
    os_free(handle_);
}

void OsHandle::Subscribe(os_event_cb callback, void* context) {
    ThrowIfFailed("os_event_subscribe", os_event_subscribe(handle_, callback, context));
}

void OsHandle::Start() {
    ThrowIfFailed("os_start", os_start(handle_));
}

os_status OsHandle::Wait(uint32_t timeout_ms) {
    return os_wait(handle_, timeout_ms);
}

void OsHandle::Stop() {
    static_cast<void>(os_stop(handle_));
}

}