#include "agent.h"

#include <cstdio>
#include <exception>
#include <print>
#include <string>

#include "agent_rules.h"
#include "dedup_sink.h"
#include "rule.h"
#include "rules.h"

namespace nano_edr {

Agent::Agent(bool quiet, uint64_t dedup_period_ms) : quiet_(quiet), engine_(BuildAgentRules(&model_)), dedup_period_ms_(dedup_period_ms) { engine_.SetModel(&model_); }

void Agent::HandleEvent(const Event& event) {
    model_.Observe(event);
    std::vector<const IRule*> fired;
    ctxprint_ = false;
    engine_.ProcessEvent(event, &fired);

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
    std::print("[STAT] в модели процессов {}, файлов {}\n", model_.process_count(), model_.file_count());
    if (suppressed_ != 0) {
        std::print("[STAT] подавлено дубликатов {} (срок {} мс)\n", suppressed_, dedup_period_ms_);
    }
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

void Agent::Subscribe(AlertSink sink) {
    FanOutSink all;
    all.Add([this](const Detection&) { PrintContextOnce(); });
    all.Add(std::move(sink));
    engine_.Subscribe(DedupSink(std::move(all), dedup_period_ms_, &suppressed_));
}

void Agent::PrintContextOnce() {
    if (!ctxprint_ && !quiet_) {
        PrintContext();
        ctxprint_ = true;
    }
}
}  // namespace nano_edr