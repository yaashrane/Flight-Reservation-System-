/*
 * main.cpp
 * --------
 * Entry point for the Flight Management and Reservation System backend.
 *
 * Execution model:
 *   The binary is invoked by the Python HTTP bridge (server/server.py) with a
 *   single command-line argument: a flat JSON string representing the request.
 *   The binary processes the request and prints a JSON response to stdout.
 *
 *   Example invocation:
 *     main.exe "{\"action\":\"searchFlight\",\"key\":\"source\",\"value\":\"Delhi\"}"
 *
 * Action routing:
 *   getFlights        → return all flights
 *   searchFlight      → linear search (source/destination/airline/date) or
 *                        binary search (flightID)
 *   searchFlights     → linear search by route/date, optionally followed by
 *                        bubble / selection / merge sort
 *   sortFlights       → bubble (price) / selection (time) / merge (duration)
 *   bookFlight        → create booking, decrement seats
 *   cancelBooking     → mark cancelled, restore seats
 *   getBookings       → return bookings (optionally filtered by email)
 *   addFlight         → admin: insert new flight
 *   updateFlight      → admin: modify existing flight fields
 *   deleteFlight      → admin: remove flight record
 */

#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include "flight.h"
#include "booking.h"
#include "filehandler.h"
#include "search.h"
#include "sort.h"
#include "utils.h"

// ── Handler: Get All Flights ──────────────────────────────────────────────────

std::string handleGetFlights() {
    auto flights = loadFlights();
    return okJson(flightsToJson(flights));
}

// ── Handler: Search Flights ───────────────────────────────────────────────────

std::string handleSearchFlight(const std::string& json) {
    std::string key   = jsonGet(json, "key");
    std::string value = jsonGet(json, "value");

    // Validation
    if (key.empty())
        return errJson("Search key is required (source/destination/flightID/airline/date/source_destination)");
    if (!isNonEmpty(value))
        return errJson("Search value cannot be empty");

    auto flights = loadFlights();
    std::vector<Flight> result;

    if (key == "flightID") {
        // Binary Search — O(log n), pre-sorted by Insertion Sort
        if (!isValidFlightID(value))
            return errJson("Invalid Flight ID format. Use alphanumeric characters only (e.g. FL001)");
        result = searchByID(flights, value);
    } else {
        if (key != "source" && key != "destination" && key != "airline" &&
            key != "date" && key != "source_destination")
            return errJson("Unknown search key '" + key + "'");
        if (key == "date" && !isValidDate(value))
            return errJson("Invalid flight date. Use YYYY-MM-DD format");
        // Linear Search — O(n)
        result = linearSearch(flights, key, value);
    }

    return okJson(flightsToJson(result));
}

std::vector<Flight> filterFlights(const std::vector<Flight>& flights,
                                  const std::string& source,
                                  const std::string& destination,
                                  const std::string& date) {
    std::vector<Flight> result = flights;

    if (!source.empty() && !destination.empty())
        result = linearSearch(result, "source_destination", source + "," + destination);
    else if (!source.empty())
        result = linearSearch(result, "source", source);
    else if (!destination.empty())
        result = linearSearch(result, "destination", destination);

    if (!date.empty())
        result = linearSearch(result, "date", date);

    return result;
}

std::string handleSearchFlights(const std::string& json) {
    const std::string source = jsonGet(json, "source");
    const std::string destination = jsonGet(json, "destination");
    const std::string date = jsonGet(json, "date");
    const std::string sortBy = jsonGet(json, "sortBy");

    if (!date.empty() && !isValidDate(date))
        return errJson("Invalid flight date. Use YYYY-MM-DD format");

    auto flights = filterFlights(loadFlights(), source, destination, date);
    if (sortBy == "price")
        bubbleSortByPrice(flights);
    else if (sortBy == "time")
        selectionSortByDepartureTime(flights);
    else if (sortBy == "duration")
        mergeSortByDuration(flights);
    else if (!sortBy.empty())
        return errJson("Unknown sort key '" + sortBy + "'. Valid options: price, time, duration");

    return okJson(flightsToJson(flights));
}

// ── Handler: Sort Flights ─────────────────────────────────────────────────────

std::string handleSortFlights(const std::string& json) {
    std::string by = jsonGet(json, "by");

    if (by.empty())
        return errJson("Sort key is required (price/time/duration)");

    auto flights = loadFlights();
    if (flights.empty())
        return okJson("[]");

    if (by == "price") {
        // Bubble Sort — O(n²) average/worst, O(n) best
        bubbleSortByPrice(flights);
    }
    else if (by == "time") {
        // Selection Sort — O(n²)
        selectionSortByDepartureTime(flights);
    }
    else if (by == "duration") {
        // Merge Sort — O(n log n)
        mergeSortByDuration(flights);
    }
    else {
        return errJson("Unknown sort key '" + by + "'. Valid options: price, time, duration");
    }

    return okJson(flightsToJson(flights));
}

// ── Handler: Book Flight ──────────────────────────────────────────────────────

std::string handleBookFlight(const std::string& json) {
    // Extract fields
    std::string flightID  = jsonGet(json, "flightID");
    std::string name      = jsonGet(json, "passengerName");
    std::string ageStr    = jsonGet(json, "passengerAge");
    std::string email     = jsonGet(json, "passengerEmail");
    std::string seatsStr  = jsonGet(json, "seats");

    // ── Input Validation ──────────────────────────────────────────────────────
    if (!isValidFlightID(flightID))
        return errJson("Invalid or missing Flight ID");

    if (!isNonEmpty(name))
        return errJson("Passenger name is required");
    if (!isSafeRecordField(name, 100))
        return errJson("Passenger name contains unsupported characters or is too long");

    int age = 0;
    if (!safeParseInt(ageStr, age) || !isValidAge(age))
        return errJson("Passenger age must be a number between 1 and 120");

    if (!isValidEmail(email))
        return errJson("Invalid email address format");

    int seats = 1;
    if (!seatsStr.empty() && (!safeParseInt(seatsStr, seats) || !isValidSeatCount(seats)))
        return errJson("Seat count must be between 1 and 10");

    // ── Business Logic ────────────────────────────────────────────────────────
    auto flights = loadFlights();

    // Find the flight using linear search (array is unsorted in this context)
    int flightIdx = -1;
    for (int i = 0; i < static_cast<int>(flights.size()); i++) {
        if (flights[i].flightID == flightID) {
            flightIdx = i;
            break;
        }
    }

    if (flightIdx == -1)
        return errJson("Flight '" + flightID + "' not found");

    if (flights[flightIdx].seatsAvailable < seats)
        return errJson("Only " + std::to_string(flights[flightIdx].seatsAvailable) +
                       " seat(s) available on this flight");

    auto bookings = loadBookings();
    Booking b;
    b.bookingID      = generateUniqueBookingID(bookings);
    b.passengerName  = name;
    b.passengerAge   = age;
    b.passengerEmail = email;
    b.flightID       = flightID;
    b.bookingDate    = todayDate();
    b.status         = "confirmed";
    b.seats          = seats;

    const auto originalFlights = flights;
    flights[flightIdx].seatsAvailable -= seats;
    saveFlights(flights);

    bookings.push_back(b);
    try {
        saveBookings(bookings);
    } catch (...) {
        saveFlights(originalFlights);
        throw;
    }

    return okJson(bookingToJson(b));
}

// ── Handler: Cancel Booking ───────────────────────────────────────────────────

std::string handleCancelBooking(const std::string& json) {
    std::string bookingID = jsonGet(json, "bookingID");

    if (!isNonEmpty(bookingID))
        return errJson("Booking ID is required");

    auto bookings = loadBookings();

    // Find the booking by linear search and reject ambiguous duplicate IDs.
    int idx = -1;
    int matches = 0;
    for (int i = 0; i < static_cast<int>(bookings.size()); i++) {
        if (bookings[i].bookingID == bookingID) {
            idx = i;
            matches++;
        }
    }

    if (idx == -1)
        return errJson("Booking '" + bookingID + "' not found");

    if (matches > 1)
        return errJson("Duplicate booking IDs exist; cancellation is ambiguous");

    if (bookings[idx].status != "confirmed")
        return errJson("Booking '" + bookingID + "' is not in confirmed status");

    auto flights = loadFlights();
    int flightIdx = -1;
    for (int i = 0; i < static_cast<int>(flights.size()); i++)
        if (flights[i].flightID == bookings[idx].flightID) { flightIdx = i; break; }
    if (flightIdx == -1)
        return errJson("Cannot cancel booking because its flight no longer exists");
    if (bookings[idx].seats < 1 ||
        flights[flightIdx].seatsAvailable > std::numeric_limits<int>::max() - bookings[idx].seats)
        return errJson("Cannot restore seats because the seat count is invalid or would overflow");

    const auto originalFlights = flights;
    flights[flightIdx].seatsAvailable += bookings[idx].seats;
    bookings[idx].status = "cancelled";
    saveFlights(flights);
    try {
        saveBookings(bookings);
    } catch (...) {
        saveFlights(originalFlights);
        throw;
    }

    return okJson("{\"bookingID\":\"" + bookingID + "\",\"message\":\"Booking cancelled successfully\"}");
}

// ── Handler: Get Bookings ─────────────────────────────────────────────────────

std::string handleGetBookings(const std::string& json) {
    std::string email = jsonGet(json, "email");

    auto bookings = loadBookings();
    std::vector<Booking> result;

    for (const auto& b : bookings) {
        // If email is provided, filter by it; otherwise return all bookings
        if (email.empty() || b.passengerEmail == email)
            result.push_back(b);
    }

    return okJson(bookingsToJson(result));
}

// ── Handler: Add Flight (Admin) ───────────────────────────────────────────────

std::string handleAddFlight(const std::string& json) {
    // Extract all fields
    std::string flightID      = jsonGet(json, "flightID");
    std::string airline       = jsonGet(json, "airline");
    std::string source        = jsonGet(json, "source");
    std::string destination   = jsonGet(json, "destination");
    std::string flightDate    = jsonGet(json, "flightDate");
    std::string departureTime = jsonGet(json, "departureTime");
    std::string arrivalTime   = jsonGet(json, "arrivalTime");
    std::string durStr        = jsonGet(json, "durationMin");
    std::string priceStr      = jsonGet(json, "price");
    std::string seatsStr      = jsonGet(json, "seatsAvailable");

    // ── Input Validation ──────────────────────────────────────────────────────
    if (!isValidFlightID(flightID))
        return errJson("Invalid Flight ID. Use alphanumeric characters only (e.g. FL011)");

    if (!isNonEmpty(airline))
        return errJson("Airline name is required");
    if (!isSafeRecordField(airline, 60))
        return errJson("Airline name contains unsupported characters or is too long");

    if (!isNonEmpty(source))
        return errJson("Source city is required");
    if (!isSafeRecordField(source, 60))
        return errJson("Source city contains unsupported characters or is too long");

    if (!isNonEmpty(destination))
        return errJson("Destination city is required");
    if (!isSafeRecordField(destination, 60))
        return errJson("Destination contains unsupported characters or is too long");

    if (toLower(source) == toLower(destination))
        return errJson("Source and destination cannot be the same city");

    if (!isValidDate(flightDate))
        return errJson("Invalid flight date. Use YYYY-MM-DD format");

    if (!isValidTime(departureTime))
        return errJson("Invalid departure time. Use HH:MM (24-hour) format");

    if (!isValidTime(arrivalTime))
        return errJson("Invalid arrival time. Use HH:MM (24-hour) format");

    int dur = 0;
    if (!safeParseInt(durStr, dur) || !isValidDuration(dur))
        return errJson("Duration must be a positive number of minutes");

    double price = 0.0;
    if (!safeParseDbl(priceStr, price) || !isValidPrice(price))
        return errJson("Price must be a positive number");

    int seats = 0;
    if (!safeParseInt(seatsStr, seats) || seats < 1)
        return errJson("Seats available must be at least 1");

    // ── Business Logic ────────────────────────────────────────────────────────
    auto flights = loadFlights();

    // Check for duplicate Flight ID
    for (const auto& f : flights)
        if (f.flightID == flightID)
            return errJson("Flight ID '" + flightID + "' already exists");

    Flight f;
    f.flightID       = flightID;
    f.airline        = airline;
    f.source         = source;
    f.destination    = destination;
    f.flightDate     = flightDate;
    f.departureTime  = departureTime;
    f.arrivalTime    = arrivalTime;
    f.durationMin    = dur;
    f.price          = price;
    f.seatsAvailable = seats;

    flights.push_back(f);
    saveFlights(flights);

    return okJson(flightToJson(f));
}

// ── Handler: Update Flight (Admin) ────────────────────────────────────────────

std::string handleUpdateFlight(const std::string& json) {
    std::string flightID = jsonGet(json, "flightID");

    if (!isValidFlightID(flightID))
        return errJson("Invalid or missing Flight ID");

    auto flights = loadFlights();

    // Find the flight to update
    int idx = -1;
    for (int i = 0; i < static_cast<int>(flights.size()); i++) {
        if (flights[i].flightID == flightID) {
            idx = i;
            break;
        }
    }

    if (idx == -1)
        return errJson("Flight '" + flightID + "' not found");

    Flight& f = flights[idx];

    // Update only the fields that are present in the request (partial update)
    std::string v;

    v = jsonGet(json, "airline");
    if (isNonEmpty(v)) {
        if (!isSafeRecordField(v, 60)) return errJson("Airline name contains unsupported characters or is too long");
        f.airline = v;
    }

    v = jsonGet(json, "source");
    if (isNonEmpty(v)) {
        if (!isSafeRecordField(v, 60)) return errJson("Source city contains unsupported characters or is too long");
        f.source = v;
    }

    v = jsonGet(json, "destination");
    if (isNonEmpty(v)) {
        if (!isSafeRecordField(v, 60)) return errJson("Destination contains unsupported characters or is too long");
        if (toLower(v) == toLower(f.source))
            return errJson("Source and destination cannot be the same city");
        f.destination = v;
    }
    if (toLower(f.source) == toLower(f.destination))
        return errJson("Source and destination cannot be the same city");

    v = jsonGet(json, "flightDate");
    if (isNonEmpty(v)) {
        if (!isValidDate(v)) return errJson("Invalid flight date. Use YYYY-MM-DD format");
        f.flightDate = v;
    }

    v = jsonGet(json, "departureTime");
    if (isNonEmpty(v)) {
        if (!isValidTime(v)) return errJson("Invalid departure time. Use HH:MM format");
        f.departureTime = v;
    }

    v = jsonGet(json, "arrivalTime");
    if (isNonEmpty(v)) {
        if (!isValidTime(v)) return errJson("Invalid arrival time. Use HH:MM format");
        f.arrivalTime = v;
    }

    v = jsonGet(json, "durationMin");
    if (isNonEmpty(v)) {
        int dur = 0;
        if (!safeParseInt(v, dur) || !isValidDuration(dur))
            return errJson("Duration must be a positive number of minutes");
        f.durationMin = dur;
    }

    v = jsonGet(json, "price");
    if (isNonEmpty(v)) {
        double price = 0.0;
        if (!safeParseDbl(v, price) || !isValidPrice(price))
            return errJson("Price must be a positive number");
        f.price = price;
    }

    v = jsonGet(json, "seatsAvailable");
    if (isNonEmpty(v)) {
        int seats = 0;
        if (!safeParseInt(v, seats) || seats < 0)
            return errJson("Seats available cannot be negative");
        if (seats != f.seatsAvailable) {
            for (const auto& booking : loadBookings())
                if (booking.flightID == flightID && booking.status == "confirmed")
                    return errJson("Cannot change available seats while confirmed bookings exist");
        }
        f.seatsAvailable = seats;
    }

    saveFlights(flights);
    return okJson(flightToJson(f));
}

// ── Handler: Delete Flight (Admin) ────────────────────────────────────────────

std::string handleDeleteFlight(const std::string& json) {
    std::string flightID = jsonGet(json, "flightID");

    if (!isValidFlightID(flightID))
        return errJson("Invalid or missing Flight ID");

    auto flights = loadFlights();

    // Find and remove the flight
    int idx = -1;
    for (int i = 0; i < static_cast<int>(flights.size()); i++) {
        if (flights[i].flightID == flightID) {
            idx = i;
            break;
        }
    }

    if (idx == -1)
        return errJson("Flight '" + flightID + "' not found");

    for (const auto& booking : loadBookings())
        if (booking.flightID == flightID && booking.status == "confirmed")
            return errJson("Cannot delete a flight with confirmed bookings");

    // Erase by shifting elements left (manual removal, no STL erase-remove)
    for (int i = idx; i < static_cast<int>(flights.size()) - 1; i++)
        flights[i] = flights[i + 1];
    flights.pop_back();

    saveFlights(flights);
    return okJson("{\"flightID\":\"" + flightID + "\",\"message\":\"Flight deleted successfully\"}");
}

// ── Entry Point ───────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << errJson("No request payload provided. Pass a JSON string as the first argument.") << std::endl;
        return 1;
    }

    std::string json   = argv[1];
    std::string action = jsonGet(json, "action");

    if (action.empty()) {
        std::cout << errJson("Missing 'action' field in request JSON") << std::endl;
        return 1;
    }

    std::string response;

    try {
        if      (action == "getFlights")    response = handleGetFlights();
        else if (action == "searchFlight")  response = handleSearchFlight(json);
        else if (action == "searchFlights") response = handleSearchFlights(json);
        else if (action == "sortFlights")   response = handleSortFlights(json);
        else if (action == "bookFlight")    response = handleBookFlight(json);
        else if (action == "cancelBooking") response = handleCancelBooking(json);
        else if (action == "getBookings")   response = handleGetBookings(json);
        else if (action == "addFlight")     response = handleAddFlight(json);
        else if (action == "updateFlight")  response = handleUpdateFlight(json);
        else if (action == "deleteFlight")  response = handleDeleteFlight(json);
        else
            response = errJson("Unknown action '" + action + "'");
    }
    catch (const std::exception& e) {
        // Catch any unexpected runtime errors (e.g. file I/O failures)
        response = errJson(std::string("Internal error: ") + e.what());
    }

    std::cout << response << std::endl;
    return 0;
}
