/*
 * filehandler.h
 * -------------
 * Handles all persistent storage operations for flights and bookings.
 *
 * Storage format: plain text files, one record per line, pipe-delimited (|).
 * Files are fully rewritten on every save (appropriate for small datasets).
 *
 * Error handling:
 *   - Corrupt / malformed lines are skipped with a warning to stderr.
 *   - A missing data file is treated as an empty dataset (not an error).
 *   - A file that cannot be opened for writing throws std::runtime_error.
 */

#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <cerrno>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif
#include "flight.h"
#include "booking.h"

// Paths are relative to the backend/ working directory (where main.exe runs)
const std::string FLIGHTS_FILE  = "../data/flights.txt";
const std::string BOOKINGS_FILE = "../data/bookings.txt";

template <typename Writer>
inline void writeAtomically(const std::string& path, Writer writeRecords) {
    const std::string temporaryPath = path + ".tmp";
    try {
        std::ofstream file(temporaryPath, std::ios::trunc);
        if (!file.is_open())
            throw std::runtime_error("Cannot open temporary data file for writing: " + temporaryPath);
        writeRecords(file);
        file.flush();
        if (!file)
            throw std::runtime_error("Failed while writing data file: " + temporaryPath);
        file.close();
        if (!file)
            throw std::runtime_error("Failed while closing data file: " + temporaryPath);

#ifdef _WIN32
        if (!MoveFileExA(temporaryPath.c_str(), path.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            DWORD error = GetLastError();
            throw std::runtime_error("Cannot replace data file '" + path +
                                     "' (Windows error " + std::to_string(error) + ")");
        }
#else
        if (std::rename(temporaryPath.c_str(), path.c_str()) != 0)
            throw std::runtime_error("Cannot replace data file '" + path + "': " + std::strerror(errno));
#endif
    } catch (...) {
        std::remove(temporaryPath.c_str());
        throw;
    }
}

// ── Flight File Operations ────────────────────────────────────────────────────

/*
 * Reads all flight records from FLIGHTS_FILE into a vector.
 * Skips blank lines and lines that fail to parse (logs to stderr).
 * Returns an empty vector if the file does not exist.
 */
inline std::vector<Flight> loadFlights() {
    std::vector<Flight> flights;
    std::ifstream file(FLIGHTS_FILE);
    if (!file.is_open()) return flights;   // file absent = empty dataset

    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        if (line.empty()) continue;
        try {
            flights.push_back(lineToFlight(line));
        } catch (const std::exception& e) {
            std::cerr << "[filehandler] Skipping malformed flight at line "
                      << lineNum << ": " << e.what() << "\n";
        }
    }
    return flights;
}

/*
 * Writes all flight records to FLIGHTS_FILE, overwriting any existing content.
 * Throws std::runtime_error if the file cannot be opened for writing.
 */
inline void saveFlights(const std::vector<Flight>& flights) {
    writeAtomically(FLIGHTS_FILE, [&](std::ofstream& file) {
        for (const auto& f : flights)
            file << flightToLine(f) << "\n";
    });
}

// ── Booking File Operations ───────────────────────────────────────────────────

/*
 * Reads all booking records from BOOKINGS_FILE into a vector.
 * Skips blank lines and lines that fail to parse (logs to stderr).
 * Returns an empty vector if the file does not exist.
 */
inline std::vector<Booking> loadBookings() {
    std::vector<Booking> bookings;
    std::ifstream file(BOOKINGS_FILE);
    if (!file.is_open()) return bookings;

    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        if (line.empty()) continue;
        try {
            bookings.push_back(lineToBooking(line));
        } catch (const std::exception& e) {
            std::cerr << "[filehandler] Skipping malformed booking at line "
                      << lineNum << ": " << e.what() << "\n";
        }
    }
    return bookings;
}

/*
 * Writes all booking records to BOOKINGS_FILE, overwriting any existing content.
 * Throws std::runtime_error if the file cannot be opened for writing.
 */
inline void saveBookings(const std::vector<Booking>& bookings) {
    writeAtomically(BOOKINGS_FILE, [&](std::ofstream& file) {
        for (const auto& b : bookings)
            file << bookingToLine(b) << "\n";
    });
}

// ── Seat Availability Update ──────────────────────────────────────────────────

/*
 * Adjusts the seatsAvailable field of a specific flight by `delta`.
 * Use negative delta for booking (seats decrease) and positive for cancellation.
 * Loads, modifies, and saves the flights file atomically within this call.
 *
 * Returns false if the flight ID is not found or if the resulting seat count
 * would go below zero.
 */
inline bool updateSeats(const std::string& flightID, int delta) {
    auto flights = loadFlights();
    for (auto& f : flights) {
        if (f.flightID == flightID) {
            int newCount = f.seatsAvailable + delta;
            if (newCount < 0) return false;   // not enough seats
            f.seatsAvailable = newCount;
            saveFlights(flights);
            return true;
        }
    }
    return false;   // flight not found
}
