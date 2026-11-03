const navToggle = document.querySelector('.nav-toggle');

if (navToggle) {
    const navLinks = document.getElementById(navToggle.getAttribute('aria-controls'));

    navToggle.addEventListener('click', () => {
        const isOpen = navToggle.getAttribute('aria-expanded') === 'true';
        navToggle.setAttribute('aria-expanded', String(!isOpen));
        navToggle.setAttribute('aria-label', isOpen ? 'Open navigation' : 'Close navigation');
        navLinks.classList.toggle('is-open', !isOpen);
    });

    navLinks.addEventListener('click', event => {
        if (!event.target.closest('a')) return;
        navToggle.setAttribute('aria-expanded', 'false');
        navToggle.setAttribute('aria-label', 'Open navigation');
        navLinks.classList.remove('is-open');
    });

    document.addEventListener('keydown', event => {
        if (event.key !== 'Escape') return;
        navToggle.setAttribute('aria-expanded', 'false');
        navToggle.setAttribute('aria-label', 'Open navigation');
        navLinks.classList.remove('is-open');
        navToggle.focus();
    });
}
