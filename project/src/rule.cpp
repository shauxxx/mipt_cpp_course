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
            pending_.erase(pending_.begin() + i);
        } else {
            ++i;
        }
    }
    if ((*second_).Matches(event)) {
        for (size_t i = 0; i < pending(); ++i) {
            if (Linked(pending_[i], event)) {
                pending_.erase(pending_.begin() + i);
                CountHit();
                return true;
            }
        }
    }
    if ((*first_).Matches(event)) {
        pending_.push_back(Pending(event.ts().ms, event.pid()));
    }
    return false;
}

LambdaRule::LambdaRule(const std::string& id, Severity severity, Predicate predicate)
    : RuleBase(id, severity), predicate_(std::move(predicate)) {}

bool LambdaRule::Check(const Event& event) {
    if (!predicate_) {
        return false;
    }
    if (predicate_(event)) {
        CountHit();
        return true;
    }
    return false;
}

bool SequenceRule::Linked(const Pending& first, const Event& second) const {
    if (child_of_prev_) {
        if (!model_) {
            return false;
        }
        const ProcessRecord* process = model_->FindProcess(second.pid());
        if (!process) {
            return false;
        }
        return process->ppid == first.pid;
    } else if (same_pid_) {
        return first.pid == second.pid();
    }
    return true;
}
}  // namespace nano_edr