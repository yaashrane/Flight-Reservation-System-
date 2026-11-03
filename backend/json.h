#pragma once
#include <sstream>
#include <string>

inline std::string jsonEscape(const std::string& value) {
    std::ostringstream escaped;
    for (unsigned char c : value) {
        switch (c) {
            case '"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (c < 0x20) {
                    const char hex[] = "0123456789abcdef";
                    escaped << "\\u00" << hex[c >> 4] << hex[c & 0x0F];
                } else {
                    escaped << static_cast<char>(c);
                }
        }
    }
    return escaped.str();
}
