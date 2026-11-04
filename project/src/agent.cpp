#include "agent.h"

#include <cstdio>
#include <exception>
#include <print>
#include <string>

#include "agent_rules.h"
#include "rule.h"
#include "rules.h"

namespace nano_edr {

Agent::Agent(bool quiet) : quiet_(quiet), engine_(BuildAgentRules()) {}

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
    window_.PushBack({event.ts(), event.type(), event.pid()});
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
    std::print("не разобрано: {}\n", broken_);
    for (std::size_t i = 0; i < engine_.size(); ++i) {
        std::print("  {}  {}\n", engine_.rule(i).id(), engine_.rule(i).hits());
    }
    std::print("[STAT] в окне {} из {} мест\n", window_.size(), window_.capacity());
}

void Agent::PrintContext() const {
    if (window_.size() >= 2) {
        std::print("[CTX] -2: {} {} pid={}\n", window_[window_.size() - 2].ts.ms, window_[window_.size() - 2].type, window_[window_.size() - 2].pid);
    }
    if (window_.size() >= 1) {
        std::print("[CTX] -1: {} {} pid={}\n", window_.back().ts.ms, window_.back().type, window_.back().pid);
    }
}

void Agent::OnSkipped() {
    ++skipped_;
}

void Agent::OnBroken() {
    ++broken_;
}

}  // namespace nano_edr