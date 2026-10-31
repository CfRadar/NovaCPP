// app.js
// Client-side application logic utilizing standard npm packages via ESM import maps
import confetti from 'canvas-confetti';
import dayjs from 'dayjs';

console.log('[NovaCPP] Initialized with npm packages at:', dayjs().format('YYYY-MM-DD HH:mm:ss'));

// URL query parameter theme override (?theme=dark or ?theme=light)
const urlParams = new URLSearchParams(window.location.search);
const themeParam = urlParams.get('theme');
if (themeParam === 'dark' || themeParam === 'light') {
    document.documentElement.setAttribute('data-theme', themeParam);
}

// Global celebration helper callable from HTML onclick or C++ events
// Styled with calm pastel palette matching the Linear/Stripe aesthetic
window.triggerNovaConfetti = () => {
    confetti({
        particleCount: 80,
        spread: 70,
        origin: { y: 0.65 },
        colors: ['#5B7CFA', '#93C5FD', '#A7F3D0', '#DDD6FE', '#FDE68A', '#FCA5A5'],
        ticks: 200,
        gravity: 0.9,
        scalar: 0.9
    });
};

// Auto-trigger confetti when user clicks any confetti-enabled element
document.addEventListener('click', (e) => {
    const btn = e.target.closest('.btn-confetti');
    if (btn) {
        window.triggerNovaConfetti();
    }
});
