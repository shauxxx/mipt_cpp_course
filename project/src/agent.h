#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <map>
#include <string>

#include "event.h"
#include "event_list.h"
#include "os.h"

namespace nano_edr {


class Agent {
 public:
    Agent(std::size_t window_size, bool quiet) : window_(window_size), quiet_(quiet) {};

    static void Trampoline(const os_event* ev, void* ctx) noexcept;

    void HandleEvent(const Event& event);

    void PrintSummary() const;

 private:
    void PrintContext() const;

    EventList window_;
    bool quiet_;
    std::size_t total_ = 0;
    std::map<std::string, std::size_t> by_type_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H