#include "conditions.h"
#include "fields.h"
#include "text.h"

namespace nano_edr {

bool FieldEquals::operator()(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return EqualLowerCase(*value, value_);
}

bool FieldContains::operator()(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return ContainsLowerCase(*value, needle_);
}

bool FieldEndsWith::operator()(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return EndIsLowerCase(*value, suffix_);
}

bool FieldInList::operator()(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    for (const std::string& val : values_) {
        if (EqualLowerCase(*value, val)) {
            return true;
        }
    }
    return false;
}

Condition Condition::Equals(const std::string& key, const std::vector<std::string>& values) {
    return Condition(Match::kEquals, key, values); 
}
Condition Condition::Contains(const std::string& key, const std::vector<std::string>& values) {
    return Condition(Match::kContains, key, values);
}
Condition Condition::EndsWith(const std::string& key, const std::vector<std::string>& values) {
    return Condition(Match::kEndsWith, key, values);
}

bool Condition::operator()(const Event &event) const {
    const std::string* value = FindField(event, key_);
    if (!value) {
        return false;
    }
    for (const auto& val : values_) {
        switch (match_) {
            case Match::kEquals:
                if (EqualLowerCase(*value, val)) return true;
                break;
            case Match::kContains:
                if (ContainsLowerCase(*value, val)) return true;
                break;
            case Match::kEndsWith:
                if (EndIsLowerCase(*value, val)) return true;
                break;
        }
    }
    return false;
}




}