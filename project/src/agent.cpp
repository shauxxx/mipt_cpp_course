#include "agent.h"

#include <cstdio>
#include <exception>
#include <print>
#include <string>

#include "agent_rules.h"
#include "rules.h"
#include "event_list.h"
#include "rule.h"

namespace nano_edr {

Agent::Agent(std::size_t window_size, bool quiet) : window_(window_size), quiet_(quiet), engine_(BuildAgentRules()) {}

void Agent::HandleEvent(const Event& event) {
    std::vector<const IRule*> fired;
    engine_.ProcessEvent(event, &fired);
    if (!quiet_ && !fired.empty()) {
        PrintContext();
    }
    for (std::size_t i = 0; i < fired.size(); ++i) {
        std::print("[DETECT] {}  {}  ts={} pid={}\n", SeverityName(fired[i]->severity()), fired[i]->id(), event.raw_ts(), event.pid());
    }

    detections_ += fired.size();
    ++total_;
    ++by_type_[event.type()];
    const EventNode* old_tail = window_.back();
    prev_ = (window_.capacity() >= 2) ? old_tail : nullptr;
    window_.PushBack(event);
}

void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("событий всего : {}\n", total_);
    for (const auto& [type, count] : by_type_) {
        std::print("событий типа {} всего : {}\n", type, count);
    }
    std::print("детектов: {}\n", detections_);
    std::print("пропущено: {}\n", skipped_);
    for (std::size_t i = 0; i < engine_.size(); ++i) {
        std::print("  {}  {}\n", engine_.rule(i).id(), engine_.rule(i).hits());
    }

}

void Agent::PrintContext() const {
    if (prev_ != nullptr) {
        std::print("[CTX] -2: ts={} type={} pid={}\n", prev_->event.raw_ts(), prev_->event.type(), prev_->event.pid());
    }
    const EventNode* last = window_.back();
    if (last != nullptr) {
        std::print("[CTX] -1: ts={} type={} pid={}\n", last->event.raw_ts(), last->event.type(), last->event.pid());
    }
} 

void Agent::OnSkipped() {
    ++skipped_;
}

void Agent::OnBroken() {
    ++broken_;
}

}  // namespace nano_edr