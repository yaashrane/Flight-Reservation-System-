let selectedFlight = null;
let bookingComplete = false;
const flightIDInput = document.getElementById('flightID');

function updateBookingProgress(currentStep) {
    document.querySelectorAll('.booking-step').forEach(step => {
        const stepNumber = Number(step.dataset.step);
        step.classList.toggle('is-complete', stepNumber < currentStep);
        step.classList.toggle('is-active', stepNumber === currentStep);
        if (stepNumber === currentStep) step.setAttribute('aria-current', 'step');
        else step.removeAttribute('aria-current');
    });
}

function updateSeatOptions(flight) {
    const select = document.getElementById('seats');
    const availability = document.getElementById('seatAvailability');
    const currentValue = Number(select.value) || 1;
    const available = Math.max(0, Number(flight.seatsAvailable) || 0);
    const maxSeats = Math.min(available, 10);
    select.innerHTML = maxSeats
        ? Array.from({ length: maxSeats }, (_, index) => {
            const count = index + 1;
            return `<option value="${count}">${count} ${count === 1 ? 'seat' : 'seats'}</option>`;
        }).join('')
        : '<option value="">No seats available</option>';
    select.disabled = maxSeats === 0;
    if (maxSeats) select.value = String(Math.min(currentValue, maxSeats));
    availability.textContent = available
        ? `${available} ${available === 1 ? 'seat is' : 'seats are'} available.`
        : 'This flight has no seats available.';
    availability.classList.toggle('field-help-warning', available === 0);
}

function syncBookingActions() {
    const validSelection = selectedFlight && selectedFlight.flightID === flightIDInput.value.trim()
        && Number(selectedFlight.seatsAvailable) > 0;
    document.getElementById('confirmBooking').disabled = !validSelection || bookingComplete;
    document.getElementById('seats').disabled = !validSelection;
}

function renderFlightDetails(flight) {
    selectedFlight = flight;
    document.getElementById('flightInfo').innerHTML = `
        <div class="summary-airline"><strong>${escapeHTML(flight.airline)}</strong><span>${escapeHTML(flight.flightID)}</span></div>
        <div class="summary-route">
            <div><strong>${escapeHTML(flight.departureTime)}</strong><span>${escapeHTML(flight.source)}</span></div>
            <span aria-hidden="true">→</span>
            <div style="text-align:right"><strong>${escapeHTML(flight.arrivalTime)}</strong><span>${escapeHTML(flight.destination)}</span></div>
        </div>
        <div class="summary-details"><span>${escapeHTML(flight.flightDate)}</span><span>${formatDuration(flight.durationMin)}</span><span>${Number(flight.seatsAvailable)} seats left</span></div>
        `;
    updateSeatOptions(flight);
    syncBookingActions();
    updateTotal();
    updateBookingProgress(bookingComplete ? 3 : 2);
}

async function loadFlightDetails(flightID) {
    if (!flightID) {
        selectedFlight = null;
        document.getElementById('flightInfo').innerHTML = '<div class="summary-placeholder">Choose a flight to see its details.</div>';
        document.getElementById('seatAvailability').textContent = 'Select a flight to check available seats.';
        document.getElementById('seats').disabled = true;
        syncBookingActions();
        updateTotal();
        updateBookingProgress(1);
        return;
    }
    try {
        const response = await FlightAPI.search('flightID', flightID);
        if (response.status !== 'ok') throw new Error(response.message || 'Could not find this flight.');
        if (!response.data.length) {
            selectedFlight = null;
            document.getElementById('flightInfo').innerHTML = '<div class="summary-placeholder">No flight found with that ID. Check the ID and try again.</div>';
            document.getElementById('seatAvailability').textContent = 'Enter a valid flight ID to check seats.';
            document.getElementById('seats').disabled = true;
            syncBookingActions();
            updateTotal();
            updateBookingProgress(1);
            return;
        }
        renderFlightDetails(response.data[0]);
    } catch (error) {
        selectedFlight = null;
        document.getElementById('seats').disabled = true;
        syncBookingActions();
        updateTotal();
        updateBookingProgress(1);
        showAlert(error.message);
    }
}

function updateTotal() {
    const row = document.getElementById('totalFareRow');
    if (!row) return;
    row.hidden = !selectedFlight;
    if (!selectedFlight) return;
    const seats = Number(document.getElementById('seats').value) || 0;
    document.getElementById('seatFare').textContent = formatPrice(selectedFlight.price);
    document.getElementById('seatCount').textContent = String(seats);
    document.getElementById('totalFare').textContent = formatPrice(selectedFlight.price * seats);
}

function showConfirmation(booking, flight, seats) {
    const confirmation = document.getElementById('confirmation');
    confirmation.innerHTML = `<div class="confirmation-icon" aria-hidden="true">✓</div>
        <div class="confirmation-copy"><p class="eyebrow eyebrow-dark">BOOKING CONFIRMED</p><h2>You're all set, ${escapeHTML(booking.passengerName)}.</h2><p>Your reservation is saved. Keep this booking reference for your records.</p></div>
        <div class="confirmation-ticket"><div><span>BOOKING REFERENCE</span><strong>${escapeHTML(booking.bookingID)}</strong></div><div><span>FLIGHT</span><strong>${escapeHTML(flight.flightID)} · ${escapeHTML(flight.airline)}</strong></div><div><span>ROUTE</span><strong>${escapeHTML(flight.source)} → ${escapeHTML(flight.destination)}</strong></div><div><span>DEPARTURE</span><strong>${escapeHTML(flight.flightDate)} · ${escapeHTML(flight.departureTime)}</strong></div><div><span>PASSENGERS</span><strong>${seats} ${seats === 1 ? 'seat' : 'seats'}</strong></div><div><span>TOTAL FARE</span><strong>${formatPrice(Number(flight.price) * seats)}</strong></div></div>
        <div class="confirmation-actions"><a class="btn btn-primary" href="bookings.html">View booking history <span>→</span></a><a class="btn btn-quiet" href="flights.html">Explore more flights</a></div>`;
    confirmation.hidden = false;
    updateBookingProgress(3);
    confirmation.scrollIntoView({ behavior: 'smooth', block: 'center' });
}

const params = new URLSearchParams(window.location.search);
flightIDInput.value = params.get('flightID') || '';
updateBookingProgress(flightIDInput.value ? 2 : 1);
if (flightIDInput.value) loadFlightDetails(flightIDInput.value);
flightIDInput.addEventListener('change', () => loadFlightDetails(flightIDInput.value.trim()));
flightIDInput.addEventListener('input', () => {
    if (selectedFlight && selectedFlight.flightID !== flightIDInput.value.trim()) {
        selectedFlight = null;
        document.getElementById('flightInfo').innerHTML = '<div class="summary-placeholder">Checking the selected flight…</div>';
        document.getElementById('seats').disabled = true;
        document.getElementById('seatAvailability').textContent = 'Select a valid flight to check available seats.';
        syncBookingActions();
        updateTotal();
        updateBookingProgress(1);
    }
});
document.getElementById('seats').addEventListener('change', updateTotal);

document.getElementById('bookForm').addEventListener('submit', async event => {
    event.preventDefault();
    const payload = {
        flightID: flightIDInput.value.trim(),
        passengerName: document.getElementById('name').value.trim(),
        passengerEmail: document.getElementById('email').value.trim(),
        passengerAge: document.getElementById('age').value,
        seats: document.getElementById('seats').value
    };
    if (!payload.flightID || !payload.passengerName || !payload.passengerEmail || !payload.passengerAge) {
        showAlert('Please complete each passenger and flight field.');
        return;
    }
    if (!selectedFlight || selectedFlight.flightID !== payload.flightID || Number(selectedFlight.seatsAvailable) < Number(payload.seats)) {
        showAlert('Select a valid flight before confirming your booking.');
        return;
    }
    const button = document.getElementById('confirmBooking');
    button.disabled = true;
    button.setAttribute('aria-busy', 'true');
    button.textContent = 'Confirming…';
    try {
        const response = await BookingAPI.book(payload);
        if (response.status !== 'ok') throw new Error(response.message || 'Booking could not be completed.');
        bookingComplete = true;
        showConfirmation(response.data, selectedFlight, Number(payload.seats));
        document.getElementById('bookForm').reset();
        flightIDInput.value = payload.flightID;
        await loadFlightDetails(payload.flightID);
        showAlert('Booking confirmed successfully.', 'success');
    } catch (error) {
        showAlert(error.message);
        if (selectedFlight) await loadFlightDetails(selectedFlight.flightID);
    } finally {
        button.innerHTML = 'Confirm booking <span>→</span>';
        button.removeAttribute('aria-busy');
        syncBookingActions();
    }
});
