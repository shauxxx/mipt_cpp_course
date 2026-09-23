#include "../kit/include/l1.2/parse.h"

namespace nano_edr {

bool IsBlankOrComment(const std::string* line) {
    if (!line) return true;

    std::size_t i = 0;
    while (i < line->size() &&
           ((*line)[i] == ' '  || (*line)[i] == '\t' ||
            (*line)[i] == '\r' || (*line)[i] == '\n')) {
        ++i;
    }
    if (i >= line->size()) return true;
    return (*line)[i] == '#' || (*line)[i] == ';';
}

bool ParseEventLine(const std::string* line, Event* out) {
    if (!line || !out) return false;
    if (IsBlankOrComment(line)) return false;

    const std::string& s = *line;
    std::size_t i = 0;
    bool has_ts = false, has_type = false, has_pid = false;

    while (i < s.size()) {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
        if (i >= s.size()) break;

        std::size_t key_start = i;
        while (i < s.size() && s[i] != '=' && s[i] != ' ' && s[i] != '\t') ++i;
        if (i >= s.size() || s[i] != '=') return false;

        std::string key = s.substr(key_start, i - key_start);
        if (key.empty()) return false;
        ++i;

        std::string value;
        if (i < s.size() && s[i] == '"') {
            ++i;
            std::size_t v_start = i;
            while (i < s.size() && s[i] != '"') ++i;
            if (i >= s.size()) return false;
            value = s.substr(v_start, i - v_start);
            ++i;
        } else {
            std::size_t v_start = i;
            while (i < s.size() && s[i] != ' ' && s[i] != '\t') ++i;
            value = s.substr(v_start, i - v_start);
        }

        if (key == "ts"   && !has_ts) { 
            out->ts   = value; has_ts   = true; 
        }
        else if (key == "type" && !has_type) { 
            out->type = value; has_type = true; 
        }
        else if (key == "pid"  && !has_pid)  {
             out->pid  = value; has_pid  = true; 
        }
        else {
            Field f;
            f.key   = key;
            f.value = value;
            out->fields.push_back(f);
        }
    }

    return has_ts && has_type;
}

}