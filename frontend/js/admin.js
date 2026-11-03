let adminFlights = [];
let adminBookings = [];

function renderAdminFlights(flights) {
    const list = document.getElementById('adminFlightList');
    const searchTerm = document.getElementById('adminFlightSearch').value.trim().toLocaleLowerCase();
    const sortBy = document.getElementById('adminFlightSort').value;
    const visibleFlights = flights.filter(flight => [
        flight.flightID, flight.airline, flight.source, flight.destination
    ].some(value => String(value).toLocaleLowerCase().includes(searchTerm)));
    const sortKeys = {
        date: flight => `${flight.flightDate}${flight.departureTime}`,
        flightID: flight => flight.flightID,
        route: flight => `${flight.source}${flight.destination}`,
        price: flight => Number(flight.price),
        seats: flight => Number(flight.seatsAvailable)
    };
    visibleFlights.sort((left, right) => {
        const leftKey = sortKeys[sortBy](left);
        const rightKey = sortKeys[sortBy](right);
        if (sortBy === 'seats') return rightKey - leftKey;
        return typeof leftKey === 'number' ? leftKey - rightKey : leftKey.localeCompare(rightKey);
    });
    document.getElementById('adminFlightCount').textContent = searchTerm
        ? `${visibleFlights.length} of ${flights.length} flights`
        : `${flights.length} ${flights.length === 1 ? 'flight' : 'flights'}`;
    if (!visibleFlights.length) {
        list.innerHTML = searchTerm
            ? '<div class="empty-state"><strong>No flights match that search.</strong><p>Try a different flight ID, airline or route.</p><button class="btn btn-quiet" type="button" data-reset-admin-search>Clear search</button></div>'
            : '<div class="empty-state"><strong>No flights in the schedule.</strong><p>Add a flight to make it available for booking.</p></div>';
        return;
    }
    list.innerHTML = visibleFlights.map(flight => `<article class="admin-flight-row">
        <div class="admin-flight-cell"><strong>${escapeHTML(flight.flightID)}</strong><span>${escapeHTML(flight.airline)}</span></div>
        <div class="admin-flight-route"><strong>${escapeHTML(flight.source)} → ${escapeHTML(flight.destination)}</strong><span>${escapeHTML(flight.flightDate)} · ${escapeHTML(flight.departureTime)}–${escapeHTML(flight.arrivalTime)}</span></div>
        <div class="admin-flight-cell"><strong>${formatPrice(flight.price)}</strong><span>per seat</span></div>
        <div class="admin-flight-cell"><strong>${Number(flight.seatsAvailable)}</strong><span>seats available</span></div>
        <div class="admin-flight-cell"><strong>${formatDuration(flight.durationMin)}</strong><span>duration</span></div>
        <div class="admin-row-actions"><button type="button" class="btn btn-quiet edit-flight" data-flight-id="${escapeHTML(flight.flightID)}">Edit</button><button type="button" class="btn btn-danger delete-flight" data-flight-id="${escapeHTML(flight.flightID)}">Delete</button></div>
    </article>`).join('');
}

function updateStats(bookings) {
    document.getElementById('totalFlights').textContent = adminFlights.length;
    document.getElementById('totalBookings').textContent = bookings.length;
    document.getElementById('availableSeats').textContent = adminFlights.reduce((sum, flight) => sum + Number(flight.seatsAvailable), 0).toLocaleString('en-IN');
    renderBookingStatusChart(bookings);
    renderRouteSeatChart(adminFlights);
}

function renderBookingStatusChart(bookings) {
    const confirmed = bookings.filter(booking => String(booking.status).toLowerCase() === 'confirmed').length;
    const cancelled = bookings.filter(booking => String(booking.status).toLowerCase() === 'cancelled').length;
    const total = confirmed + cancelled;
    const confirmedPercent = total ? Math.round((confirmed / total) * 100) : 0;
    const cancelledPercent = total ? 100 - confirmedPercent : 0;
    document.getElementById('bookingStatusChart').innerHTML = total
        ? `<div class="booking-status-total"><strong>${total}</strong><span>total reservations</span></div>
            <div class="status-chart-track" role="img" aria-label="${confirmed} confirmed and ${cancelled} cancelled reservations">
                <span class="status-chart-confirmed" style="width:${confirmedPercent}%"></span><span class="status-chart-cancelled" style="width:${cancelledPercent}%"></span>
            </div>
            <div class="status-chart-legend"><span><i class="legend-dot confirmed"></i>Confirmed <strong>${confirmed}</strong></span><span><i class="legend-dot cancelled"></i>Cancelled <strong>${cancelled}</strong></span></div>`
        : '<div class="chart-empty"><span aria-hidden="true">▤</span><strong>No reservations yet</strong><small>Booking activity will appear here.</small></div>';
}

function renderRouteSeatChart(flights) {
    const routeSeats = new Map();
    flights.forEach(flight => {
        const route = `${flight.source} → ${flight.destination}`;
        routeSeats.set(route, (routeSeats.get(route) || 0) + Number(flight.seatsAvailable));
    });
    const routes = [...routeSeats.entries()].sort((left, right) => right[1] - left[1] || left[0].localeCompare(right[0])).slice(0, 5);
    const maxSeats = routes[0]?.[1] || 1;
    document.getElementById('routeSeatChart').innerHTML = routes.length
        ? routes.map(([route, seats]) => `<div class="route-seat-row"><div class="route-seat-label"><span>${escapeHTML(route)}</span><strong>${seats.toLocaleString('en-IN')}</strong></div><div class="route-seat-track" role="progressbar" aria-label="${escapeHTML(route)} available seats" aria-valuemin="0" aria-valuemax="${maxSeats}" aria-valuenow="${seats}"><span style="width:${seats > 0 ? Math.max(4, seats / maxSeats * 100) : 0}%"></span></div></div>`).join('')
        : '<div class="chart-empty"><span aria-hidden="true">✈</span><strong>No scheduled flights</strong><small>Add flights to see available seat inventory.</small></div>';
}

async function loadAdminData() {
    document.getElementById('adminFlightList').innerHTML = '<div class="admin-skeleton" aria-hidden="true"><span></span><span></span><span></span></div>';
    document.getElementById('adminFlightList').setAttribute('aria-busy', 'true');
    document.getElementById('alert').classList.remove('show');
    const results = await Promise.allSettled([FlightAPI.getAll(), BookingAPI.get('')]);
    const flightResult = results[0];
    const bookingResult = results[1];
    if (flightResult.status === 'fulfilled' && flightResult.value.status === 'ok') {
        adminFlights = flightResult.value.data;
        renderAdminFlights(adminFlights);
    } else {
        const error = flightResult.status === 'rejected' ? flightResult.reason : new Error(flightResult.value.message || 'Could not load flights.');
        showAlert(error.message);
        document.getElementById('adminFlightList').innerHTML = '';
    }
    if (bookingResult.status === 'fulfilled' && bookingResult.value.status === 'ok') {
        adminBookings = bookingResult.value.data;
        updateStats(adminBookings);
    } else {
        const error = bookingResult.status === 'rejected' ? bookingResult.reason : new Error(bookingResult.value.message || 'Could not load bookings.');
        showAlert(`Booking statistics unavailable: ${error.message}`);
        document.getElementById('totalBookings').textContent = '—';
        adminBookings = [];
        document.getElementById('bookingStatusChart').innerHTML = '<div class="chart-empty"><strong>Reservation data unavailable</strong><small>Refresh the page to try again.</small></div>';
        document.getElementById('totalFlights').textContent = adminFlights.length;
        document.getElementById('availableSeats').textContent = adminFlights.reduce((sum, flight) => sum + Number(flight.seatsAvailable), 0).toLocaleString('en-IN');
        renderRouteSeatChart(adminFlights);
    }
    document.getElementById('adminFlightList').removeAttribute('aria-busy');
}

function showForm(mode = 'add', flight = null) {
    const section = document.getElementById('formSection');
    section.hidden = false;
    document.getElementById('formTitle').textContent = mode === 'add' ? 'Add a flight' : `Update ${flight.flightID}`;
    document.getElementById('formMode').value = mode;
    const fields = ['flightID', 'airline', 'source', 'destination', 'flightDate', 'departureTime', 'arrivalTime', 'durationMin', 'price', 'seatsAvailable'];
    fields.forEach(id => {
        const input = document.getElementById(`f_${id}`);
        input.value = flight ? flight[id] : '';
        input.readOnly = mode === 'update' && id === 'flightID';
    });
    section.scrollIntoView({ behavior: 'smooth', block: 'center' });
    document.getElementById('f_flightID').focus();
}

function hideForm() {
    document.getElementById('formSection').hidden = true;
    document.getElementById('flightForm').reset();
    document.getElementById('f_flightID').readOnly = false;
}

async function deleteFlight(flightID, button) {
    if (!window.confirm(`Delete flight ${flightID}? This cannot be undone.`)) return;
    button.disabled = true;
    try {
        const response = await FlightAPI.delete(flightID);
        if (response.status !== 'ok') throw new Error(response.message || 'Could not delete this flight.');
        showAlert(`Flight ${flightID} deleted.`, 'success');
        await loadAdminData();
    } catch (error) {
        showAlert(error.message);
        button.disabled = false;
    }
}

document.getElementById('addBtn').addEventListener('click', () => showForm());
document.getElementById('cancelFormBtn').addEventListener('click', hideForm);
document.getElementById('cancelFormTextBtn').addEventListener('click', hideForm);
document.getElementById('adminFlightList').addEventListener('click', event => {
    if (event.target.closest('[data-reset-admin-search]')) {
        document.getElementById('adminFlightSearch').value = '';
        renderAdminFlights(adminFlights);
        return;
    }
    const editButton = event.target.closest('.edit-flight');
    if (editButton) {
        const flight = adminFlights.find(item => item.flightID === editButton.dataset.flightId);
        if (flight) showForm('update', flight);
        return;
    }
    const deleteButton = event.target.closest('.delete-flight');
    if (deleteButton) deleteFlight(deleteButton.dataset.flightId, deleteButton);
});
document.getElementById('adminFlightSearch').addEventListener('input', () => renderAdminFlights(adminFlights));
document.getElementById('adminFlightSort').addEventListener('change', () => renderAdminFlights(adminFlights));

document.getElementById('flightForm').addEventListener('submit', async event => {
    event.preventDefault();
    const mode = document.getElementById('formMode').value;
    const data = {
        flightID: document.getElementById('f_flightID').value.trim(),
        airline: document.getElementById('f_airline').value.trim(),
        source: document.getElementById('f_source').value.trim(),
        destination: document.getElementById('f_destination').value.trim(),
        flightDate: document.getElementById('f_flightDate').value,
        departureTime: document.getElementById('f_departureTime').value,
        arrivalTime: document.getElementById('f_arrivalTime').value,
        durationMin: document.getElementById('f_durationMin').value,
        price: document.getElementById('f_price').value,
        seatsAvailable: document.getElementById('f_seatsAvailable').value
    };
    const submitButton = document.querySelector('#flightForm button[type="submit"]');
    submitButton.disabled = true;
    try {
        const response = mode === 'add' ? await FlightAPI.add(data) : await FlightAPI.update(data);
        if (response.status !== 'ok') throw new Error(response.message || 'Could not save this flight.');
        hideForm();
        showAlert(mode === 'add' ? 'Flight added to the schedule.' : 'Flight details updated.', 'success');
        await loadAdminData();
    } catch (error) {
        showAlert(error.message);
    } finally {
        submitButton.disabled = false;
    }
});

loadAdminData();
