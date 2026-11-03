/*
 * sort.h
 * ------
 * Implements three sorting algorithms on a vector of Flight records.
 * No STL sorting functions (std::sort, std::stable_sort) are used.
 *
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │  Algorithm       │ Sort Field       │ Time Complexity │ Space Complexity │
 * ├──────────────────────────────────────────────────────────────────────────┤
 * │  Bubble Sort     │ Ticket Price     │ O(n²)           │ O(1)             │
 * │  Selection Sort  │ Departure Time   │ O(n²)           │ O(1)             │
 * │  Merge Sort      │ Flight Duration  │ O(n log n)      │ O(n)             │
 * └──────────────────────────────────────────────────────────────────────────┘
 */

#pragma once
#include <vector>
#include <string>
#include "flight.h"

// ── Algorithm 1: Bubble Sort — by Ticket Price ───────────────────────────────
/*
 * BUBBLE SORT
 * -----------
 * Approach  : Make repeated passes through the array. On each pass, compare
 *             adjacent elements and swap them if they are in the wrong order.
 *             After each pass, the largest unsorted element "bubbles up" to
 *             its correct position at the end of the unsorted region.
 *
 * Optimization: An `earlyExit` flag detects when no swaps occurred in a full
 *               pass, meaning the array is already sorted — exits early.
 *               This improves best-case performance to O(n).
 *
 * Time Complexity
 *   Best    : O(n)   — array already sorted (early exit triggers)
 *   Average : O(n²)  — random order
 *   Worst   : O(n²)  — array sorted in reverse order
 *
 * Space Complexity : O(1) — in-place, only a temp variable for swapping.
 *
 * Stability : Stable — equal-price flights retain their original order
 *             because we use strict > (not >=) for the swap condition.
 *
 * Sort order : Ascending by price (cheapest first).
 */
inline void bubbleSortByPrice(std::vector<Flight>& flights) {
    int n = static_cast<int>(flights.size());

    for (int i = 0; i < n - 1; i++) {
        bool earlyExit = true;   // assume sorted; set false if any swap occurs

        // Inner pass: compare adjacent pairs in the unsorted region [0 .. n-i-1]
        for (int j = 0; j < n - i - 1; j++) {
            if (flights[j].price > flights[j + 1].price) {
                // Swap: the larger element moves one step to the right
                Flight temp    = flights[j];
                flights[j]     = flights[j + 1];
                flights[j + 1] = temp;
                earlyExit = false;
            }
        }

        // If no swaps happened in this pass, the array is fully sorted
        if (earlyExit) break;
    }
}

// ── Algorithm 2: Selection Sort — by Departure Time ──────────────────────────
/*
 * SELECTION SORT
 * --------------
 * Approach  : Divide the array into a sorted region (left) and an unsorted
 *             region (right). On each iteration, find the minimum element in
 *             the unsorted region and swap it with the first element of the
 *             unsorted region, growing the sorted region by one.
 *
 * Time Complexity
 *   Best    : O(n²)  — always performs n*(n-1)/2 comparisons regardless of input
 *   Average : O(n²)
 *   Worst   : O(n²)
 *
 * Space Complexity : O(1) — in-place, only index variables used.
 *
 * Stability : NOT stable — the swap can change the relative order of equal
 *             elements. Acceptable here since departure times are typically unique.
 *
 * Sort order : Ascending by departure time (earliest first).
 *              Departure time is stored as "HH:MM" (24-hour), so lexicographic
 *              string comparison correctly reflects chronological order.
 *              When times are equal, flightDate is used as a tiebreaker.
 */
inline void selectionSortByDepartureTime(std::vector<Flight>& flights) {
    int n = static_cast<int>(flights.size());

    for (int i = 0; i < n - 1; i++) {
        int minIdx = i;   // index of the minimum element in the unsorted region

        // Find the index of the earliest departure in flights[i+1 .. n-1]
        for (int j = i + 1; j < n; j++) {
            // Compare date first, then time (concatenation gives correct order)
            std::string minKey = flights[minIdx].flightDate + flights[minIdx].departureTime;
            std::string curKey = flights[j].flightDate      + flights[j].departureTime;
            if (curKey < minKey)
                minIdx = j;
        }

        // Swap the found minimum with the first element of the unsorted region
        if (minIdx != i) {
            Flight temp      = flights[i];
            flights[i]       = flights[minIdx];
            flights[minIdx]  = temp;
        }
    }
}

// ── Algorithm 3: Merge Sort — by Flight Duration ─────────────────────────────
/*
 * MERGE SORT — MERGE STEP
 * -----------------------
 * Merges two sorted sub-arrays flights[l..m] and flights[m+1..r] into a
 * single sorted sub-array in-place within flights[l..r].
 *
 * Approach  : Copy both halves into temporary arrays (left, right).
 *             Use two pointers to compare elements from each half and write
 *             the smaller one back into the original array. Append any
 *             remaining elements from whichever half is not exhausted.
 *
 * Time Complexity  : O(n) per merge call — each element is copied once.
 * Space Complexity : O(n) auxiliary — temporary left and right arrays.
 */
inline void mergeDuration(std::vector<Flight>& flights, int l, int m, int r) {
    // Copy left and right halves into temporary vectors
    std::vector<Flight> leftHalf(flights.begin() + l, flights.begin() + m + 1);
    std::vector<Flight> rightHalf(flights.begin() + m + 1, flights.begin() + r + 1);

    int i = 0;                          // pointer into leftHalf
    int j = 0;                          // pointer into rightHalf
    int k = l;                          // write position in original array

    // Merge: pick the smaller duration element from either half
    while (i < static_cast<int>(leftHalf.size()) &&
           j < static_cast<int>(rightHalf.size())) {
        if (leftHalf[i].durationMin <= rightHalf[j].durationMin)
            flights[k++] = leftHalf[i++];
        else
            flights[k++] = rightHalf[j++];
    }

    // Append any remaining elements from the left half
    while (i < static_cast<int>(leftHalf.size()))
        flights[k++] = leftHalf[i++];

    // Append any remaining elements from the right half
    while (j < static_cast<int>(rightHalf.size()))
        flights[k++] = rightHalf[j++];
}

/*
 * MERGE SORT — RECURSIVE DIVIDE STEP
 * ------------------------------------
 * Approach  : Divide the array in half recursively until sub-arrays of size 1
 *             are reached (base case — a single element is trivially sorted).
 *             Then merge the sorted halves back together bottom-up.
 *
 * Time Complexity
 *   Best    : O(n log n) — always divides into equal halves
 *   Average : O(n log n)
 *   Worst   : O(n log n) — consistent regardless of input order
 *
 * Space Complexity : O(n) — auxiliary arrays in merge + O(log n) call stack.
 *
 * Stability : Stable — the merge step uses <= so equal-duration flights
 *             retain their original relative order.
 *
 * Sort order : Ascending by durationMin (shortest flight first).
 */
inline void mergeSortDuration(std::vector<Flight>& flights, int l, int r) {
    if (l >= r) return;   // base case: sub-array of 0 or 1 element is sorted

    int m = l + (r - l) / 2;           // midpoint (avoids integer overflow)
    mergeSortDuration(flights, l, m);   // sort left half
    mergeSortDuration(flights, m + 1, r); // sort right half
    mergeDuration(flights, l, m, r);    // merge the two sorted halves
}

/*
 * Public interface for duration-based sort.
 * Calls the recursive Merge Sort on the full flights vector.
 */
inline void mergeSortByDuration(std::vector<Flight>& flights) {
    if (flights.size() > 1)
        mergeSortDuration(flights, 0, static_cast<int>(flights.size()) - 1);
}
