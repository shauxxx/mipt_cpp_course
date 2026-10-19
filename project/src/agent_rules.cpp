#include "agent_rules.h"

#include <string>

#include "fields.h"
#include "rules.h"

//============================================================================
// Пять правил детектирования и таблица правил.
//
// Все функции-проверки (ScriptHostFromTemp и т.д.) возвращают bool.
// Они не ловят исключения — если GetRequiredField бросит
// std::invalid_argument (например, process_start без image), исключение
// пролетит вверх по стеку до catch в main. Это правильное поведение:
// отсутствие обязательного поля — нарушение контракта формата журнала.
//
// AgentRules и AgentRuleCount не бросают исключений.
// ===========================================================================

namespace nano_edr {

namespace {
const std::string* CheckTypeFile(const Event& event) {
    const std::string* ans = nullptr;
    const std::string& check_type = event.type;
    if (check_type == "file_create" || check_type == "file_write" || check_type == "file_move") {
        if (event.type == "file_move") {
            ans = FindField(event, "to");
        } else {
            ans = FindField(event, "path");
        }
        return ans;
    }
    return ans;
}

bool CheckTypeProcess(const Event& event) {
    const std::string& check_type = event.type;
    if (check_type == "process_start") {
        return true;
    }
    return false;
}

bool ScriptHostFromTemp(const Event& event) {
    if (!CheckTypeProcess(event)) {
        return false;
    }

    const std::string& image = GetRequiredField(event, "image");
    std::string norm_image = NormalizePath(image);
    if (norm_image.ends_with("wscript.exe") || norm_image.ends_with("cscript.exe")) {
        if (CommandLineContains(event, "\\appdata\\local\\temp\\") || CommandLineContains(event, "\\windows\\temp\\")) {
            return true;
        }
    }
    return false;
}

bool LolbinDownload(const Event& event) {
    if (!CheckTypeProcess(event)) {
        return false;
    }

    const std::string& image = GetRequiredField(event, "image");
    std::string norm_image = NormalizePath(image);
    if (norm_image.ends_with("certutil.exe") || norm_image.ends_with("bitsadmin.exe")) {
        if (CommandLineContains(event, "urlcache") || CommandLineContains(event, "transfer") || CommandLineContains(event, "http:") || CommandLineContains(event, "https:")) {
            return true;
        }
    }
    return false;
}

bool HiddenPowershell(const Event& event) {
    if (!CheckTypeProcess(event)) {
        return false;
    }

    const std::string& image = GetRequiredField(event, "image");
    std::string norm_image = NormalizePath(image);
    if (norm_image.ends_with("powershell.exe") || norm_image.ends_with("pwsh.exe")) {
        if (CommandLineContains(event, "-w hidden") || CommandLineContains(event, "-windowstyle hidden") || CommandLineContains(event, "-enc") || CommandLineContains(event, "-encodedcommand")) {
            return true;
        }
    }
    return false;
}

bool AutostartWrite(const Event& event) {
    const std::string* path_ptr = CheckTypeFile(event);

    if (path_ptr == nullptr) {
        return false;
    }

    std::string normalized = NormalizePath(*path_ptr);
    if (normalized.find("\\start menu\\programs\\startup\\") != std::string::npos) {
        return true;
    }

    return false;
}

bool RansomExtension(const Event& event) {
    const std::string* path_ptr = CheckTypeFile(event);

    if (path_ptr == nullptr) {
        return false;
    }

    std::string normalized = NormalizePath(*path_ptr);
    if (normalized.ends_with(".locked")) {
        return true;
    }

    return false;
}

constexpr Rule kRules[] = {
    {"script_host_from_temp", ScriptHostFromTemp, Severity::kHigh},
    {"lolbin_download", LolbinDownload, Severity::kHigh},
    {"hidden_powershell", HiddenPowershell, Severity::kMedium},
    {"autostart_write", AutostartWrite, Severity::kHigh},
    {"ransom_extension", RansomExtension, Severity::kCritical}};

}  // namespace
const Rule* AgentRules() {
    return kRules;
}

size_t AgentRuleCount() {
    return std::size(kRules);
}

}  // namespace nano_edr