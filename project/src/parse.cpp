#include "../kit/include/l1.2/parse.h"




namespace nano_edr {
   
bool IsBlankOrComment(const std::string* line) {
    if (!line) return true;

    std::size_t i = 0;
    while (i < line->size() && (line->operator[](i)==' ' || line->operator[](i)=='\t')) ++i;

    if (i >= line->size()) return true;

    return line->operator[](i) == '#' || line->operator[](i) == ';';
}

bool ParseEventLine(const std::string* line, Event* out) {
    if (IsBlankOrComment(line)) return false;

    const std::string& s = *line;
    std::size_t i = 0;
    bool has_ts = false, has_type = false, has_pid = false;

    while (i < s.size()) {
        // пропускаем пробелы и табы
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
        if (i >= s.size()) break;

        // ключ — до '='
        std::size_t key_start = i;
        while (i < s.size() && s[i] != '=' && s[i] != ' ' && s[i] != '\t') ++i;
        if (i >= s.size() || s[i] != '=') return false;   // "word" без '='
        std::string key = s.substr(key_start, i - key_start);
        if (key.empty()) return false;                    // "=value"
        ++i;                                              // через '='

        // значение — в кавычках или до пробела
        std::string value;
        if (i < s.size() && s[i] == '"') {
            ++i;
            std::size_t v_start = i;
            while (i < s.size() && s[i] != '"') ++i;
            if (i >= s.size()) return false;              // кавычка не закрыта
            value = s.substr(v_start, i - v_start);
            ++i;
            if (i < s.size() && s[i] != ' ' && s[i] != '\t') return false; // мусор после "
        } else {
            std::size_t v_start = i;
            while (i < s.size() && s[i] != ' ' && s[i] != '\t') ++i;
            value = s.substr(v_start, i - v_start);
        }

        if      (key == "ts"   && !has_ts)   { out->ts   = value; has_ts   = true; }
        else if (key == "type" && !has_type) { out->type = value; has_type = true; }
        else if (key == "pid"  && !has_pid)  { out->pid  = value; has_pid  = true; }
        else {
            Field f;
            f.key = key;
            f.value = value;
            out->fields.push_back(f);
        }
    }

    return has_ts && has_type;
}
}