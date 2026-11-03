function renderFlights(flights) {
    const list = document.getElementById('flightList');
    const count = document.getElementById('resultsCount');
    count.textContent = `${flights.length} ${flights.length === 1 ? 'flight' : 'flights'} found`;
    if (!flights.length) {
        list.innerHTML = `<div class="empty-state"><span class="empty-state-icon" aria-hidden="true">✈</span><strong>No flights match those details.</strong><p>Try a nearby date, check the city spelling, or clear filters to browse all scheduled flights.</p><button class="btn btn-quiet" type="button" data-clear-search>Clear search filters</button></div>`;
        return;
    }
    list.innerHTML = flights.map((flight, index) => {
        const flightID = escapeHTML(flight.flightID);
        const route = `${escapeHTML(flight.source)} to ${escapeHTML(flight.destination)}`;
        const bookURL = `book.html?flightID=${encodeURIComponent(flight.flightID)}`;
        return `<article class="flight-card" style="animation-delay:${Math.min(index, 8) * 45}ms">
            <div class="airline-ident"><span class="airline-mark" aria-hidden="true">✈</span><div><strong>${escapeHTML(flight.airline)}</strong><span class="flight-id">${flightID}</span></div></div>
            <div class="flight-route" aria-label="${route}">
                <div class="route-end"><strong>${escapeHTML(flight.departureTime)}</strong><span>${escapeHTML(flight.source)}</span></div>
                <div class="route-track"><span>${formatDuration(flight.durationMin)}</span><i class="route-track-line"></i><span>Non-stop</span></div>
                <div class="route-end"><strong>${escapeHTML(flight.arrivalTime)}</strong><span>${escapeHTML(flight.destination)}</span></div>
            </div>
            <div class="flight-meta"><span>DEPARTURE</span><strong>${escapeHTML(flight.flightDate)}</strong><span class="seats-note ${Number(flight.seatsAvailable) <= 5 ? 'seats-low' : ''}">${Number(flight.seatsAvailable) === 0 ? 'Sold out' : `${Number(flight.seatsAvailable)} ${Number(flight.seatsAvailable) === 1 ? 'seat' : 'seats'} available`}</span></div>
            <div class="flight-price"><strong>${formatPrice(flight.price)}</strong><span>per passenger</span><span class="seats-chip-mobile ${Number(flight.seatsAvailable) <= 5 ? 'seats-low' : ''}">${Number(flight.seatsAvailable) === 0 ? 'Sold out' : `${Number(flight.seatsAvailable)} seats available`}</span>${Number(flight.seatsAvailable) > 0 ? `<a class="btn btn-primary" href="${bookURL}">Select <span>→</span></a>` : '<span class="sold-out-label">Currently full</span>'}</div>
        </article>`;
    }).join('');
}

function renderActiveFilters() {
    const criteria = [
        ['source', 'From', document.getElementById('sourceFilter').value.trim()],
        ['destination', 'To', document.getElementById('destinationFilter').value.trim()],
        ['date', 'Date', document.getElementById('dateFilter').value]
    ].filter(([, , value]) => value);
    const container = document.getElementById('activeFilters');
    container.hidden = criteria.length === 0;
    container.innerHTML = criteria.map(([key, label, value]) => `<button type="button" class="filter-chip" data-filter="${key}" aria-label="Remove ${label} filter: ${escapeHTML(value)}"><span>${label}</span> ${escapeHTML(value)} <span aria-hidden="true">×</span></button>`).join('')
        + (criteria.length ? '<button type="button" class="clear-filters" data-clear-search>Clear all</button>' : '');
}

function updateSearchURL() {
    const params = new URLSearchParams();
    const source = document.getElementById('sourceFilter').value.trim();
    const destination = document.getElementById('destinationFilter').value.trim();
    const date = document.getElementById('dateFilter').value;
    if (source) params.set('source', source);
    if (destination) params.set('destination', destination);
    if (date) params.set('date', date);
    const queryString = params.toString();
    history.replaceState(null, '', `${location.pathname}${queryString ? `?${queryString}` : ''}`);
}

function updateResultsTitle() {
    const source = document.getElementById('sourceFilter').value.trim();
    const destination = document.getElementById('destinationFilter').value.trim();
    document.getElementById('resultsTitle').textContent = source || destination
        ? `${source || 'Any city'} → ${destination || 'Any destination'}`
        : 'Available flights';
}

async function refreshFlights() {
    const loader = document.getElementById('loader');
    loader.hidden = false;
    const button = document.getElementById('searchBtn');
    button.disabled = true;
    button.setAttribute('aria-busy', 'true');
    document.getElementById('flightList').setAttribute('aria-busy', 'true');
    document.getElementById('alert').classList.remove('show');
    try {
        const criteria = {
            source: document.getElementById('sourceFilter').value.trim(),
            destination: document.getElementById('destinationFilter').value.trim(),
            date: document.getElementById('dateFilter').value
        };
        const sortBy = document.getElementById('sortBy').value;
        const response = await FlightAPI.searchFlights(criteria, sortBy);
        if (response.status !== 'ok') throw new Error(response.message || 'Could not load flights.');
        updateResultsTitle();
        renderActiveFilters();
        updateSearchURL();
        renderFlights(response.data);
    } catch (error) {
        showAlert(error.message);
        document.getElementById('flightList').innerHTML = '';
        document.getElementById('resultsCount').textContent = 'Flights could not be loaded';
    } finally {
        loader.hidden = true;
        button.disabled = false;
        button.removeAttribute('aria-busy');
        document.getElementById('flightList').removeAttribute('aria-busy');
    }
}

const query = new URLSearchParams(window.location.search);
document.getElementById('sourceFilter').value = query.get('source') || '';
document.getElementById('destinationFilter').value = query.get('destination') || '';
document.getElementById('dateFilter').value = query.get('date') || '';
document.getElementById('searchBtn').addEventListener('click', refreshFlights);
document.getElementById('sourceFilter').addEventListener('keydown', event => { if (event.key === 'Enter') refreshFlights(); });
document.getElementById('destinationFilter').addEventListener('keydown', event => { if (event.key === 'Enter') refreshFlights(); });
document.getElementById('dateFilter').addEventListener('keydown', event => { if (event.key === 'Enter') refreshFlights(); });
document.getElementById('sortBy').addEventListener('change', refreshFlights);

document.getElementById('flightList').addEventListener('click', event => {
    if (event.target.closest('[data-clear-search]')) {
        document.getElementById('sourceFilter').value = '';
        document.getElementById('destinationFilter').value = '';
        document.getElementById('dateFilter').value = '';
        refreshFlights();
    }
});
document.getElementById('activeFilters').addEventListener('click', event => {
    const button = event.target.closest('[data-filter]');
    const clearAll = event.target.closest('[data-clear-search]');
    if (button) document.getElementById(`${button.dataset.filter}Filter`).value = '';
    if (clearAll) {
        document.getElementById('sourceFilter').value = '';
        document.getElementById('destinationFilter').value = '';
        document.getElementById('dateFilter').value = '';
    }
    if (button || clearAll) refreshFlights();
});

FlightAPI.getAll().then(response => {
    if (response.status !== 'ok') throw new Error(response.message || 'Could not load city suggestions.');
    const cities = [...new Set(response.data.flatMap(flight => [flight.source, flight.destination]))].sort((a, b) => a.localeCompare(b));
    document.getElementById('flightCities').innerHTML = cities.map(city => `<option value="${escapeHTML(city)}"></option>`).join('');
}).catch(error => showAlert(`City suggestions unavailable: ${error.message}`));

refreshFlights();
