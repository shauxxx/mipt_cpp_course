#include "os_source.h"
#include "event.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace nano_edr {

namespace {
constexpr std::uint32_t kWaitMs = 100;

Event ConvertOsEventToEvent(const os_event* ev) {
    EventParts parts;
    parts.ts = std::to_string(ev->ts);
    parts.type = ev->type != nullptr ? ev->type : "";
    parts.pid = ev->pid == 0 ? "" : std::to_string(ev->pid);
    for (std::size_t i = 0; i < ev->field_count; ++i) {
        const os_kv& kv = ev->fields[i];
        parts.fields.push_back(Field{kv.key != nullptr ? kv.key : "", kv.value != nullptr ? kv.value : ""});
        }
    const Event event(parts);
    return event;
}
} // namespace

OsSource::OsSource(const std::string& config_path) : handle_(config_path) {}

void OsSource::Run(IEventHandler* handler) {
    handle_.Subscribe(&OsSource::Trampoline, handler);
    handle_.Start();

    os_status status = handle_.Wait(kWaitMs);
    while (status == OS_TIMEOUT) {
        handler->OnTick();
        status = handle_.Wait(kWaitMs);
    }

    if (status != OS_OK) {
        throw std::runtime_error(std::string("источник остановился: ") +
                                 os_status_str(status));
    }
}

void OsSource::Trampoline(const os_event* ev, void* ctx) noexcept {
    if (ev == nullptr || ctx == nullptr) {
        return;
    }
    auto* self = static_cast<IEventHandler*>(ctx);
    try {
        const Event event = ConvertOsEventToEvent(ev);
        self->HandleEvent(event);    
    } catch (const std::exception& e) {
        std::fprintf(stderr, "событие ts=%llu пропущено: %s\n",
                     static_cast<unsigned long long>(ev->ts), e.what());
        self->OnBroken();
    } catch (...) {
        std::fprintf(stderr, "событие ts=%llu пропущено: неизвестная ошибка\n",
                     static_cast<unsigned long long>(ev->ts));
        self->OnBroken();
    }
}

}  // namespace nano_edr