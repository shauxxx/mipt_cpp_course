#include "agent_rules.h"

#include <string>

#include "combinators.h"
#include "conditions.h"
#include "entity_model.h"
#include "fields.h"
#include "make_rule.h"
#include "rule.h"
#include "rules.h"

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
    rule->AddAction("quarantine_source");
    rule->AddAction("delete_dropped");

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

    rule->AddAction("delete_evidence");

    return rule;
}

std::unique_ptr<MatchRule> MakeRansomExtension() {
    auto rule = std::make_unique<MatchRule>("ransom_extension", Severity::kCritical);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite, EventType::kFileMove}));

    auto paths = std::make_unique<AnyOf>();
    paths->Add(std::make_unique<FieldEndsWith>("path", ".locked"));
    paths->Add(std::make_unique<FieldEndsWith>("to", ".locked"));
    rule->AddCondition(std::move(paths));

    rule->AddAction("kill_process");
    rule->AddAction("quarantine_image");

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

std::unique_ptr<ThresholdRule<128>> MakeMassWrite() {
    auto mass_write = MakeRule<ThresholdRule<128>>(
        "ransomware_mass_write", Severity::kCritical,
        20,
        10000,
        "pid");

    mass_write->SetCondition(MakeCondition(AllOfStatic(
        EventTypeIs({EventType::kFileCreate, EventType::kFileWrite,
                     EventType::kFileMove}),
        AnyOfStatic(FieldContains("path", "\\Documents\\"),
                    FieldContains("path", "\\Desktop\\"),
                    FieldContains("to", "\\Documents\\"),
                    FieldContains("to", "\\Desktop\\")))));

    mass_write->AddAction("kill_process");
    mass_write->AddAction("quarantine_image");
    return mass_write;
}

std::unique_ptr<SequenceRule> MakeShellSpawnsScriptHost(const EntityModel* model) {
    auto shell = MakeRule<SequenceRule>("shell_spawns_script_host",
                                        Severity::kHigh, 30000);

    auto first = std::make_unique<AllOf>();
    first->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));

    auto first_image = std::make_unique<AnyOf>();
    first_image->Add(std::make_unique<FieldEndsWith>("image", "cmd.exe"));
    first_image->Add(std::make_unique<FieldEndsWith>("image", "powershell.exe"));
    first_image->Add(std::make_unique<FieldEndsWith>("image", "pwsh.exe"));

    first->Add(std::move(first_image));
    shell->SetFirst(std::move(first));

    auto second = std::make_unique<AllOf>();
    second->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));

    auto second_image = std::make_unique<AnyOf>();
    second_image->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    second_image->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));

    second->Add(std::move(second_image));
    shell->SetSecond(std::move(second));
    shell->set_child_of_prev(true);
    shell->SetModel(model);

    shell->AddAction("kill_process");
    return shell;
}

std::unique_ptr<LambdaRule> MakeSpawnedFromDroppedFile(const EntityModel* model) {
    auto dropped = MakeRule<LambdaRule>(
        "spawned_from_dropped_file",
        Severity::kHigh,
        [model](const Event& event) {
            if (event.event_type() != EventType::kProcessStart) {
                return false;
            }
            const FileRecord* source = model->FindSpawnSource(event.pid());
            return source && !source->writer_pid.empty() && source->writer_pid != event.pid();
        });
    dropped->AddAction("kill_process");
    dropped->AddAction("quarantine_source");
    dropped->AddAction("delete_dropped");
    return dropped;
}

}  // namespace

RuleEngine BuildAgentRules(const EntityModel* model) {
    RuleEngine engine;
    engine.AddRule(MakeScriptHostFromTemp());
    engine.AddRule(MakeLolbinDownload());
    engine.AddRule(MakeHiddenPowershell());
    engine.AddRule(MakeAutostartWrite());
    engine.AddRule(MakeRansomExtension());
    engine.AddRule(MakeScriptHostFromBeacon());
    engine.AddRule(MakeMassWrite());
    engine.AddRule(MakeShellSpawnsScriptHost(model));
    engine.AddRule(MakeSpawnedFromDroppedFile(model));
    return engine;
}

}  // namespace nano_edr