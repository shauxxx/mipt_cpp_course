#include "rule.h"

namespace nano_edr {
RuleBase::RuleBase(const std::string& id, Severity severity) : id_(id), severity_(severity) {}

void RuleBase::AddAction(const std::string& action) {
    actions_.push_back(action);
}

MatchRule::MatchRule(const std::string& id, Severity severity) : RuleBase(id, severity) {}

void MatchRule::AddCondition(std::unique_ptr<ICondition> condition) {
    conditions_.push_back(std::move(condition));
}

bool MatchRule::Check(const Event& event) {
    for (const auto& condition : conditions_) {
        if (!condition->Matches(event)) {
            return false;
        }
    }
    CountHit();
    return true;
}

SequenceRule::SequenceRule(const std::string& id, Severity severity,
                        uint64_t window_ms)
    : RuleBase(id, severity), window_ms_(window_ms) {}

void SequenceRule::SetFirst(std::unique_ptr<ICondition> condition) {
    first_ = std::move(condition);
}

void SequenceRule::SetSecond(std::unique_ptr<ICondition> condition) {
    second_ = std::move(condition);
}

bool SequenceRule::Check(const Event& event) {
    if (!first_ || !second_) {
        return false;
    }
    for (size_t i = 0; i < pending();) {
        if (pending_[i].ts_ms + window_ms_ < event.ts().ms) {
            pending_.erase(pending_.begin());
        } else {
            ++i;
        }
    }
    if ((*second_).Matches(event)) {
        if (!same_pid_ && pending()) {
            pending_.erase(pending_.begin());
            CountHit();
            return true;
        } else {
            for (size_t i = 0; i < pending(); ++i) {
                if (pending_[i].pid == event.pid()) {
                    pending_.erase(pending_.begin() + i);
                    CountHit();
                    return true;
                }
            }
        }
    }
    if ((*first_).Matches(event)) {
        pending_.push_back(Pending(event.ts().ms, event.pid()));
    }
    return false;
}
} // namespace nano_edr