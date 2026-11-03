const API = window.location.protocol === 'file:' ? 'http://127.0.0.1:8080' : window.location.origin;

async function request(url, options) {
    let response;
    try {
        response = await fetch(url, options);
    } catch (error) {
        throw new Error('Unable to connect to Skylane. Make sure the local server is running.');
    }

    let result;
    try {
        result = await response.json();
    } catch (error) {
        throw new Error('The server returned an invalid response. Please try again.');
    }

    if (!response.ok) {
        throw new Error(result.message || `Request failed (${response.status}).`);
    }
    return result;
}

async function apiGet(params) {
    return request(`${API}?${new URLSearchParams(params).toString()}`);
}

async function apiPost(payload) {
    return request(API, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
    });
}

function escapeHTML(value) {
    return String(value ?? '').replace(/[&<>"']/g, character => ({
        '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'
    })[character]);
}

function formatPrice(value) {
    return new Intl.NumberFormat('en-IN', {
        style: 'currency', currency: 'INR', maximumFractionDigits: 0
    }).format(Number(value) || 0);
}

function formatDuration(minutes) {
    const duration = Number(minutes) || 0;
    const hours = Math.floor(duration / 60);
    const remainder = duration % 60;
    return hours ? `${hours}h ${remainder}m` : `${remainder}m`;
}

function showAlert(message, type = 'error') {
    const element = document.getElementById('alert');
    if (!element) return;
    element.textContent = message;
    element.className = `alert alert-${type} show`;
    clearTimeout(element.alertTimeout);
    element.alertTimeout = setTimeout(() => element.classList.remove('show'), 5000);
}

const FlightAPI = {
    getAll: () => apiGet({ action: 'getFlights' }),
    search: (key, value) => apiGet({ action: 'searchFlight', key, value }),
    searchFlights: (criteria, sortBy) => apiGet({
        action: 'searchFlights',
        ...criteria,
        ...(sortBy ? { sortBy } : {})
    }),
    sort: by => apiGet({ action: 'sortFlights', by }),
    add: data => apiPost({ action: 'addFlight', ...data }),
    update: data => apiPost({ action: 'updateFlight', ...data }),
    delete: flightID => apiPost({ action: 'deleteFlight', flightID })
};

const BookingAPI = {
    book: data => apiPost({ action: 'bookFlight', ...data }),
    cancel: bookingID => apiPost({ action: 'cancelBooking', bookingID }),
    get: email => apiGet({ action: 'getBookings', email: email || '' })
};
