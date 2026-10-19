#include "agent.h"

#include <cstdio>
#include <exception>
#include <print>
#include <string>

#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {

void Agent::HandleEvent(const Event& event) {
    const std::size_t detected = CheckRules(event, AgentRules(), AgentRuleCount());

    if (detected > 0 && !quiet_) {
        PrintContext();
    }

    window_.PushBack(event);
    ++total_;
    ++by_type_[event.type()];
}

void Agent::Trampoline(const os_event* ev, void* ctx) noexcept {
    if (ev == nullptr || ctx == nullptr) {
        return;
    }
    try {
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
    } catch (const std::exception& e) {
        std::fprintf(stderr, "событие ts=%llu пропущено: %s\n",
                     static_cast<unsigned long long>(ev->ts), e.what());
    } catch (...) {
        std::fprintf(stderr, "событие ts=%llu пропущено: неизвестная ошибка\n",
                     static_cast<unsigned long long>(ev->ts));
    }
    }

void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("событий всего : {}\n", total_);
    for (const auto& [type, count] : by_type_) {
        std::print("событий типа {} всего : {}\n", type, count);
    }
}

void Agent::PrintContext() const {
    const std::size_t total = window_.size();
    std::size_t k = 0;
    for (const EventNode* node = window_.head(); node != nullptr;
         node = node->next, ++k) {
        if (k + 2 >= total) {
            std::print("[CTX] -{}: ts={} type={} pid={}\n", total - k,
                       node->event.raw_ts(), node->event.type(),
                       node->event.pid());
        }
    }
}

}  // namespace nano_edr