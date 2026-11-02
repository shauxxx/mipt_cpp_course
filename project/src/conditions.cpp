#include "conditions.h"
#include "fields.h"
#include "text.h"

namespace nano_edr {

bool FieldEquals::Matches(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return EqualLowerCase(*value, value_);
}

bool FieldContains::Matches(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return ContainsLowerCase(*value, needle_);
}

bool FieldEndsWith::Matches(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return EndIsLowerCase(*value, suffix_);
}

bool FieldInList::Matches(const Event& event) const {
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

bool EventTypeIs::Matches(const Event& event) const {
    const EventType& value = event.event_type();
    for (const auto& type: types_) {
        if (value == type) {
            return true;
        }
    }
    return false;
}

void AllOf::Add(std::unique_ptr<ICondition> condition) {
    parts_.push_back(std::move(condition));
}

void AnyOf::Add(std::unique_ptr<ICondition> condition) {
    parts_.push_back(std::move(condition));
}

bool AllOf::Matches(const Event& event) const {
    if (size() == 0) {
        return true;
    }
    for (const auto& part : parts_) {
        if (!part->Matches(event)) {
            return false;
        }
    }
    return true;
}

bool AnyOf::Matches(const Event& event) const {
    if (size() == 0) {
        return false;
    }
    for (const auto& part : parts_) {
        if (part->Matches(event)) {
            return true;
        }
    }
    return false;
}

}