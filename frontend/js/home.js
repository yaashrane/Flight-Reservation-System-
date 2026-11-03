function updateCitySuggestions(flights) {
    const cities = [...new Set(flights.flatMap(flight => [flight.source, flight.destination]))]
        .filter(Boolean)
        .sort((left, right) => left.localeCompare(right));
    document.getElementById('homeCities').innerHTML = cities
        .map(city => `<option value="${escapeHTML(city)}"></option>`)
        .join('');
}

function renderPopularDestinations(flights) {
    const destinationCounts = new Map();
    flights.forEach(flight => {
        destinationCounts.set(flight.destination, (destinationCounts.get(flight.destination) || 0) + 1);
    });
    const destinations = [...destinationCounts.entries()]
        .sort((left, right) => right[1] - left[1] || left[0].localeCompare(right[0]))
        .slice(0, 3);
    const list = document.getElementById('popularDestinations');
    const status = document.getElementById('destinationStatus');

    if (!destinations.length) {
        list.innerHTML = '<div class="empty-state destination-empty"><strong>No destinations in the schedule yet.</strong>Add flights from the Admin page to feature destinations here.</div>';
        status.textContent = 'Destinations are generated from the current flight schedule.';
        return;
    }

    const labels = ['MOST CONNECTED', 'ON THE MAP', 'PLAN A GETAWAY'];
    const descriptions = ['Explore available arrivals', 'See flights landing here', 'Find your next connection'];
    list.innerHTML = destinations.map(([city, count], index) => {
        const url = `flights.html?${new URLSearchParams({ destination: city }).toString()}`;
        return `<a class="destination-card destination-dynamic destination-photo-${index + 1}" href="${url}">
            <span class="destination-tag">${labels[index]}</span>
            <span class="destination-name">${escapeHTML(city)}</span>
            <span class="destination-meta">${descriptions[index]} · ${count} ${count === 1 ? 'flight' : 'flights'} <span aria-hidden="true">↗</span></span>
        </a>`;
    }).join('');
    status.textContent = 'Popular destinations, based on flights in the current schedule.';
}

async function loadHomeDiscovery() {
    const status = document.getElementById('destinationStatus');
    try {
        const response = await FlightAPI.getAll();
        if (response.status !== 'ok') throw new Error(response.message || 'Could not load destinations.');
        updateCitySuggestions(response.data);
        renderPopularDestinations(response.data);
    } catch (error) {
        document.getElementById('popularDestinations').innerHTML = '<div class="empty-state destination-empty"><strong>Destinations are temporarily unavailable.</strong>You can still search using a city from the flight schedule.</div>';
        status.textContent = error.message;
    }
}

document.getElementById('swapCities').addEventListener('click', () => {
    const source = document.getElementById('homeSource');
    const destination = document.getElementById('homeDestination');
    [source.value, destination.value] = [destination.value, source.value];
    const button = document.getElementById('swapCities');
    button.classList.remove('is-swapping');
    requestAnimationFrame(() => button.classList.add('is-swapping'));
});

document.getElementById('homeSearch').addEventListener('submit', event => {
    event.preventDefault();
    const source = document.getElementById('homeSource').value.trim();
    const destination = document.getElementById('homeDestination').value.trim();
    const date = document.getElementById('homeDate').value;
    const params = new URLSearchParams();
    if (source) params.set('source', source);
    if (destination) params.set('destination', destination);
    if (date) params.set('date', date);
    const query = params.toString();
    window.location.href = `flights.html${query ? `?${query}` : ''}`;
});

loadHomeDiscovery();
