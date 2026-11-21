#include "event.h"
#include "copy_stats.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <charconv>

namespace nano_edr {

namespace {

Timestamp ParseTimestamp(const std::string& text) {
    Timestamp result;
    const char* first = text.data();
    const char* last = first + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, result.ms);
    if (ec != std::errc() || ptr != last) {
        throw std::invalid_argument("неверное время события: '" + text + "'");
    }
    return result;
}
}  //namespace

EventType EventTypeFromString(const std::string& type) {
    if (type == "process_start") {
        return EventType::kProcessStart;
    } else if (type == "process_end") {
        return EventType::kProcessEnd;
    } else if (type == "file_create") {
        return EventType::kFileCreate;
    } else if (type == "file_write") {
        return EventType::kFileWrite;
    } else if (type == "file_delete") {
        return EventType::kFileDelete;
    } else if (type == "file_move") {
        return EventType::kFileMove;
    } else if (type == "net_connect") {
        return EventType::kNetConnect;
    }
    return EventType::kOther;
}

Event::Event(const EventParts& parts) {
    ts_ = Validate(parts.ts, parts.type);
    raw_ts_ = parts.ts;
    type_ = parts.type;
    event_type_ = EventTypeFromString(type_);
    pid_ = parts.pid;
    fields_ = parts.fields;
    copy_stats().event_strings_copied += 3 + 2 * parts.fields.size();
}

Event::Event(EventParts&& parts) {
    ts_ = Validate(parts.ts, parts.type);
    raw_ts_ = std::move(parts.ts);
    type_ = std::move(parts.type);
    event_type_ = EventTypeFromString(type_);
    pid_ = std::move(parts.pid);
    copy_stats().event_strings_moved += 3 + 2 * parts.fields.size();
    fields_ = std::move(parts.fields);
}

Timestamp Event::Validate(const std::string& raw_ts, const std::string& type) {
    if (raw_ts.empty()) {
        throw std::invalid_argument("ts не может быть пустым");
    }
    if (type.empty()) {
        throw std::invalid_argument("type не может быть пустым");
    }
    return ParseTimestamp(raw_ts);
}

std::string ToString(const Event& event) {
    std::string result = "ts=" + event.raw_ts() + " type=" + event.type();
    if (!event.pid().empty()) {
        result += " pid=" + event.pid();
    }
    for (const auto& field : event.fields()) {
        result += " " + field.key + "=" + field.value;
    }
    return result;
}

std::ostream& operator<<(std::ostream& out, const Event& event) {
    return out << ToString(event);
}

}  // namespace nano_edr