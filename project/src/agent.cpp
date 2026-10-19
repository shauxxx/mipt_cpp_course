#include "agent.h"

#include <cstdio>
#include <exception>
#include <print>
#include <string>

#include "agent_rules.h"
#include "rules.h"
#include "event_list.h"

namespace nano_edr {

namespace {

    void Transition(const os_event* ev, void* ctx) {
        EventParts parts;
        parts.ts = std::to_string(ev->ts);
        parts.type = ev->type != nullptr ? ev->type : "";
        parts.pid = ev->pid == 0 ? "" : std::to_string(ev->pid);
        for (std::size_t i = 0; i < ev->field_count; ++i) {
            const os_kv& kv = ev->fields[i];
            parts.fields.push_back(Field{kv.key != nullptr ? kv.key : "",
                                         kv.value != nullptr ? kv.value : ""});
            }
        const Event event(parts);
        static_cast<Agent*>(ctx)->HandleEvent(event); 
        }

}
Agent::Agent(std::size_t window_size, bool quiet) : window_(window_size), quiet_(quiet), rules_(BuildAgentRules()) {}

void Agent::HandleEvent(const Event& event) {
    bool isfind = false;
    for (std::size_t i = 0; i < rules_.size(); ++i) {
        if (rules_[i].Check(event)) {
            if (!quiet_ && !isfind) {
                PrintContext();
                isfind = true;
            }
            std::print("[DETECT] {}  {}  ts={} pid={}\n", SeverityName(rules_[i].severity()), rules_[i].id(), event.raw_ts(), event.pid());
            ++detections_;
        }

    }

    window_.PushBack(event);
    ++total_;
    ++by_type_[event.type()];
    const EventNode* old_tail = window_.back();
    prev_ = (window_.size() >= 2) ? old_tail : nullptr;
}

void Agent::Trampoline(const os_event* ev, void* ctx) noexcept {
    if (ev == nullptr || ctx == nullptr) {
        return;
    }
    auto* self = static_cast<Agent*>(ctx);
    try {
        Transition(ev, ctx);   
    } catch (const std::exception& e) {
        std::fprintf(stderr, "событие ts=%llu пропущено: %s\n",
                     static_cast<unsigned long long>(ev->ts), e.what());
        ++(self->skipped_);
    } catch (...) {
        std::fprintf(stderr, "событие ts=%llu пропущено: неизвестная ошибка\n",
                     static_cast<unsigned long long>(ev->ts));
        ++(self->skipped_);
    }
    }

void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("событий всего : {}\n", total_);
    for (const auto& [type, count] : by_type_) {
        std::print("событий типа {} всего : {}\n", type, count);
    std::print("детектов: {}\n", detections_);
    std::print("пропущено: {}\n", skipped_);
    }
    for (const MatchRule& rule : rules_) {
        std::print("  {}  {}\n", rule.id(), rule.hits());
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

}  // namespace nano_edr