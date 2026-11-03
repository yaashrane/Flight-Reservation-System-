let currentBookings = [];
let flightLookup = {};
let activeBookingFilter = 'all';

function renderBookings(bookings) {
    const list = document.getElementById('bookingList');
    const count = document.getElementById('bookingCount');
    const filteredBookings = activeBookingFilter === 'all'
        ? bookings
        : bookings.filter(booking => String(booking.status).toLowerCase() === activeBookingFilter);
    count.textContent = `${filteredBookings.length} ${filteredBookings.length === 1 ? 'reservation' : 'reservations'}${filteredBookings.length !== bookings.length ? ` · ${bookings.length} total` : ''}`;
    if (!filteredBookings.length) {
        const message = bookings.length
            ? `No ${activeBookingFilter} bookings. Choose another status filter to see more trips.`
            : 'Try another email address or browse available flights to plan your next trip.';
        list.innerHTML = `<div class="empty-state"><span class="empty-state-icon" aria-hidden="true">▤</span><strong>${bookings.length ? `No ${escapeHTML(activeBookingFilter)} reservations.` : 'No bookings found.'}</strong><p>${message}</p><a class="btn btn-quiet" href="flights.html">Browse flights</a></div>`;
        return;
    }

    list.innerHTML = filteredBookings.map(booking => {
        const status = String(booking.status).toLowerCase();
        const flight = flightLookup[booking.flightID];
        const route = flight
            ? `${escapeHTML(flight.source)} <span aria-hidden="true">→</span> ${escapeHTML(flight.destination)}`
            : `Flight ${escapeHTML(booking.flightID)}`;
        const details = flight
            ? `${escapeHTML(flight.flightDate)} · ${escapeHTML(flight.departureTime)} → ${escapeHTML(flight.arrivalTime)} · ${formatDuration(flight.durationMin)} · ${escapeHTML(flight.airline)}`
            : `Booked ${escapeHTML(booking.bookingDate)}`;
        return `<article class="booking-card">
            <div class="booking-card-top"><div class="booking-ref">BOOKING ID &nbsp;<strong>${escapeHTML(booking.bookingID)}</strong></div><span class="status-badge status-${escapeHTML(status)}">${escapeHTML(status)}</span></div>
            <div class="booking-card-body">
                <div class="booking-person"><strong>${escapeHTML(booking.passengerName)}</strong><span>${escapeHTML(booking.passengerEmail)}</span></div>
                <div class="booking-route"><strong>${route}</strong><span>${details}</span><small>Flight ${escapeHTML(booking.flightID)}</small></div>
                <div class="booking-seats">${Number(booking.seats)} ${Number(booking.seats) === 1 ? 'seat' : 'seats'}<span>Booked ${escapeHTML(booking.bookingDate)}</span></div>
                ${status === 'confirmed' ? `<button type="button" class="btn btn-danger cancel-booking" data-booking-id="${escapeHTML(booking.bookingID)}">Cancel booking</button>` : '<span class="muted-copy">No action needed</span>'}
            </div>
        </article>`;
    }).join('');
}

async function loadBookings(email = '') {
    const button = document.getElementById('searchBtn');
    const loader = document.getElementById('bookingLoader');
    button.disabled = true;
    document.getElementById('showAllBtn').disabled = true;
    loader.hidden = false;
    document.getElementById('bookingList').setAttribute('aria-busy', 'true');
    document.getElementById('alert').classList.remove('show');
    try {
        const [bookingResponse, flightResponse] = await Promise.all([BookingAPI.get(email), FlightAPI.getAll()]);
        if (bookingResponse.status !== 'ok') throw new Error(bookingResponse.message || 'Could not load bookings.');
        if (flightResponse.status !== 'ok') throw new Error(flightResponse.message || 'Could not load flight details.');
        currentBookings = bookingResponse.data;
        flightLookup = Object.fromEntries(flightResponse.data.map(flight => [flight.flightID, flight]));
        renderBookings(currentBookings);
    } catch (error) {
        showAlert(error.message);
        document.getElementById('bookingCount').textContent = 'Bookings could not be loaded';
        document.getElementById('bookingList').innerHTML = '';
    } finally {
        loader.hidden = true;
        document.getElementById('bookingList').removeAttribute('aria-busy');
        button.disabled = false;
        document.getElementById('showAllBtn').disabled = false;
    }
}

document.getElementById('searchBtn').addEventListener('click', () => {
    const email = document.getElementById('emailFilter').value.trim();
    if (!email) {
        showAlert('Enter the email address used for your booking.');
        return;
    }
    loadBookings(email);
});
document.getElementById('showAllBtn').addEventListener('click', () => {
    document.getElementById('emailFilter').value = '';
    loadBookings();
});
document.getElementById('emailFilter').addEventListener('keydown', event => {
    if (event.key === 'Enter') document.getElementById('searchBtn').click();
});
document.getElementById('bookingList').addEventListener('click', async event => {
    const button = event.target.closest('.cancel-booking');
    if (!button) return;
    const bookingID = button.dataset.bookingId;
    if (!window.confirm(`Cancel booking ${bookingID}?`)) return;
    button.disabled = true;
    try {
        const response = await BookingAPI.cancel(bookingID);
        if (response.status !== 'ok') throw new Error(response.message || 'Could not cancel this booking.');
        showAlert('Your booking has been cancelled.', 'success');
        await loadBookings(document.getElementById('emailFilter').value.trim());
    } catch (error) {
        showAlert(error.message);
        button.disabled = false;
    }
});

document.querySelector('.booking-filters').addEventListener('click', event => {
    const button = event.target.closest('[data-status]');
    if (!button) return;
    activeBookingFilter = button.dataset.status;
    document.querySelectorAll('.booking-filter').forEach(filter => {
        const active = filter === button;
        filter.classList.toggle('is-active', active);
        filter.setAttribute('aria-pressed', String(active));
    });
    renderBookings(currentBookings);
});

document.getElementById('bookingCount').textContent = 'Search by booking email to see your trips';
document.getElementById('bookingList').innerHTML = '<div class="empty-state"><strong>Your journeys will show up here.</strong>Enter the email address used when booking to find your reservations.</div>';
