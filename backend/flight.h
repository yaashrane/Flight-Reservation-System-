/*
 * flight.h
 * --------
 * Defines the Flight data structure and all serialization helpers.
 *
 * Fields
 * ------
 *  flightID       : Unique identifier, e.g. "FL001"
 *  airline        : Airline name, e.g. "IndiGo"
 *  source         : Departure city
 *  destination    : Arrival city
 *  flightDate     : Date of flight  (YYYY-MM-DD)
 *  departureTime  : Departure time  (HH:MM, 24-hour)
 *  arrivalTime    : Arrival time    (HH:MM, 24-hour)
 *  durationMin    : Flight duration in minutes  (derived / stored)
 *  price          : Ticket price in INR
 *  seatsAvailable : Remaining bookable seats
 *
 * File format (pipe-delimited, one record per line)
 * -------------------------------------------------
 *  flightID|airline|source|destination|flightDate|departureTime|arrivalTime|durationMin|price|seatsAvailable
 */

#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include "json.h"

// ── Data Structure ────────────────────────────────────────────────────────────

struct Flight {
    std::string flightID;
    std::string airline;
    std::string source;
    std::string destination;
    std::string flightDate;       // YYYY-MM-DD
    std::string departureTime;    // HH:MM  (24-hour)
    std::string arrivalTime;      // HH:MM  (24-hour)
    int         durationMin;      // total flight duration in minutes
    double      price;            // ticket price in INR
    int         seatsAvailable;   // seats still open for booking
};

// ── Serialization ─────────────────────────────────────────────────────────────

/*
 * Converts a Flight struct into a single pipe-delimited line for file storage.
 * Format: flightID|airline|source|destination|flightDate|departureTime|arrivalTime|durationMin|price|seatsAvailable
 */
inline std::string flightToLine(const Flight& f) {
    return f.flightID       + "|" +
           f.airline        + "|" +
           f.source         + "|" +
           f.destination    + "|" +
           f.flightDate     + "|" +
           f.departureTime  + "|" +
           f.arrivalTime    + "|" +
           std::to_string(f.durationMin)    + "|" +
           std::to_string(f.price)          + "|" +
           std::to_string(f.seatsAvailable);
}

/*
 * Parses a pipe-delimited line from the data file back into a Flight struct.
 * Throws std::invalid_argument if the line has fewer than 10 fields.
 */
inline Flight lineToFlight(const std::string& line) {
    Flight f;
    std::stringstream ss(line);
    std::string tok;

    auto next = [&](std::string& field) {
        if (!std::getline(ss, field, '|'))
            throw std::invalid_argument("Malformed flight record: " + line);
    };

    next(f.flightID);
    next(f.airline);
    next(f.source);
    next(f.destination);
    next(f.flightDate);
    next(f.departureTime);
    next(f.arrivalTime);

    next(tok); f.durationMin    = std::stoi(tok);
    next(tok); f.price          = std::stod(tok);
    next(tok); f.seatsAvailable = std::stoi(tok);

    return f;
}

// ── JSON Serialization ────────────────────────────────────────────────────────

/*
 * Serializes a single Flight to a JSON object string.
 * Used by the HTTP bridge to return data to the frontend.
 */
inline std::string flightToJson(const Flight& f) {
    // Format price to 2 decimal places for clean JSON output
    char priceBuf[32];
    std::snprintf(priceBuf, sizeof(priceBuf), "%.2f", f.price);

    return "{\"flightID\":\""      + jsonEscape(f.flightID)      +
           "\",\"airline\":\""     + jsonEscape(f.airline)        +
           "\",\"source\":\""      + jsonEscape(f.source)         +
           "\",\"destination\":\"" + jsonEscape(f.destination)    +
           "\",\"flightDate\":\""  + jsonEscape(f.flightDate)     +
           "\",\"departureTime\":\"" + jsonEscape(f.departureTime) +
           "\",\"arrivalTime\":\"" + jsonEscape(f.arrivalTime)    +
           "\",\"durationMin\":"   + std::to_string(f.durationMin) +
           ",\"price\":"           + priceBuf         +
           ",\"seatsAvailable\":"  + std::to_string(f.seatsAvailable) + "}";
}

/*
 * Serializes a vector of Flights to a JSON array string.
 */
inline std::string flightsToJson(const std::vector<Flight>& flights) {
    std::string out = "[";
    for (size_t i = 0; i < flights.size(); i++) {
        if (i) out += ",";
        out += flightToJson(flights[i]);
    }
    return out + "]";
}
