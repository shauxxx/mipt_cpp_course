#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <map>
#include <string>

#include "event.h"
#include "event_list.h"
#include "agent_rules.h"
#include "rule_engine.h"
#include "event_source.h"

namespace nano_edr {


class Agent : public IEventHandler {
 public:
    Agent(std::size_t window_size, bool quiet);

    void PrintSummary() const;

    void skip() {skipped_++;}

    void HandleEvent(const Event& event) override;

    void OnSkipped() override;

    void OnBroken() override;

    void OnTick() override {};

 private:
    void PrintContext() const;

    EventList window_;
    const EventNode* prev_ = nullptr;

    bool quiet_ = false;
    std::size_t skipped_ = 0;
    std::size_t broken_ = 0;

    std::size_t total_ = 0;
    std::size_t detections_ = 0;
    std::map<std::string, std::size_t> by_type_;

    RuleEngine engine_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H