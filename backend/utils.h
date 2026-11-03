/*
 * utils.h
 * -------
 * Utility functions shared across all backend modules:
 *   - JSON key extraction (no external library)
 *   - JSON response builders
 *   - Unique ID and date generation
 *   - Input validation helpers
 */

#pragma once
#include <string>
#include <ctime>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <cmath>
#include <cctype>
#include <vector>
#include "booking.h"
#include "json.h"

// ── JSON Helpers ──────────────────────────────────────────────────────────────

/*
 * Extracts the value of a key from a flat (non-nested) JSON string.
 * Handles both string values ("key":"value") and numeric values ("key":123).
 * Returns an empty string if the key is not found.
 *
 * Limitation: does not handle nested objects or arrays — sufficient for this
 * project since all request payloads are flat key-value maps.
 */
inline std::string jsonGet(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) pos++;

    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        std::string value;
        for (++pos; pos < json.size(); ++pos) {
            unsigned char c = static_cast<unsigned char>(json[pos]);
            if (c == '"') return value;
            if (c < 0x20) return "";
            if (c != '\\') {
                value += static_cast<char>(c);
                continue;
            }

            if (++pos >= json.size()) return "";
            switch (json[pos]) {
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                case '/': value += '/'; break;
                case 'b': value += '\b'; break;
                case 'f': value += '\f'; break;
                case 'n': value += '\n'; break;
                case 'r': value += '\r'; break;
                case 't': value += '\t'; break;
                case 'u': {
                    auto readCodeUnit = [&](size_t offset, unsigned int& codeUnit) {
                        if (offset + 4 > json.size()) return false;
                        codeUnit = 0;
                        for (size_t i = 0; i < 4; ++i) {
                            char digit = json[offset + i];
                            codeUnit <<= 4;
                            if (digit >= '0' && digit <= '9') codeUnit += digit - '0';
                            else if (digit >= 'a' && digit <= 'f') codeUnit += digit - 'a' + 10;
                            else if (digit >= 'A' && digit <= 'F') codeUnit += digit - 'A' + 10;
                            else return false;
                        }
                        return true;
                    };
                    unsigned int codePoint = 0;
                    if (!readCodeUnit(pos + 1, codePoint)) return "";
                    pos += 4;
                    if (codePoint >= 0xD800 && codePoint <= 0xDBFF) {
                        if (pos + 6 >= json.size() || json[pos + 1] != '\\' || json[pos + 2] != 'u')
                            return "";
                        unsigned int lowSurrogate = 0;
                        if (!readCodeUnit(pos + 3, lowSurrogate) ||
                            lowSurrogate < 0xDC00 || lowSurrogate > 0xDFFF)
                            return "";
                        codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (lowSurrogate - 0xDC00);
                        pos += 6;
                    } else if (codePoint >= 0xDC00 && codePoint <= 0xDFFF) {
                        return "";
                    }

                    if (codePoint <= 0x7F) value += static_cast<char>(codePoint);
                    else if (codePoint <= 0x7FF) {
                        value += static_cast<char>(0xC0 | (codePoint >> 6));
                        value += static_cast<char>(0x80 | (codePoint & 0x3F));
                    } else if (codePoint <= 0xFFFF) {
                        value += static_cast<char>(0xE0 | (codePoint >> 12));
                        value += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                        value += static_cast<char>(0x80 | (codePoint & 0x3F));
                    } else {
                        value += static_cast<char>(0xF0 | (codePoint >> 18));
                        value += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
                        value += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                        value += static_cast<char>(0x80 | (codePoint & 0x3F));
                    }
                    break;
                }
                default: return "";
            }
        }
        return "";
    }

    // numeric value — read until comma or closing brace
    size_t start = pos;
    size_t end   = json.find_first_of(",}", pos);
    if (end == std::string::npos) end = json.size();
    std::string val = json.substr(start, end - start);

    // trim surrounding whitespace
    size_t s = val.find_first_not_of(" \t\r\n");
    size_t e = val.find_last_not_of(" \t\r\n");
    return (s == std::string::npos) ? "" : val.substr(s, e - s + 1);
}

// ── Response Builders ─────────────────────────────────────────────────────────

inline std::string okJson(const std::string& dataJson) {
    return "{\"status\":\"ok\",\"data\":" + dataJson + "}";
}

inline std::string errJson(const std::string& msg) {
    return "{\"status\":\"error\",\"message\":\"" + jsonEscape(msg) + "\"}";
}

// ── ID and Date Generation ────────────────────────────────────────────────────

/*
 * Generates a unique ID by combining a prefix with the current Unix timestamp.
 * Collision probability is negligible for a single-user academic system.
 * Example output: "BK1720512345"
 */
inline std::string generateID(const std::string& prefix) {
    std::time_t t = std::time(nullptr);
    std::ostringstream ss;
    ss << prefix << t;
    return ss.str();
}

inline std::string generateUniqueBookingID(const std::vector<Booking>& bookings) {
    const std::string base = generateID("BK");
    std::string candidate = base;
    unsigned int suffix = 1;
    auto exists = [&](const std::string& id) {
        for (const auto& booking : bookings)
            if (booking.bookingID == id) return true;
        return false;
    };
    while (exists(candidate))
        candidate = base + "-" + std::to_string(suffix++);
    return candidate;
}

/*
 * Returns today's date as a YYYY-MM-DD string using local system time.
 */
inline std::string todayDate() {
    std::time_t t = std::time(nullptr);
    char buf[11];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&t));
    return std::string(buf);
}

// ── Input Validation ──────────────────────────────────────────────────────────

/*
 * Returns true if the string is non-empty after trimming whitespace.
 */
inline bool isNonEmpty(const std::string& s) {
    return s.find_first_not_of(" \t\r\n") != std::string::npos;
}

inline bool isSafeRecordField(const std::string& value, size_t maxLength) {
    if (value.empty() || value.size() > maxLength) return false;
    for (unsigned char c : value)
        if (c < 0x20 || c == '|' || c == '"' || c == '\\') return false;
    return true;
}

/*
 * Validates a Flight ID: must be non-empty and contain only alphanumeric
 * characters (e.g. "FL001", "AI202").
 */
inline bool isValidFlightID(const std::string& id) {
    if (!isNonEmpty(id) || id.size() > 20) return false;
    for (char c : id)
        if (!std::isalnum(static_cast<unsigned char>(c))) return false;
    return true;
}

/*
 * Validates a date string in YYYY-MM-DD format.
 * Checks format only — does not validate calendar correctness.
 */
inline bool isValidDate(const std::string& date) {
    // Must match YYYY-MM-DD exactly
    if (date.size() != 10) return false;
    if (date[4] != '-' || date[7] != '-') return false;
    for (int i : {0,1,2,3,5,6,8,9})
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) return false;
    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));
    if (year < 1 || month < 1 || month > 12) return false;
    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysInMonth[month - 1];
    bool leapYear = year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
    if (month == 2 && leapYear) maxDay = 29;
    return day >= 1 && day <= maxDay;
}

/*
 * Validates a time string in HH:MM format (24-hour clock).
 */
inline bool isValidTime(const std::string& t) {
    if (t.size() != 5) return false;
    if (t[2] != ':') return false;
    for (int i : {0,1,3,4})
        if (!std::isdigit(static_cast<unsigned char>(t[i]))) return false;
    int hh = std::stoi(t.substr(0, 2));
    int mm = std::stoi(t.substr(3, 2));
    return hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59;
}

/*
 * Validates an email address using a basic regex pattern.
 * Accepts standard formats like "user@domain.com".
 */
inline bool isValidEmail(const std::string& email) {
    if (!isNonEmpty(email) || email.size() > 120) return false;
    static const std::regex emailRe(R"([a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, emailRe);
}

/*
 * Validates passenger age: must be between 1 and 120 inclusive.
 */
inline bool isValidAge(int age) {
    return age >= 1 && age <= 120;
}

/*
 * Validates seat count: must be between 1 and 10 inclusive.
 * (Reasonable upper limit for a single booking transaction.)
 */
inline bool isValidSeatCount(int seats) {
    return seats >= 1 && seats <= 10;
}

/*
 * Validates ticket price: must be a positive value.
 */
inline bool isValidPrice(double price) {
    return price > 0.0;
}

/*
 * Validates flight duration: must be a positive number of minutes.
 */
inline bool isValidDuration(int minutes) {
    return minutes > 0;
}

/*
 * Safe integer parser — returns false and sets out=0 on failure.
 */
inline bool safeParseInt(const std::string& s, int& out) {
    if (s.empty()) return false;
    try {
        size_t parsed = 0;
        int value = std::stoi(s, &parsed);
        if (parsed != s.size()) return false;
        out = value;
        return true;
    }
    catch (...) { out = 0; return false; }
}

/*
 * Safe double parser — returns false and sets out=0.0 on failure.
 */
inline bool safeParseDbl(const std::string& s, double& out) {
    if (s.empty()) return false;
    try {
        size_t parsed = 0;
        double value = std::stod(s, &parsed);
        if (parsed != s.size() || !std::isfinite(value)) return false;
        out = value;
        return true;
    }
    catch (...) { out = 0.0; return false; }
}
