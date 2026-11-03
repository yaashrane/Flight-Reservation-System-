# Skylane Flight Management and Reservation System

A local flight booking and operations application with a responsive HTML/CSS/JavaScript frontend, a Python standard-library HTTP bridge, and a C++17 backend. Flight and booking records are stored in pipe-delimited text files under `data/`.

## Features

- Search actual stored flights by source, destination, route, airline, flight ID, and date.
- Use city autocomplete and destination cards generated from the current flight schedule.
- Remove individual search filters, clear all criteria, and get helpful no-results guidance.
- Flight ID lookup uses binary search; route and text searches use linear search.
- Sort the matching flight records by price, departure date/time, or duration using the C++ sorting implementations.
- See a booking progress indicator, live seat availability, and a fare total that updates with seat count.
- Book one or more seats, check inventory, and persist the booking; view the saved flight and booking reference on confirmation.
- Cancel confirmed bookings, persist the status change, and restore seats.
- Filter booking history by status and view full route and schedule details.
- Administer flights with add, update, and delete operations. Flights with confirmed bookings cannot be deleted, and their available-seat count cannot be manually changed until those bookings are cancelled.
- Review live booking-status and seat-inventory charts; search and sort the admin flight schedule.
- Use the responsive navigation, loading skeletons, and keyboard-accessible controls across screen sizes.
- Serve the frontend and API on one local origin using Python's built-in HTTP server.
- Validate request data, escape JSON output, and atomically replace each individual data file on save.

## Requirements

- Windows 10/11
- Python 3.8 or later
- A C++17 compiler available as `g++` in `PATH` (for example, MinGW-w64)
- A modern browser

No Python packages or frontend build tools are required. The UI loads DM Sans and Manrope fonts and destination photography from Google Fonts and Unsplash when an internet connection is available; a local SVG illustration keeps the landing hero attractive if the remote flight photo is unavailable. The application and its data remain local.

## Project layout

```text
backend/       C++ records, algorithms, file handlers, and request dispatcher
data/          Persistent flights.txt and bookings.txt
frontend/      HTML pages, CSS, and browser JavaScript
server/        Python local HTTP server and C++ process bridge
tests/         Standard-library integration and behavior tests
compile.bat    Compile the C++ backend
start.bat      Compile, launch the local server, and open the UI
```

## Setup and execution

1. Install Python and a C++17 `g++` compiler. Confirm both commands are available in Command Prompt or PowerShell:

   ```powershell
   python --version
   g++ --version
   ```

2. From the project root, compile the backend:

   ```powershell
   .\compile.bat
   ```

   Or compile directly:

   ```powershell
   g++ -std=c++17 -O2 -Wall -Wextra -o backend\main.exe backend\main.cpp
   ```

3. Start the application from the project root:

   ```powershell
   .\start.bat
   ```

   `start.bat` recompiles the C++ program, starts the HTTP server, and opens `http://127.0.0.1:8080`. To start only the server after compiling, run:

   ```powershell
   python server\server.py
   ```

4. Use **Discover**, **Flights**, **Book**, **My bookings**, and **Admin** from the site navigation. Stop the server with Ctrl+C in its console.

The server binds to the loopback interface and is intended for local development/demo use. The admin page is not authenticated; do not expose this service to an untrusted network.

## Data files

`data/flights.txt` has one record per line:

```text
flightID|airline|source|destination|flightDate|departureTime|arrivalTime|durationMin|price|seatsAvailable
```

`data/bookings.txt` has one record per line:

```text
bookingID|passengerName|passengerAge|passengerEmail|flightID|bookingDate|status|seats
```

Dates use `YYYY-MM-DD`; times use 24-hour `HH:MM`. Data files are read and written relative to the backend working directory. The HTTP bridge always invokes the backend with `backend/` as its working directory. Save operations write a temporary sibling file and replace the target after a successful flush, protecting each file from partial writes. Booking/cancellation operations also roll back the flight file if the corresponding booking-file save fails. Back up `data/` before manual edits or resets.

## HTTP / C++ integration

The Python server serves the frontend and forwards requests to `backend/main.exe`. The C++ process validates and processes the action, reads or updates the text data files, and returns JSON. The browser does not contain a hardcoded flight catalog.

| Method | Action | Purpose |
|---|---|---|
| GET | `getFlights` | Return persisted flights |
| GET | `searchFlight` | Search by `key` and `value` (including binary `flightID` search) |
| GET | `searchFlights` | Filter by optional `source`, `destination`, `date`; optionally set `sortBy` |
| GET | `sortFlights` | Sort by `price`, `time`, or `duration` |
| GET | `getBookings` | List bookings; optional exact `email` filter |
| POST | `bookFlight` | Create booking, persist it, and decrement seats |
| POST | `cancelBooking` | Cancel a confirmed booking and restore seats |
| POST | `addFlight` | Add a validated flight |
| POST | `updateFlight` | Update a flight by ID |
| POST | `deleteFlight` | Delete a flight if it has no confirmed bookings |

Example requests:

```text
GET http://127.0.0.1:8080/?action=searchFlight&key=source&value=del
GET http://127.0.0.1:8080/?action=searchFlight&key=flightID&value=FL001
GET http://127.0.0.1:8080/?action=searchFlights&source=del&destination=mumb&sortBy=price
```

POST requests send a JSON object with the `action` and operation fields. Application-level validation errors return `{"status":"error","message":"..."}`; successful responses return `{"status":"ok","data":...}`.

## Data structures and algorithms

### Searching

- **Linear search** compares each flight's source, destination, airline, or date. Text matches are case-insensitive substring matches; the combined route search requires both source and destination to match.
- **Binary search** locates a flight by ID. The backend first makes a copy of the records, insertion-sorts that copy by flight ID, and then performs binary search. The binary-search step is logarithmic; the complete lookup includes the insertion-sort pre-processing.

### Sorting

- **Bubble sort** orders ticket prices ascending and exits early when a pass makes no swaps.
- **Selection sort** orders by flight date and then departure time.
- **Merge sort** orders flight duration ascending and preserves relative order for equal durations.

### Complexity

Let `n` be the number of flight records and `k` the number of matches.

| Algorithm / operation | Best time | Average time | Worst time | Additional space |
|---|---:|---:|---:|---:|
| Linear search | O(1) | O(n) | O(n) | O(k) results |
| Binary search step | O(1) | O(log n) | O(log n) | O(1) |
| Flight ID search, including insertion-sort pre-pass | O(n) | O(n²) | O(n²) | O(n) copy |
| Bubble sort by price | O(n) | O(n²) | O(n²) | O(1) |
| Selection sort by departure | O(n²) | O(n²) | O(n²) | O(1) |
| Merge sort by duration | O(n log n) | O(n log n) | O(n log n) | O(n) |
| Booking lookup / cancellation lookup | O(n) | O(n) | O(n) | O(1) beyond loaded records |

Flight ID search has an O(log n) binary-search phase, but total request time is O(n²) in the average/worst case because the current implementation insertion-sorts a copy on each lookup.

## Validation and edge cases

- Flight IDs are alphanumeric and limited to 20 characters; duplicate IDs cannot be added.
- Passenger names and airline/city fields are length-limited and reject pipe delimiters and unsafe control/escape characters.
- Dates must be real calendar dates; times must be valid 24-hour times.
- Numeric fields must parse completely; non-finite prices and values with trailing text are rejected.
- Passenger age must be 1–120, email must match the supported format, and booking seat count must be 1–10.
- Booking more seats than available is rejected without changing either data file.
- Only a unique, confirmed booking can be cancelled. Missing, already-cancelled, ambiguous duplicate-ID, or orphaned-flight cancellations are rejected without changing the booking status.
- Booking IDs are checked against existing records and receive a suffix when a timestamp ID already exists.
- Deleting a flight with confirmed bookings or manually changing its seat count while confirmed bookings exist is rejected.

## Tests

The test suite uses only Python's standard library. It compiles the C++ backend into a temporary directory, copies the fixture data, and runs all mutations against those temporary files. It does not modify the project's `data/` files.

Run the complete suite from the project root:

```powershell
python -m unittest discover -s tests -v
```

The test cases and expected outcomes are:

| Test | Expected result |
|---|---|
| Linear search across source, destination, airline, date, and route | Exact matching flight IDs, case-insensitively |
| Binary search on reversed/unsorted records | Existing ID found; missing ID returns an empty result |
| Bubble, selection, and merge sorts | Correct ascending order; no records lost |
| Search filters combined with sorting | Only matching records returned, in requested order |
| Valid booking and repeat booking in one timestamp interval | Booking lines saved, unique IDs, seats decremented |
| Overbooking, zero, over-limit, decimal, and malformed seat counts | Error response; inventory and booking file unchanged |
| Invalid age, email, and passenger record delimiter | Error response; Unicode passenger name round-trips |
| Cancellation, repeated cancellation, and unknown ID | Confirmed booking cancels and seats restore once; invalid cases do not mutate |
| Duplicate booking IDs and missing referenced flight | Ambiguous/orphan cancellation rejected; booking stays confirmed |
| Admin add/update/delete and duplicate ID | Changes persist; active booking blocks seat edit and deletion |
| Invalid dates, times, price, duration, search key, and sort key | Validation error; existing records remain intact |
| Backend process restart | Previously saved flights and bookings remain readable |
| Failed temporary-file write | Error reported; original data file remains intact |
| Booking/cancellation data-file write failure | Seat inventory is rolled back; booking remains unchanged |
| Malformed flight data line | Line is logged and skipped; remaining valid records load |
| HTTP integration | Frontend is served, real C++ search returns records, booking saves and decrements seats |

The DSA Lab was also smoke-tested in a browser against 10 live backend records: all five traces rendered, search results and sort orders matched the records, invalid blank search input was rejected, and no horizontal page overflow appeared at a 390 px viewport width.

## Troubleshooting

- **`Backend not compiled`**: run `.\compile.bat` from the project root and confirm `g++` is on `PATH`.
- **Port 8080 is already in use**: stop the other local service or set a different port before launching:

  ```powershell
  $env:FLIGHTMS_PORT = "8081"
  python server\server.py
  ```

- **No flights appear**: check that `data\flights.txt` exists and that every non-empty line has ten pipe-delimited fields.
- **Changes do not appear**: use the app through the server URL, not by opening an HTML file directly. Confirm the UI and API are on the same origin.
- **A data write fails**: check that the project `data\` directory is writable and that no `flights.txt.tmp` / `bookings.txt.tmp` file is locked by another process.

## DSA Lab and project report

Open **DSA Lab** in the application navigation for interactive demonstrations using the current flight records returned by the C++ backend. Select Linear Search, Binary Search, Bubble Sort, Selection Sort, or Merge Sort, then run the trace to see its input, intermediate steps, final output, and complexity. The trace is an educational visualization of the matching algorithm; actual flight search and sorting operations in the booking pages execute in C++.

### Abstract

The Flight Management and Reservation System is a local web application for searching flights, booking seats, managing reservations, and maintaining a flight schedule. It combines an HTML, CSS, and JavaScript frontend with a Python HTTP bridge and a C++17 backend. Flights and bookings persist in text files. Linear search supports flexible route and airline matching, binary search demonstrates lookup on sorted flight IDs, and bubble, selection, and merge sort order flights by useful travel criteria. Booking and cancellation update saved seat inventory, connecting DSA concepts to practical workflows.

### Problem statement

Flight reservation tasks require locating suitable flights, comparing schedules, tracking seat availability, and keeping booking history. The project implements these tasks in a small local application while demonstrating how search and sorting algorithms work on flight records. It must also validate requests and preserve consistent flight and booking data when seats are booked or cancelled.

### Objectives

- Model flights and reservations as structured records.
- Implement linear and binary flight search and three sorting algorithms.
- Connect algorithm operations to flight search, booking, and admin workflows.
- Persist flight and booking records across application restarts.
- Validate user input and protect seat inventory from invalid updates.
- Make algorithm input, execution, output, and complexity easy to demonstrate.

### Proposed methodology

1. Load pipe-delimited flight or booking records from the data files into C++ vectors.
2. Receive browser requests through the Python standard-library HTTP server.
3. Validate request fields in C++ and run the relevant search or sort.
4. For bookings and cancellations, update inventory and booking status, then save the files.
5. Return JSON to the browser and display the result or a validation error.
6. Exercise normal, boundary, invalid, persistence, and integration cases with tests using isolated copies of the data.

### Hardware and software requirements

| Category | Requirement |
|---|---|
| Hardware | A standard laptop or desktop; a dual-core processor, 4 GB RAM, and a few hundred MB of free disk space are sufficient for this small local demo. |
| Operating system | Windows 10 or later (the supplied launcher scripts use Windows batch files). |
| Backend | Python 3.8 or later and a C++17 compiler such as MinGW-w64 `g++`, available on `PATH`. |
| Frontend | A modern browser; HTML, CSS, and browser JavaScript require no build step. |
| Dependencies | Python standard library only; no third-party Python packages are needed. |

### DSA concepts used

- **Vector:** Dynamic array used to hold records in memory.
- **Linear search:** Examines records in order for case-insensitive partial field matches.
- **Insertion sort:** Orders a copy of the flight list by ID before binary search.
- **Binary search:** Repeatedly halves the sorted ID range to locate a flight.
- **Bubble sort:** Repeatedly swaps adjacent out-of-order prices, with an early-exit check.
- **Selection sort:** Finds the earliest date and departure time in the remaining records.
- **Merge sort:** Recursively divides records and merges them by ascending flight duration.
- **File handling:** Reads and writes records so application data persists after exit.

### Advantages and limitations

**Advantages:** The project is locally runnable, provides a clear visual algorithm walkthrough, uses real persisted records, validates inputs, and demonstrates seat-inventory updates in a practical context.

**Limitations:** Plain text files and a single-user local server are suitable for a classroom demonstration, not concurrent production traffic. Binary ID lookup currently includes insertion-sort preparation on every request. The admin screen is unauthenticated, and the application does not provide production airline booking, payment, or deployment features.

### Future scope

- Replace text files with a database and transactional inventory updates.
- Add authentication and role-based authorization for administrative actions.
- Maintain an indexed ID lookup to avoid sorting a copy for every binary search.
- Add richer passenger/ticket details, filters, and fare comparison.
- Expand automated browser, accessibility, and concurrency testing.

### Conclusion

This project applies core searching and sorting techniques to flight records and connects them to a persistent reservation workflow. The DSA Lab presents the algorithm inputs, intermediate execution, outputs, and complexity, while the rest of the application demonstrates how flight search, booking, cancellation, and schedule administration use stored data. It is an educational prototype and a starting point for future improvements, rather than a production airline service.
