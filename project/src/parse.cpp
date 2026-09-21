#include "../kit/include/l1.2/parse.h"

namespace nano_edr {

bool IsBlankOrComment(const std::string* line) {
    if (line == nullptr) {
        return true;
    }
    std::size_t curr = 0;
    while (curr < line->size() && (line->operator[](curr) == ' ' || line->operator[](curr) == '\t')) {
        ++curr;
    }
    if (curr >= line->size()) {
        return true;
    }
    return line->operator[](curr) == '#' || line->operator[](curr) == ';';
}

bool ParseEventLine(const std::string* line, Event* out) {
    if (IsBlankOrComment(line)) {
        return false;
    }
    const std::string& loc_line = *line;
    std::size_t curr = 0;
    bool has_ts = false, has_type = false, has_pid = false;

    while (curr < loc_line.size()) {
        while (curr < loc_line.size() && (loc_line[curr] == ' ' || loc_line[curr] == '\t')) ++curr;
        if (curr >= loc_line.size()) break;

        std::size_t key_start = curr;
        while (curr < loc_line.size() && loc_line[curr] != '=' && loc_line[curr] != ' ' && loc_line[curr] != '\t') ++curr;
        if (curr >= loc_line.size() || loc_line[curr] != '=') return false;

        std::string key = loc_line.substr(key_start, curr - key_start);
        if (key.empty()) return false;

        ++curr;

        std::string value;
        if (curr < loc_line.size() && loc_line[curr] == '"') {
            ++curr;
            std::size_t v_start = curr;
            while (curr < loc_line.size() && loc_line[curr] != '"') ++curr;
            if (curr >= loc_line.size()) return false;

            value = loc_line.substr(v_start, curr - v_start);
            ++curr;

            if (curr < loc_line.size() && loc_line[curr] != ' ' && loc_line[curr] != '\t') return false;
        } else {
            std::size_t v_start = curr;
            while (curr < loc_line.size() && loc_line[curr] != ' ' && loc_line[curr] != '\t') ++curr;
            value = loc_line.substr(v_start, curr - v_start);
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
            Field f;
            f.key = key;
            f.value = value;
            out->fields.push_back(f);
        }
    }

    return has_ts && has_type;
}

}