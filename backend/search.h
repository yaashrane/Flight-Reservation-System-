/*
 * search.h
 * --------
 * Implements two searching algorithms on a vector of Flight records.
 *
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │  Algorithm        │ Search Field          │ Time Complexity             │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │  Linear Search    │ source, destination,  │ O(n) — scans every element  │
 * │                   │ airline, date         │                             │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │  Binary Search    │ flightID              │ O(log n) — requires sorted  │
 * │                   │                       │ input; preceded by manual   │
 * │                   │                       │ Insertion Sort O(n²)        │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * Note: No STL sorting functions (std::sort, std::stable_sort) are used.
 *       The pre-sort for Binary Search is done with a manual Insertion Sort.
 */

#pragma once
#include <vector>
#include <string>
#include <algorithm>   // only for std::transform (toLower) — NOT for sorting
#include <cctype>
#include "flight.h"

// ── Helper: Case-Insensitive Conversion ──────────────────────────────────────

inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

// ── Algorithm 1: Linear Search ────────────────────────────────────────────────
/*
 * LINEAR SEARCH
 * -------------
 * Approach  : Iterate through every element in the array and compare the
 *             target field against the search value.
 *
 * Time Complexity  : O(n) — must examine every record in the worst case.
 * Space Complexity : O(k) — where k is the number of matching results.
 *
 * Why used here    : The flights array is unsorted for general field searches
 *                    (source, destination, airline, date), so Binary Search
 *                    cannot be applied. Linear Search is the correct choice.
 *
 * Supported keys   : "source", "destination", "airline", "date"
 *                    Also supports "source_destination" for combined search.
 *
 * Matching         : Case-insensitive substring match — allows partial input
 *                    (e.g. searching "del" matches "Delhi").
 */
inline std::vector<Flight> linearSearch(const std::vector<Flight>& flights,
                                        const std::string& key,
                                        const std::string& value) {
    std::vector<Flight> result;
    std::string val = toLower(value);

    for (const auto& f : flights) {
        // Step 1: Determine which field to compare based on the search key
        std::string field;

        if (key == "source") {
            field = toLower(f.source);
        }
        else if (key == "destination") {
            field = toLower(f.destination);
        }
        else if (key == "airline") {
            field = toLower(f.airline);
        }
        else if (key == "date") {
            field = f.flightDate;   // date comparison is already case-neutral
        }
        else if (key == "source_destination") {
            // Combined search: value must be "Source,Destination"
            // e.g. value = "delhi,mumbai"
            size_t comma = val.find(',');
            if (comma == std::string::npos) continue;
            std::string srcVal  = val.substr(0, comma);
            std::string dstVal  = val.substr(comma + 1);
            auto trim = [](std::string value) {
                size_t start = value.find_first_not_of(" \t\r\n");
                if (start == std::string::npos) return std::string();
                size_t end = value.find_last_not_of(" \t\r\n");
                return value.substr(start, end - start + 1);
            };
            srcVal = trim(srcVal);
            dstVal = trim(dstVal);
            if (srcVal.empty() || dstVal.empty()) continue;

            bool srcMatch = toLower(f.source).find(srcVal) != std::string::npos;
            bool dstMatch = toLower(f.destination).find(dstVal) != std::string::npos;
            if (srcMatch && dstMatch) result.push_back(f);
            continue;   // skip the generic field.find() below
        }
        else {
            continue;   // unknown key — skip this record
        }

        // Step 2: Substring match (partial search support)
        if (field.find(val) != std::string::npos)
            result.push_back(f);
    }

    return result;
}

// ── Pre-sort Helper: Manual Insertion Sort by Flight ID ───────────────────────
/*
 * INSERTION SORT (used as pre-sort for Binary Search)
 * ----------------------------------------------------
 * Approach  : Build a sorted sub-array one element at a time. For each
 *             element, shift all larger elements one position right, then
 *             insert the current element in its correct position.
 *
 * Time Complexity  : O(n²) worst/average — acceptable here because:
 *                    (a) n is small (flight catalog), and
 *                    (b) this is a one-time pre-sort before a single lookup.
 * Space Complexity : O(1) — in-place, no auxiliary array needed.
 *
 * Sorts by flightID (lexicographic) to satisfy Binary Search precondition.
 */
inline void insertionSortByID(std::vector<Flight>& flights) {
    int n = static_cast<int>(flights.size());
    for (int i = 1; i < n; i++) {
        Flight key = flights[i];   // element to be placed in sorted position
        int j = i - 1;
        // Shift elements that are greater than key one position to the right
        while (j >= 0 && flights[j].flightID > key.flightID) {
            flights[j + 1] = flights[j];
            j--;
        }
        flights[j + 1] = key;   // insert key at its correct sorted position
    }
}

// ── Algorithm 2: Binary Search ────────────────────────────────────────────────
/*
 * BINARY SEARCH
 * -------------
 * Approach  : On a sorted array, repeatedly halve the search space.
 *             Compare the middle element with the target:
 *               - Equal   → found, return index
 *               - Target > middle → search right half
 *               - Target < middle → search left half
 *             Repeat until found or search space is empty.
 *
 * Time Complexity  : O(log n) — search space halves each iteration.
 * Space Complexity : O(1) — iterative implementation, no recursion stack.
 *
 * Precondition     : The flights vector MUST be sorted by flightID.
 *                    This is guaranteed by calling insertionSortByID() first.
 *
 * Returns          : Index of the matching flight, or -1 if not found.
 */
inline int binarySearchByID(const std::vector<Flight>& flights, const std::string& id) {
    int lo = 0;
    int hi = static_cast<int>(flights.size()) - 1;

    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;   // avoids integer overflow vs (lo+hi)/2

        if (flights[mid].flightID == id)
            return mid;                  // found — return index

        if (flights[mid].flightID < id)
            lo = mid + 1;               // target is in the right half
        else
            hi = mid - 1;               // target is in the left half
    }

    return -1;   // not found
}

/*
 * Public interface for Flight ID search.
 * Sorts a copy of the flights vector by ID (Insertion Sort),
 * then applies Binary Search on the sorted copy.
 *
 * Returns a vector containing the matching flight, or an empty vector.
 */
inline std::vector<Flight> searchByID(std::vector<Flight> flights, const std::string& id) {
    if (id.empty()) return {};

    // Pre-sort using manual Insertion Sort (no STL sort used)
    insertionSortByID(flights);

    int idx = binarySearchByID(flights, id);
    if (idx == -1) return {};
    return { flights[idx] };
}
