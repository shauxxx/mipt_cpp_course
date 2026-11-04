#include "field_traits.h"

#include <charconv>

#include "text.h"

namespace nano_edr {

bool FieldTraits<std::string>::Parse(const std::string& text, std::string* out) {
    *out = text;
    return true;
}
bool FieldTraits<uint64_t>::Parse(const std::string& text, uint64_t* out) {
    uint64_t buf = 0;
    auto [pt, erc] = std::from_chars(text.data(), text.data() + text.size(), buf);
    if (erc == std::errc() && pt == text.data() + text.size()) {
        *out = buf;
        return true;
    }
    return false;
}
bool FieldTraits<bool>::Parse(const std::string& text, bool* out) {
    if (text == "1" || EqualLowerCase(text, "true") || EqualLowerCase(text, "yes")) {
        *out = true;
        return true;
    }
    if (text == "0" || EqualLowerCase(text, "false") || EqualLowerCase(text, "no")) {
        *out = false;
        return true;
    }
    return false;
}

}  // namespace nano_edr
