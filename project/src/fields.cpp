#include "../kit/include/l2.1/fields.h"
#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <string>
#include <system_error>
#include "charconv"

namespace nano_edr {

const std::string* FindField(const Event& event, const std::string& key) {
    if (key == "ts") return &event.raw_ts();
    if (key == "type") return &event.type();
    if (key == "pid") return &event.pid();

    for (const auto& pos_fields : event.fields()) {
        if (pos_fields.key == key) {
            return &pos_fields.value;
        }
    }
    return nullptr;
}

const std::string& GetRequiredField(const Event& event, const std::string& key) {
    const std::string* finded = FindField(event, key);
    if (finded == nullptr) {
        throw std::invalid_argument("Required field not found: " + key);
    }
    return *finded;
}

bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    uint64_t buf = 0;
    if (!out) return false;
    
    const std::string* finded = FindField(event, key);
    if (finded == nullptr) return false;
    
    auto [pt, erc] = std::from_chars(finded->data(), finded->data() + finded->size(), buf);
    if (erc == std::errc() && pt == finded->data() + finded->size()) {
        *out = buf;
        return true;
    }
    return false;
}

uint64_t GetIntField(const Event& event, const std::string& key, uint64_t fallback) {
    uint64_t buf = 0;
    if (GetIntField(event, key, &buf)) {
        return buf;
    }
    return fallback;
}

bool IsProcessStart(const Event& event) {
    return event.type() == "process_start";
}

bool IsFileWrite(const Event& event) {
    return event.type() == "file_write";
}

bool IsNetConnect(const Event& event) {
    return event.type() == "net_connect";
}

bool PathEndsWith(const Event& event, const std::string& suffix) {
    std::string find = NormalizePath(suffix);
    const std::string* buf = FindField(event, "path");
    if (buf == nullptr) return false;
    
    std::string compared = NormalizePath(*buf);
    return compared.ends_with(find);
}

bool CommandLineContains(const Event& event, const std::string& needle) {
    std::string find = NormalizePath(needle);
    const std::string* buf = FindField(event, "cmdline");
    if (buf == nullptr) return false;
    
    std::string compared = NormalizePath(*buf);
    return compared.find(find) != std::string::npos;
}

std::string NormalizePath(const std::string& path) {
    std::string res = path;
    std::ranges::replace(res, '/', '\\');
    std::ranges::transform(res, res.begin(), [](unsigned char c) -> char {
        return static_cast<char>(std::tolower(c));
    });
    auto [trash_start, trash_end] = std::ranges::unique(res, [](char a, char b) {
        return a == '\\' && a == b;
    });
    res.erase(trash_start, trash_end);
    
    if (res.find("%temp%") != std::string::npos) {
        res.replace(res.find("%temp%"), 6, "\\appdata\\local\\temp");
    }
    if (res.find("%tmp%") != std::string::npos) {
        res.replace(res.find("%tmp%"), 5, "\\appdata\\local\\temp");
    }
    return res;
}

}