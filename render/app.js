// app.js
// Client-side application logic utilizing standard npm packages via ESM import maps
import confetti from 'canvas-confetti';
import dayjs from 'dayjs';

console.log('[NovaCPP] Initialized with npm packages at:', dayjs().format('YYYY-MM-DD HH:mm:ss'));

// URL query parameter theme override (?theme=dark or ?theme=light)
const urlParams = new URLSearchParams(window.location.search);
const themeParam = urlParams.get('theme');
const savedTheme = localStorage.getItem('nova-theme');

if (themeParam === 'dark' || themeParam === 'light') {
    document.documentElement.setAttribute('data-theme', themeParam);
    localStorage.setItem('nova-theme', themeParam);
} else if (savedTheme === 'dark' || savedTheme === 'light') {
    document.documentElement.setAttribute('data-theme', savedTheme);
}

// URL query parameter ?flip=all or ?flip=clock-comp to flip cards on load
const flipParam = urlParams.get('flip');
if (flipParam) {
    const applyFlip = () => {
        if (flipParam === 'all') {
            document.querySelectorAll('.flip-card').forEach(c => c.classList.add('is-flipped'));
        } else {
            flipParam.split(',').forEach(id => {
                const card = document.querySelector(`#${id} .flip-card`);
                if (card) card.classList.add('is-flipped');
            });
        }
    };
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', applyFlip);
    } else {
        applyFlip();
    }
}

// Global Theme Toggle Function
window.toggleNovaTheme = () => {
    const current = document.documentElement.getAttribute('data-theme');
    let nextTheme = 'dark';
    if (current === 'dark') {
        nextTheme = 'light';
    } else if (current === 'light') {
        nextTheme = 'dark';
    } else {
        const isDark = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches;
        nextTheme = isDark ? 'light' : 'dark';
    }
    document.documentElement.setAttribute('data-theme', nextTheme);
    localStorage.setItem('nova-theme', nextTheme);
};

// 3D Bento Card Flip Handler
window.flipCard = (btn) => {
    const card = btn.closest('.flip-card');
    if (card) {
        card.classList.toggle('is-flipped');
    }
};

// Copy C++ Code to Clipboard
window.copyCardCode = async (btn) => {
    const card = btn.closest('.flip-card');
    if (!card) return;
    const codeEl = card.querySelector('.code-pre code');
    if (!codeEl) return;

    const codeText = codeEl.innerText || codeEl.textContent;
    try {
        await navigator.clipboard.writeText(codeText.trim());
        const originalHtml = btn.innerHTML;
        btn.innerHTML = '<span>✓ Copied!</span>';
        btn.classList.add('copied');
        setTimeout(() => {
            btn.innerHTML = originalHtml;
            btn.classList.remove('copied');
        }, 2000);
    } catch (err) {
        console.error('Failed to copy code:', err);
    }
};

// Global celebration helper callable from HTML onclick or C++ events
// Styled with calm pastel palette matching Linear/Stripe aesthetic
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

// Register in NovaEffects registry for declarative data-nova-effect="confetti"
window.NovaEffects = window.NovaEffects || {};
window.NovaEffects['confetti'] = () => window.triggerNovaConfetti();

// Auto-trigger confetti when user clicks any confetti-enabled element
document.addEventListener('click', (e) => {
    const btn = e.target.closest('.btn-confetti');
    if (btn) {
        window.triggerNovaConfetti();
    }
});
