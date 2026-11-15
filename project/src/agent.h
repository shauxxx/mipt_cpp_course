#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <map>
#include <string>

#include "agent_rules.h"
#include "event.h"
#include "event_source.h"
#include "ring_buffer.h"
#include "rule_engine.h"

namespace nano_edr {

struct WindowEntry {
    Timestamp ts;
    std::string type;
    std::string pid;
};

class Agent : public IEventHandler {
 public:
    static constexpr std::size_t kWindowCapacity = 256;

    Agent(bool quiet, uint64_t dedup_period_ms);

    void PrintSummary() const;

    void HandleEvent(const Event& event) override;

    void OnSkipped() override;

    void OnBroken() override;

    void OnTick() override {};

    void Subscribe(AlertSink sink);

 private:
    void PrintContext() const;

    void PrintContextOnce();

    RingBuffer<WindowEntry, kWindowCapacity> window_;

    bool quiet_ = false;
    std::size_t skipped_ = 0;
    std::size_t broken_ = 0;

    std::size_t total_ = 0;
    std::size_t detections_ = 0;
    std::map<std::string, std::size_t> by_type_;

    EntityModel model_;
    RuleEngine engine_;

    bool ctxprint_ = false;

    uint64_t dedup_period_ms_{};
    std::size_t suppressed_{};
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H