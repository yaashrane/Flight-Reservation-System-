let flights = [];

const algorithmInfo = {
    linear: {
        title: 'Linear Search',
        icon: '⌕',
        description: 'Scan every flight record in its stored order and collect each case-insensitive match.',
        time: 'Worst time: O(n)',
        space: 'Space: O(k) results'
    },
    binary: {
        title: 'Binary Search',
        icon: '↔',
        description: 'Prepare an ID-sorted copy, then repeatedly halve the range until the target is found or the range is empty.',
        time: 'Search: O(log n); total: O(n²)',
        space: 'Copy: O(n); search: O(1)'
    },
    bubble: {
        title: 'Bubble Sort',
        icon: '↕',
        description: 'Compare neighboring ticket prices and swap out-of-order pairs. Stop early if a full pass makes no swaps.',
        time: 'Best: O(n); worst: O(n²)',
        space: 'Space: O(1)'
    },
    selection: {
        title: 'Selection Sort',
        icon: '⇅',
        description: 'Find the earliest date-and-time in the unsorted range, then put it into the next sorted position.',
        time: 'All cases: O(n²)',
        space: 'Space: O(1)'
    },
    merge: {
        title: 'Merge Sort',
        icon: '⌘',
        description: 'Recursively divide the duration list, then merge the sorted halves by choosing the shorter front record.',
        time: 'All cases: O(n log n)',
        space: 'Space: O(n)'
    }
};

function showDSAAlert(message, type = 'error') {
    const alert = document.getElementById('dsaAlert');
    alert.textContent = message;
    alert.className = `alert alert-${type} show`;
}

function flightLabel(flight, key) {
    if (key === 'price') return `${flight.flightID} · ${formatPrice(flight.price)}`;
    if (key === 'duration') return `${flight.flightID} · ${formatDuration(flight.durationMin)}`;
    if (key === 'departure') return `${flight.flightID} · ${flight.flightDate} ${flight.departureTime}`;
    return `${flight.flightID} · ${flight.source} → ${flight.destination}`;
}

function createDemoShell(key, input, steps, result, note = '') {
    const inputHTML = input.length
        ? `<div class="dsa-input-list">${input.map(value => `<span>${escapeHTML(value)}</span>`).join('')}</div>`
        : '<p class="dsa-empty-input">The input list is empty.</p>';
    const stepsHTML = steps.length
        ? `<ol class="dsa-step-list">${steps.map((step, index) => `<li><span class="dsa-step-number">${String(index + 1).padStart(2, '0')}</span><span>${escapeHTML(step)}</span></li>`).join('')}</ol>`
        : '<p class="dsa-empty-input">No comparisons were needed for this input.</p>';
    const outputHTML = result.length
        ? `<div class="dsa-input-list dsa-output-list">${result.map(value => `<span>${escapeHTML(value)}</span>`).join('')}</div>`
        : '<p class="dsa-empty-input">No matching records.</p>';
    return `<div class="dsa-trace">
        <div class="dsa-trace-column"><h4><span>01</span> Input data <small>${input.length} records</small></h4>${inputHTML}</div>
        <div class="dsa-trace-column"><h4><span>02</span> Step-by-step execution <small>${steps.length} steps</small></h4>${stepsHTML}${note ? `<p class="dsa-trace-note">${escapeHTML(note)}</p>` : ''}</div>
        <div class="dsa-trace-column dsa-final-output"><h4><span>03</span> Final output</h4>${outputHTML}</div>
    </div>`;
}

function demonstrateLinearSearch() {
    const key = document.getElementById('linearKey').value;
    const rawTarget = document.getElementById('linearTarget').value.trim();
    if (!rawTarget) throw new Error('Enter a non-empty value to search for.');
    const target = key === 'date' ? rawTarget : rawTarget.toLocaleLowerCase();
    const steps = [];
    const matches = [];

    flights.forEach((flight, index) => {
        const fieldValue = String(flight[key]).toLocaleLowerCase();
        const matched = fieldValue.includes(target);
        steps.push(`Check ${index + 1}: ${flightLabel(flight)}. Compare ${key} "${flight[key]}" with "${rawTarget}" → ${matched ? 'match; keep this flight.' : 'not a match; continue.'}`);
        if (matched) matches.push(flight);
    });

    const input = flights.map(flight => flightLabel(flight));
    const output = matches.map(flight => flightLabel(flight));
    const summary = matches.length
        ? `Matched ${matches.length} flight${matches.length === 1 ? '' : 's'} for ${key} containing "${rawTarget}".`
        : `No flight has a ${key} containing "${rawTarget}".`;
    return createDemoShell(key, input, steps, output, summary);
}

function demonstrateBinarySearch() {
    const target = document.getElementById('binaryTarget').value.trim().toUpperCase();
    if (!/^[A-Z0-9]{1,20}$/.test(target)) throw new Error('Enter a flight ID containing only letters and numbers (up to 20 characters).');
    const sorted = flights.slice();
    const preparationSteps = [];

    for (let i = 1; i < sorted.length; i++) {
        const keyFlight = sorted[i];
        let j = i - 1;
        const shifts = [];
        while (j >= 0 && sorted[j].flightID > keyFlight.flightID) {
            shifts.push(sorted[j].flightID);
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = keyFlight;
        if (shifts.length) {
            preparationSteps.push(`Insert ${keyFlight.flightID}: shift ${shifts.join(', ')} right → ${sorted.map(flight => flight.flightID).join(' · ')}.`);
        }
    }

    const steps = [];
    let low = 0;
    let high = sorted.length - 1;
    let found = null;
    while (low <= high) {
        const middle = low + Math.floor((high - low) / 2);
        const current = sorted[middle];
        steps.push(`low=${low}, high=${high}, mid=${middle} → ${current.flightID}. Compare ${current.flightID} with ${target}: ${current.flightID === target ? 'equal; found.' : current.flightID < target ? 'smaller; search the right half.' : 'larger; search the left half.'}`);
        if (current.flightID === target) {
            found = current;
            break;
        }
        if (current.flightID < target) low = middle + 1;
        else high = middle - 1;
    }
    if (!found) steps.push(`low (${low}) is now greater than high (${high}); stop. The ID is not in the list.`);

    const preparationNote = `Pre-processing: insertion-sort the copy by ID. ${preparationSteps.length ? preparationSteps.join(' ') : 'The ID list was already in order; no shifts were needed.'} Binary search now runs on ${sorted.length} sorted IDs.`;
    const result = found ? [flightLabel(found)] : [];
    const summary = found ? `Found ${found.flightID}: ${found.source} → ${found.destination}.` : `Flight ID ${target} was not found.`;
    return createDemoShell('flightID', flights.map(flight => flightLabel(flight)), steps, result, `${preparationNote} ${summary}`);
}

function demonstrateBubbleSort() {
    const result = flights.slice();
    const steps = [];
    for (let pass = 0; pass < result.length - 1; pass++) {
        let swapped = false;
        for (let index = 0; index < result.length - pass - 1; index++) {
            const left = result[index];
            const right = result[index + 1];
            if (left.price > right.price) {
                result[index] = right;
                result[index + 1] = left;
                swapped = true;
                steps.push(`Pass ${pass + 1}, compare ${left.flightID} (${formatPrice(left.price)}) and ${right.flightID} (${formatPrice(right.price)}): swap → ${right.flightID}, ${left.flightID}.`);
            } else {
                steps.push(`Pass ${pass + 1}, compare ${left.flightID} (${formatPrice(left.price)}) and ${right.flightID} (${formatPrice(right.price)}): already in order; no swap.`);
            }
        }
        if (!swapped) {
            steps.push(`Pass ${pass + 1} made no swaps. The list is sorted, so stop early.`);
            break;
        }
    }
    return createDemoShell(
        'price',
        flights.map(flight => flightLabel(flight, 'price')),
        steps,
        result.map(flight => flightLabel(flight, 'price')),
        `Final order: ${result.map(flight => flight.flightID).join(' → ')} (lowest fare first).`
    );
}

function demonstrateSelectionSort() {
    const result = flights.slice();
    const steps = [];
    const keyOf = flight => `${flight.flightDate}${flight.departureTime}`;
    for (let start = 0; start < result.length - 1; start++) {
        let minimum = start;
        for (let index = start + 1; index < result.length; index++) {
            const candidateIsEarlier = keyOf(result[index]) < keyOf(result[minimum]);
            steps.push(`Pass ${start + 1}: compare ${result[index].flightID} (${keyOf(result[index])}) with current minimum ${result[minimum].flightID} (${keyOf(result[minimum])}) → ${candidateIsEarlier ? 'new minimum.' : 'keep current minimum.'}`);
            if (candidateIsEarlier) minimum = index;
        }
        if (minimum !== start) {
            const first = result[start];
            const earliest = result[minimum];
            result[start] = earliest;
            result[minimum] = first;
            steps.push(`Place earliest remaining flight ${earliest.flightID} at position ${start + 1}; swap with ${first.flightID}.`);
        } else {
            steps.push(`${result[start].flightID} is already the earliest in the unsorted range; keep it at position ${start + 1}.`);
        }
    }
    if (result.length) steps.push(`The final remaining record ${result[result.length - 1].flightID} is now in its sorted position.`);
    return createDemoShell(
        'departure',
        flights.map(flight => flightLabel(flight, 'departure')),
        steps,
        result.map(flight => flightLabel(flight, 'departure')),
        `Final order: ${result.map(flight => flight.flightID).join(' → ')} (earliest date and departure time first).`
    );
}

function demonstrateMergeSort() {
    const result = flights.slice();
    const steps = [];
    const sortRange = (start, end) => {
        if (start >= end) return;
        const middle = start + Math.floor((end - start) / 2);
        steps.push(`Divide positions ${start + 1}–${end + 1}: [${result.slice(start, middle + 1).map(flight => flight.flightID).join(', ')}] and [${result.slice(middle + 1, end + 1).map(flight => flight.flightID).join(', ')}].`);
        sortRange(start, middle);
        sortRange(middle + 1, end);

        const left = result.slice(start, middle + 1);
        const right = result.slice(middle + 1, end + 1);
        let leftIndex = 0;
        let rightIndex = 0;
        let writeIndex = start;
        const merged = [];
        while (leftIndex < left.length && rightIndex < right.length) {
            if (left[leftIndex].durationMin <= right[rightIndex].durationMin) {
                merged.push(left[leftIndex++]);
            } else {
                merged.push(right[rightIndex++]);
            }
        }
        while (leftIndex < left.length) merged.push(left[leftIndex++]);
        while (rightIndex < right.length) merged.push(right[rightIndex++]);
        merged.forEach(flight => { result[writeIndex++] = flight; });
        steps.push(`Merge positions ${start + 1}–${end + 1} by duration → ${merged.map(flight => `${flight.flightID} (${formatDuration(flight.durationMin)})`).join(' · ')}.`);
    };

    if (result.length) sortRange(0, result.length - 1);
    return createDemoShell(
        'duration',
        flights.map(flight => flightLabel(flight, 'duration')),
        steps,
        result.map(flight => flightLabel(flight, 'duration')),
        `Final order: ${result.map(flight => flight.flightID).join(' → ')} (shortest duration first).`
    );
}

const demos = {
    linear: demonstrateLinearSearch,
    binary: demonstrateBinarySearch,
    bubble: demonstrateBubbleSort,
    selection: demonstrateSelectionSort,
    merge: demonstrateMergeSort
};

function updateAlgorithmControls() {
    const selected = document.getElementById('algorithmSelect').value;
    const searchAlgorithm = selected === 'linear' || selected === 'binary';
    document.getElementById('searchControls').hidden = selected !== 'linear';
    document.getElementById('binaryControls').hidden = selected !== 'binary';
    document.getElementById('runDemo').textContent = searchAlgorithm ? 'Run search trace →' : 'Run sorting trace →';

    const info = algorithmInfo[selected];
    document.getElementById('algorithmIcon').textContent = info.icon;
    document.getElementById('algorithmTitle').textContent = info.title;
    document.getElementById('algorithmDescription').textContent = info.description;
    document.getElementById('timeComplexity').textContent = info.time;
    document.getElementById('spaceComplexity').textContent = info.space;
}

async function loadFlights() {
    const button = document.getElementById('refreshData');
    button.disabled = true;
    document.getElementById('dataStatus').textContent = 'Loading current flight records…';
    try {
        const response = await FlightAPI.getAll();
        if (response.status !== 'ok') throw new Error(response.message || 'Could not load flight records.');
        flights = response.data;
        document.getElementById('flightCount').textContent = `${flights.length} ${flights.length === 1 ? 'record' : 'records'}`;
        document.getElementById('dataStatus').textContent = `Live data · ${flights.length} flight records from the C++ backend`;
        document.getElementById('demoOutput').innerHTML = '<div class="dsa-demo-placeholder"><span>↗</span><strong>Ready to trace live data.</strong><p>Choose an algorithm, set a search value if needed, then run the demonstration.</p></div>';
        document.getElementById('runDemo').disabled = false;
        showDSAAlert('Flight records refreshed.', 'success');
    } catch (error) {
        flights = [];
        document.getElementById('flightCount').textContent = '0 records';
        document.getElementById('dataStatus').textContent = 'Flight data could not be loaded';
        document.getElementById('demoOutput').innerHTML = '<div class="dsa-demo-placeholder"><strong>Connect to the local backend to run a demonstration.</strong><p>Start the app with start.bat, then refresh this page.</p></div>';
        document.getElementById('runDemo').disabled = true;
        showDSAAlert(error.message);
    } finally {
        button.disabled = false;
    }
}

document.getElementById('algorithmSelect').addEventListener('change', updateAlgorithmControls);
document.getElementById('runDemo').addEventListener('click', () => {
    try {
        if (!flights.length) throw new Error('There are no flight records to demonstrate.');
        document.getElementById('demoOutput').innerHTML = demos[document.getElementById('algorithmSelect').value]();
        document.getElementById('dsaAlert').classList.remove('show');
    } catch (error) {
        showDSAAlert(error.message);
    }
});
document.getElementById('refreshData').addEventListener('click', loadFlights);
document.getElementById('linearKey').addEventListener('change', event => {
    const examples = { source: 'Delhi', destination: 'Mumbai', airline: 'IndiGo', date: flights[0]?.flightDate || '' };
    document.getElementById('linearTarget').value = examples[event.target.value];
    document.getElementById('linearTarget').type = event.target.value === 'date' ? 'date' : 'text';
});
document.getElementById('linearTarget').addEventListener('keydown', event => {
    if (event.key === 'Enter') document.getElementById('runDemo').click();
});
document.getElementById('binaryTarget').addEventListener('keydown', event => {
    if (event.key === 'Enter') document.getElementById('runDemo').click();
});

updateAlgorithmControls();
loadFlights();
