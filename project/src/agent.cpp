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
    if (responder_ != nullptr && responder_->Recognize(event)) {
        WindowEntry entry;
        entry.ts = event.ts();
        entry.type = event.type();
        entry.pid = event.pid();
        window_.PushBack(std::move(entry));
        return;
    }
    std::vector<const IRule*> fired;
    ctxprint_ = false;
    engine_.ProcessEvent(event, &fired);

    detections_ += fired.size();
    ++total_;
    ++by_type_[event.type()];
    WindowEntry entry;
    entry.ts = event.ts();
    entry.type = event.type();
    entry.pid = event.pid();
    window_.PushBack(std::move(entry));
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
    if (responder_) {
        std::print("[STAT] действий запланировано {}, повторов отброшено {}\n", responder_->stats().planned, responder_->stats().skipped_repeat);
        std::print("[STAT] завершено процессов {}, изолировано файлов {}, удалено файлов {}\n", responder_->stats().killed, responder_->stats().quarantined, responder_->stats().deleted);
        if (responder_->stats().own_events) {
            std::print("[STAT] своих событий {}\n", responder_->stats().own_events);
        }
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

void Agent::EnableResponse(OsHandle* os, bool dry_run, const std::string& quarantine_dir) {
    planner_ = std::make_unique<ResponsePlanner>(&model_, quarantine_dir);
    responder_ = std::make_unique<Responder>(os, dry_run);
    engine_.Subscribe([this](const Detection& detection) {
        responder_->Execute(planner_->Plan(detection));
    });
}

}  // namespace nano_edr