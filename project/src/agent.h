#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <map>
#include <string>

#include "event.h"
#include "event_list.h"
#include "os.h"
#include "agent_rules.h"

namespace nano_edr {


class Agent {
 public:
    Agent(std::size_t window_size, bool quiet);

    static void Trampoline(const os_event* ev, void* ctx) noexcept;

    void HandleEvent(const Event& event);

    void PrintSummary() const;

    void skip() {skipped_++;}

 private:
    void PrintContext() const;

    EventList window_;
    const EventNode* prev_ = nullptr;

    bool quiet_ = false;
    std::size_t skipped_ = 0;
    std::size_t total_ = 0;
    std::size_t detections_ = 0;
    std::map<std::string, std::size_t> by_type_;
    std::vector<MatchRule> rules_;

};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H