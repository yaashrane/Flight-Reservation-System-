/*
 * booking.h
 * ---------
 * Defines the Booking data structure and all serialization helpers.
 *
 * Fields
 * ------
 *  bookingID      : Unique identifier, e.g. "BK1720000000"
 *  passengerName  : Full name of the passenger
 *  passengerAge   : Age of the passenger (must be 1–120)
 *  passengerEmail : Contact email address
 *  flightID       : References the booked Flight
 *  bookingDate    : Date the booking was made (YYYY-MM-DD)
 *  status         : "confirmed" or "cancelled"
 *  seats          : Number of seats booked
 *
 * File format (pipe-delimited, one record per line)
 * -------------------------------------------------
 *  bookingID|passengerName|passengerAge|passengerEmail|flightID|bookingDate|status|seats
 */

#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include "json.h"

// ── Data Structure ────────────────────────────────────────────────────────────

struct Booking {
    std::string bookingID;
    std::string passengerName;
    int         passengerAge;     // validated: 1–120
    std::string passengerEmail;
    std::string flightID;
    std::string bookingDate;      // YYYY-MM-DD
    std::string status;           // "confirmed" | "cancelled"
    int         seats;            // number of seats booked
};

// ── Serialization ─────────────────────────────────────────────────────────────

/*
 * Converts a Booking struct into a single pipe-delimited line for file storage.
 * Format: bookingID|passengerName|passengerAge|passengerEmail|flightID|bookingDate|status|seats
 */
inline std::string bookingToLine(const Booking& b) {
    return b.bookingID      + "|" +
           b.passengerName  + "|" +
           std::to_string(b.passengerAge) + "|" +
           b.passengerEmail + "|" +
           b.flightID       + "|" +
           b.bookingDate    + "|" +
           b.status         + "|" +
           std::to_string(b.seats);
}

/*
 * Parses a pipe-delimited line from the data file back into a Booking struct.
 * Throws std::invalid_argument if the line has fewer than 8 fields.
 */
inline Booking lineToBooking(const std::string& line) {
    Booking b;
    std::stringstream ss(line);
    std::string tok;

    auto next = [&](std::string& field) {
        if (!std::getline(ss, field, '|'))
            throw std::invalid_argument("Malformed booking record: " + line);
    };

    next(b.bookingID);
    next(b.passengerName);
    next(tok); b.passengerAge = std::stoi(tok);
    next(b.passengerEmail);
    next(b.flightID);
    next(b.bookingDate);
    next(b.status);
    next(tok); b.seats = std::stoi(tok);

    return b;
}

// ── JSON Serialization ────────────────────────────────────────────────────────

/*
 * Serializes a single Booking to a JSON object string.
 */
inline std::string bookingToJson(const Booking& b) {
    return "{\"bookingID\":\""      + jsonEscape(b.bookingID)      +
           "\",\"passengerName\":\"" + jsonEscape(b.passengerName)  +
           "\",\"passengerAge\":"   + std::to_string(b.passengerAge) +
           ",\"passengerEmail\":\"" + jsonEscape(b.passengerEmail)  +
           "\",\"flightID\":\""     + jsonEscape(b.flightID)        +
           "\",\"bookingDate\":\""  + jsonEscape(b.bookingDate)     +
           "\",\"status\":\""       + jsonEscape(b.status)          +
           "\",\"seats\":"          + std::to_string(b.seats) + "}";
}

/*
 * Serializes a vector of Bookings to a JSON array string.
 */
inline std::string bookingsToJson(const std::vector<Booking>& bookings) {
    std::string out = "[";
    for (size_t i = 0; i < bookings.size(); i++) {
        if (i) out += ",";
        out += bookingToJson(bookings[i]);
    }
    return out + "]";
}
