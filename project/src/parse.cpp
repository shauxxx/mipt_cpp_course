#include "../kit/include/l2.1/parse.h"

namespace nano_edr {

bool IsBlankOrComment(const std::string& line) {
    std::size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
        ++i;
    }
    if (i >= line.size()) return true;
    return line[i] == '#' || line[i] == ';';
}

bool ParseEventParts(const std::string& line, EventParts* out) {
    if (!out) return false;
    
    std::size_t i = 0;
    bool has_ts = false, has_type = false, has_pid = false;
    
    while (i < line.size()) {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
        if (i >= line.size()) break;
        
        std::size_t key_start = i;
        while (i < line.size() && line[i] != '=' && line[i] != ' ' && line[i] != '\t') ++i;
        
        if (i >= line.size() || line[i] != '=') return false;
        
        std::string key = line.substr(key_start, i - key_start);
        if (key.empty()) return false;
        ++i; 

        std::string value;
        if (i < line.size() && line[i] == '"') {
            ++i;
            std::size_t v_start = i;
            while (i < line.size() && line[i] != '"') ++i;
            if (i >= line.size()) return false;
            value = line.substr(v_start, i - v_start);
            ++i;
        } else {
            std::size_t v_start = i;
            while (i < line.size() && line[i] != ' ' && line[i] != '\t') ++i;
            value = line.substr(v_start, i - v_start);
        }
        
        if (key == "ts" && !has_ts) {
            out->ts = value;
            has_ts = true;
        } else if (key == "type" && !has_type) {
            out->type = value;
            has_type = true;
        } else if (key == "pid" && !has_pid) {
            out->pid = value;
            has_pid = true;
        } else {
            out->fields.push_back({key, value});
        }
    }
    
    return true;
}

} 