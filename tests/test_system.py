import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import unittest
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
COMPILER = shutil.which("g++")


@unittest.skipUnless(COMPILER, "g++ is required to compile the C++ backend")
class FlightSystemTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build_dir = Path(tempfile.mkdtemp(prefix="flightms-build-"))
        cls.binary = cls.build_dir / ("main.exe" if os.name == "nt" else "main")
        result = subprocess.run(
            [
                COMPILER,
                "-std=c++17",
                "-O2",
                "-Wall",
                "-Wextra",
                "-o",
                str(cls.binary),
                str(ROOT / "backend" / "main.cpp"),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=120,
        )
        if result.returncode:
            raise RuntimeError(f"C++ compilation failed:\n{result.stdout}\n{result.stderr}")

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.build_dir, ignore_errors=True)

    def setUp(self):
        self.temp_dir = Path(tempfile.mkdtemp(prefix="flightms-test-"))
        self.backend_dir = self.temp_dir / "backend"
        self.data_dir = self.temp_dir / "data"
        self.backend_dir.mkdir()
        self.data_dir.mkdir()
        self.binary_path = self.backend_dir / self.binary.name
        shutil.copy2(self.binary, self.binary_path)
        shutil.copy2(ROOT / "data" / "flights.txt", self.data_dir / "flights.txt")
        shutil.copy2(ROOT / "data" / "bookings.txt", self.data_dir / "bookings.txt")

    def tearDown(self):
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def run_backend(self, payload):
        process = subprocess.run(
            [str(self.binary_path), json.dumps(payload, ensure_ascii=True)],
            cwd=self.backend_dir,
            capture_output=True,
            text=True,
            encoding="utf-8",
            timeout=10,
        )
        self.assertTrue(process.stdout.strip(), process.stderr)
        return json.loads(process.stdout)

    def get_flights(self):
        return self.run_backend({"action": "getFlights"})["data"]

    def flight(self, flight_id):
        return next(flight for flight in self.get_flights() if flight["flightID"] == flight_id)

    def add_test_flight(self, flight_id="TEST100", seats=5):
        return self.run_backend(
            {
                "action": "addFlight",
                "flightID": flight_id,
                "airline": "Test Air",
                "source": "Test City",
                "destination": "Sample City",
                "flightDate": "2027-06-18",
                "departureTime": "06:30",
                "arrivalTime": "08:00",
                "durationMin": "90",
                "price": "3000",
                "seatsAvailable": str(seats),
            }
        )

    def book(self, flight_id="FL001", seats=1, **overrides):
        payload = {
            "action": "bookFlight",
            "flightID": flight_id,
            "passengerName": "Test Passenger",
            "passengerAge": "34",
            "passengerEmail": "test@example.com",
            "seats": str(seats),
        }
        payload.update(overrides)
        return self.run_backend(payload)

    def test_linear_search_by_fields_and_partial_route(self):
        tests = [
            ({"key": "source", "value": "del"}, ["FL001", "FL003", "FL009"]),
            ({"key": "destination", "value": "mumb"}, ["FL001", "FL007"]),
            ({"key": "airline", "value": "ind"}, ["FL001", "FL002", "FL004", "FL006", "FL008"]),
            ({"key": "date", "value": "2025-08-01"}, ["FL001", "FL002"]),
            ({"key": "source_destination", "value": "del, mum"}, ["FL001"]),
        ]
        for criteria, expected in tests:
            with self.subTest(criteria=criteria):
                result = self.run_backend({"action": "searchFlight", **criteria})
                self.assertEqual(result["status"], "ok")
                self.assertEqual([flight["flightID"] for flight in result["data"]], expected)

    def test_binary_search_on_unsorted_records_and_missing_id(self):
        path = self.data_dir / "flights.txt"
        path.write_text("\n".join(reversed(path.read_text().splitlines())) + "\n", encoding="utf-8")
        result = self.run_backend({"action": "searchFlight", "key": "flightID", "value": "FL004"})
        self.assertEqual([flight["flightID"] for flight in result["data"]], ["FL004"])
        missing = self.run_backend({"action": "searchFlight", "key": "flightID", "value": "FL999"})
        self.assertEqual(missing["data"], [])

    def test_all_three_sorts_return_correct_order_and_records(self):
        flights = self.get_flights()
        expected_ids = {flight["flightID"] for flight in flights}
        cases = [
            ("price", lambda flight: flight["price"]),
            ("time", lambda flight: flight["flightDate"] + flight["departureTime"]),
            ("duration", lambda flight: flight["durationMin"]),
        ]
        for key, sort_key in cases:
            with self.subTest(sort=key):
                result = self.run_backend({"action": "sortFlights", "by": key})
                self.assertEqual(result["status"], "ok")
                self.assertEqual([sort_key(flight) for flight in result["data"]],
                                 sorted(sort_key(flight) for flight in result["data"]))
                self.assertEqual({flight["flightID"] for flight in result["data"]}, expected_ids)

    def test_combined_search_and_sort_only_sorts_matching_records(self):
        result = self.run_backend(
            {
                "action": "searchFlights",
                "source": "del",
                "destination": "mumb",
                "date": "2025-08-01",
                "sortBy": "price",
            }
        )
        self.assertEqual([flight["flightID"] for flight in result["data"]], ["FL001"])
        invalid = self.run_backend({"action": "searchFlights", "date": "2025-02-29"})
        self.assertEqual(invalid["status"], "error")

    def test_booking_persists_unique_id_and_updates_seats(self):
        self.add_test_flight(seats=5)
        first = self.book("TEST100", seats=2)
        second = self.book("TEST100", seats=1)
        self.assertEqual(first["status"], "ok")
        self.assertEqual(second["status"], "ok")
        self.assertNotEqual(first["data"]["bookingID"], second["data"]["bookingID"])
        self.assertEqual(self.flight("TEST100")["seatsAvailable"], 2)
        records = (self.data_dir / "bookings.txt").read_text(encoding="utf-8").splitlines()
        self.assertEqual(len(records), 2)
        self.assertEqual(len({record.split("|", 1)[0] for record in records}), 2)

    def test_overbooking_and_invalid_seat_counts_leave_inventory_unchanged(self):
        self.add_test_flight(seats=4)
        for seat_count in (5, 0, 11, "2.5", "3x"):
            result = self.book("TEST100", seats=seat_count)
            self.assertEqual(result["status"], "error", seat_count)
            self.assertEqual(self.flight("TEST100")["seatsAvailable"], 4)
        self.assertEqual((self.data_dir / "bookings.txt").read_text(encoding="utf-8"), "")

    def test_booking_input_validation_and_unicode_round_trip(self):
        invalid = [
            {"passengerAge": "0"},
            {"passengerAge": "121"},
            {"passengerAge": "30years"},
            {"passengerEmail": "not-an-email"},
            {"passengerName": "Bad|Name"},
        ]
        for overrides in invalid:
            with self.subTest(overrides=overrides):
                self.assertEqual(self.book(**overrides)["status"], "error")
        result = self.book(passengerName="Zoë")
        self.assertEqual(result["status"], "ok")
        listing = self.run_backend({"action": "getBookings", "email": "test@example.com"})
        self.assertEqual(listing["data"][0]["passengerName"], "Zo" + chr(0x00EB))

    def test_cancel_restores_seats_and_repeat_or_unknown_cancellation_is_rejected(self):
        self.add_test_flight(seats=5)
        booking = self.book("TEST100", seats=3)["data"]
        self.assertEqual(self.flight("TEST100")["seatsAvailable"], 2)
        cancelled = self.run_backend({"action": "cancelBooking", "bookingID": booking["bookingID"]})
        self.assertEqual(cancelled["status"], "ok")
        self.assertEqual(self.flight("TEST100")["seatsAvailable"], 5)
        records = self.run_backend({"action": "getBookings", "email": "test@example.com"})["data"]
        self.assertEqual(records[0]["status"], "cancelled")
        self.assertEqual(self.run_backend({"action": "cancelBooking", "bookingID": booking["bookingID"]})["status"], "error")
        self.assertEqual(self.run_backend({"action": "cancelBooking", "bookingID": "BK-NOT-FOUND"})["status"], "error")
        self.assertEqual(self.flight("TEST100")["seatsAvailable"], 5)

    def test_ambiguous_duplicate_booking_ids_cannot_be_cancelled(self):
        self.add_test_flight(seats=5)
        self.book("TEST100", seats=2)
        records = (self.data_dir / "bookings.txt").read_text(encoding="utf-8").splitlines()
        (self.data_dir / "bookings.txt").write_text("\n".join([records[0], records[0]]) + "\n", encoding="utf-8")
        before = self.flight("TEST100")["seatsAvailable"]
        result = self.run_backend({"action": "cancelBooking", "bookingID": records[0].split("|")[0]})
        self.assertEqual(result["status"], "error")
        self.assertIn("Duplicate", result["message"])
        self.assertEqual(self.flight("TEST100")["seatsAvailable"], before)

    def test_orphan_booking_cannot_be_cancelled_or_marked_cancelled(self):
        booking = self.book()["data"]
        flight_lines = (self.data_dir / "flights.txt").read_text(encoding="utf-8").splitlines()
        (self.data_dir / "flights.txt").write_text(
            "\n".join(line for line in flight_lines if not line.startswith("FL001|")) + "\n",
            encoding="utf-8",
        )
        result = self.run_backend({"action": "cancelBooking", "bookingID": booking["bookingID"]})
        self.assertEqual(result["status"], "error")
        records = self.run_backend({"action": "getBookings", "email": "test@example.com"})["data"]
        self.assertEqual(records[0]["status"], "confirmed")

    def test_admin_add_update_and_delete_with_active_booking_guard(self):
        self.assertEqual(self.add_test_flight()["status"], "ok")
        self.assertEqual(self.add_test_flight()["status"], "error")
        self.assertEqual(self.run_backend({"action": "updateFlight", "flightID": "TEST100", "price": "4500"})["status"], "ok")
        self.assertEqual(self.flight("TEST100")["price"], 4500)
        booking = self.book("TEST100")["data"]
        self.assertEqual(self.run_backend({"action": "deleteFlight", "flightID": "TEST100"})["status"], "error")
        self.assertEqual(
            self.run_backend({"action": "updateFlight", "flightID": "TEST100", "seatsAvailable": "100"})["status"],
            "error",
        )
        self.assertEqual(self.run_backend({"action": "cancelBooking", "bookingID": booking["bookingID"]})["status"], "ok")
        self.assertEqual(self.run_backend({"action": "deleteFlight", "flightID": "TEST100"})["status"], "ok")
        self.assertFalse(any(flight["flightID"] == "TEST100" for flight in self.get_flights()))

    def test_invalid_flight_fields_duplicate_ids_and_search_keys(self):
        self.assertEqual(self.add_test_flight()["status"], "ok")
        invalid_adds = [
            {"flightID": "TEST101", "flightDate": "2025-99-99"},
            {"flightID": "TEST102", "price": "Infinity"},
            {"flightID": "TEST103", "durationMin": "90minutes"},
            {"flightID": "TEST104", "airline": "Pipe|Air"},
            {"flightID": "TEST105", "departureTime": "25:61"},
        ]
        valid = {
            "airline": "Test Air",
            "source": "Test City",
            "destination": "Sample City",
            "flightDate": "2027-06-18",
            "departureTime": "06:30",
            "arrivalTime": "08:00",
            "durationMin": "90",
            "price": "3000",
            "seatsAvailable": "5",
        }
        for bad in invalid_adds:
            with self.subTest(bad=bad):
                self.assertEqual(self.run_backend({"action": "addFlight", **valid, **bad})["status"], "error")
        self.assertEqual(self.run_backend({"action": "searchFlight", "key": "nonsense", "value": "x"})["status"], "error")
        self.assertEqual(self.run_backend({"action": "sortFlights", "by": "unknown"})["status"], "error")

    def test_persistence_survives_backend_process_restart(self):
        self.add_test_flight("PERSIST1")
        self.book("PERSIST1")
        self.assertTrue((self.data_dir / "flights.txt").exists())
        self.assertTrue((self.data_dir / "bookings.txt").read_text(encoding="utf-8").strip())
        self.assertTrue(any(flight["flightID"] == "PERSIST1" for flight in self.get_flights()))
        self.assertEqual(len(self.run_backend({"action": "getBookings"})["data"]), 1)

    def test_failed_atomic_write_does_not_truncate_original_data(self):
        flights_path = self.data_dir / "flights.txt"
        original = flights_path.read_text(encoding="utf-8")
        (self.data_dir / "flights.txt.tmp").mkdir()
        result = self.add_test_flight("WRITEFAIL")
        self.assertEqual(result["status"], "error")
        self.assertEqual(flights_path.read_text(encoding="utf-8"), original)
        self.assertFalse(any(flight["flightID"] == "WRITEFAIL" for flight in self.get_flights()))

    def test_booking_and_cancellation_roll_back_seats_when_booking_file_write_fails(self):
        self.add_test_flight("ROLLBACK", seats=5)
        (self.data_dir / "bookings.txt.tmp").mkdir()
        failed_booking = self.book("ROLLBACK", seats=2)
        self.assertEqual(failed_booking["status"], "error")
        self.assertEqual(self.flight("ROLLBACK")["seatsAvailable"], 5)
        self.assertEqual((self.data_dir / "bookings.txt").read_text(encoding="utf-8"), "")
        (self.data_dir / "bookings.txt.tmp").rmdir()

        booking = self.book("ROLLBACK", seats=2)["data"]
        (self.data_dir / "bookings.txt.tmp").mkdir()
        failed_cancel = self.run_backend({"action": "cancelBooking", "bookingID": booking["bookingID"]})
        self.assertEqual(failed_cancel["status"], "error")
        self.assertEqual(self.flight("ROLLBACK")["seatsAvailable"], 3)
        persisted = self.run_backend({"action": "getBookings", "email": "test@example.com"})["data"]
        self.assertEqual(persisted[0]["status"], "confirmed")

    def test_malformed_data_lines_are_skipped_without_losing_valid_records(self):
        flights_path = self.data_dir / "flights.txt"
        flights_path.write_text("malformed|record\n" + flights_path.read_text(encoding="utf-8"), encoding="utf-8")
        result = self.run_backend({"action": "getFlights"})
        self.assertEqual(result["status"], "ok")
        self.assertEqual(len(result["data"]), 10)
        self.assertIn("Skipping malformed flight", self.run_backend_stderr())

    def run_backend_stderr(self):
        process = subprocess.run(
            [str(self.binary_path), json.dumps({"action": "getFlights"})],
            cwd=self.backend_dir,
            capture_output=True,
            text=True,
            encoding="utf-8",
            timeout=10,
        )
        return process.stderr

    def test_http_server_serves_frontend_and_real_backend_operations(self):
        shutil.copytree(ROOT / "frontend", self.temp_dir / "frontend")
        server_dir = self.temp_dir / "server"
        server_dir.mkdir()
        shutil.copy2(ROOT / "server" / "server.py", server_dir / "server.py")
        with socket.socket() as listener:
            listener.bind(("127.0.0.1", 0))
            port = listener.getsockname()[1]
        environment = os.environ.copy()
        environment["FLIGHTMS_PORT"] = str(port)
        server = subprocess.Popen(
            [sys.executable, str(server_dir / "server.py")],
            cwd=self.temp_dir,
            env=environment,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        base_url = f"http://127.0.0.1:{port}"

        def request(path="/", payload=None):
            body = None if payload is None else json.dumps(payload).encode("utf-8")
            headers = {"Content-Type": "application/json"} if body else {}
            req = urllib.request.Request(base_url + path, data=body, headers=headers)
            with urllib.request.urlopen(req, timeout=5) as response:
                content = response.read()
                if "application/json" in response.headers.get("Content-Type", ""):
                    content = json.loads(content)
                return response.status, content

        try:
            status = None
            for _ in range(50):
                if server.poll() is not None:
                    self.fail("HTTP integration server exited before becoming ready")
                try:
                    status, html = request("/index.html")
                    break
                except (OSError, urllib.error.URLError):
                    time.sleep(0.1)
            self.assertIsNotNone(status, "HTTP integration server did not become ready")
            self.assertEqual(status, 200)
            if isinstance(html, bytes):
                html = html.decode("utf-8")
            self.assertIn("Skylane", html)

            query = urllib.parse.urlencode(
                {"action": "searchFlights", "source": "del", "destination": "mumb", "date": "2025-08-01"}
            )
            _, flights = request("/?" + query)
            self.assertEqual([flight["flightID"] for flight in flights["data"]], ["FL001"])

            _, booking = request(
                "/",
                {
                    "action": "bookFlight",
                    "flightID": "FL001",
                    "passengerName": "HTTP Passenger",
                    "passengerAge": "30",
                    "passengerEmail": "http@example.com",
                    "seats": "2",
                },
            )
            self.assertEqual(booking["status"], "ok")
            _, updated_flights = request("/?action=getFlights")
            self.assertEqual(
                next(flight for flight in updated_flights["data"] if flight["flightID"] == "FL001")["seatsAvailable"],
                78,
            )
            self.assertTrue((self.data_dir / "bookings.txt").read_text(encoding="utf-8").strip())
        finally:
            server.terminate()
            server.wait(timeout=5)


if __name__ == "__main__":
    unittest.main()
