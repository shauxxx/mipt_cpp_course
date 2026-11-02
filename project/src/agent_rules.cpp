#include "agent_rules.h"
#include "fields.h"
#include "rules.h"
#include "rule.h"
#include "conditions.h"


#include <string>

namespace nano_edr {

namespace {

std::unique_ptr<MatchRule> MakeScriptHostFromTemp() {
    auto rule = std::make_unique<MatchRule>("script_host_from_temp", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));

    auto image = std::make_unique<AnyOf>();
    image->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    image->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    rule->AddCondition(std::move(image));

    auto cmd = std::make_unique<AnyOf>();
    cmd->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));
    rule->AddCondition(std::move(cmd));

    rule->AddAction("kill_process");

    return rule;
}

std::unique_ptr<MatchRule> MakeLolbinDownload() {
    auto rule = std::make_unique<MatchRule>("lolbin_download", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));

    auto image = std::make_unique<AnyOf>();
    image->Add(std::make_unique<FieldEndsWith>("image", "certutil.exe"));
    image->Add(std::make_unique<FieldEndsWith>("image", "bitsadmin.exe"));
    rule->AddCondition(std::move(image));

    auto cmd = std::make_unique<AnyOf>();
    cmd->Add(std::make_unique<FieldContains>("cmdline", "urlcache"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "transfer"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "http:"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "https:"));
    rule->AddCondition(std::move(cmd));

    rule->AddAction("kill_process");

    return rule;
}

std::unique_ptr<MatchRule> MakeHiddenPowershell() {
    auto rule = std::make_unique<MatchRule>("hidden_powershell", Severity::kMedium);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));

    auto image = std::make_unique<AnyOf>();
    image->Add(std::make_unique<FieldEndsWith>("image", "powershell.exe"));
    image->Add(std::make_unique<FieldEndsWith>("image", "pwsh.exe"));
    rule->AddCondition(std::move(image));

    auto cmd = std::make_unique<AnyOf>();
    cmd->Add(std::make_unique<FieldContains>("cmdline", "-w hidden"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "-windowstyle hidden"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "-enc"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "-encodedcommand"));
    rule->AddCondition(std::move(cmd));

    rule->AddAction("kill_process");

    return rule;
}

std::unique_ptr<MatchRule> MakeAutostartWrite() {
    auto rule = std::make_unique<MatchRule>("autostart_write", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite, EventType::kFileMove}));

    auto paths = std::make_unique<AnyOf>();
    paths->Add(std::make_unique<FieldContains>("path", "\\start menu\\programs\\startup\\"));
    paths->Add(std::make_unique<FieldContains>("path", "/start menu/programs/startup/"));
    paths->Add(std::make_unique<FieldContains>("to", "\\start menu\\programs\\startup\\"));
    paths->Add(std::make_unique<FieldContains>("to", "/start menu/programs/startup/"));
    rule->AddCondition(std::move(paths));

    rule->AddAction("quarantine_file");

    return rule;
}

std::unique_ptr<MatchRule> MakeRansomExtension() {
    auto rule = std::make_unique<MatchRule>("ransom_extension", Severity::kCritical);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite, EventType::kFileMove}));

    auto paths = std::make_unique<AnyOf>();
    paths->Add(std::make_unique<FieldEndsWith>("path", ".locked"));
    paths->Add(std::make_unique<FieldEndsWith>("to", ".locked"));
    rule->AddCondition(std::move(paths));

    rule->AddAction("quarantine_file");

    return rule;
}

std::unique_ptr<SequenceRule> MakeScriptHostFromBeacon() {
    auto beacon = std::make_unique<SequenceRule>("script_host_beacon", Severity::kCritical, 10000);

    auto first = std::make_unique<AllOf>();
    first->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));

    auto first_image = std::make_unique<AnyOf>();
    first_image->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    first_image->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));

    auto first_cmd = std::make_unique<AnyOf>();
    first_cmd->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
    first_cmd->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
    first_cmd->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
    first_cmd->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
    first_cmd->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
    first_cmd->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));

    first->Add(std::move(first_image));
    first->Add(std::move(first_cmd));

    beacon->SetFirst(std::move(first));
    beacon->SetSecond(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kNetConnect}));
    beacon->AddAction("kill_process");

    return beacon;
}

}  // namespace

RuleEngine BuildAgentRules() {
    RuleEngine engine;
    engine.AddRule(MakeScriptHostFromTemp());
    engine.AddRule(MakeLolbinDownload());
    engine.AddRule(MakeHiddenPowershell());
    engine.AddRule(MakeAutostartWrite());
    engine.AddRule(MakeRansomExtension());
    engine.AddRule(MakeScriptHostFromBeacon());
    return engine;
}

}  // namespace nano_edr