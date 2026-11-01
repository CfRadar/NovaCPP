// nova.js
// This script acts as the bridge between the browser DOM and our C++ backend.

// Global interval tracking to prevent duplicate polling loops during Fast Refresh
window._novaPollIntervals = window._novaPollIntervals || {};

// Registries for declarative npm effects and visualizer widgets
window.NovaEffects = window.NovaEffects || {};
window.NovaWidgets = window.NovaWidgets || {};

// Built-in default effects
window.NovaEffects['confetti'] = function(el) {
    if (typeof window.triggerNovaConfetti === 'function') {
        window.triggerNovaConfetti();
    }
};

// Mount visual widgets (charts, pickers, 3D, code highlighters) automatically
function setupWidgets(root = document) {
    const widgets = root.querySelectorAll('[data-nova-widget]');
    widgets.forEach(el => {
        const name = el.getAttribute('data-nova-widget');
        if (window.NovaWidgets && window.NovaWidgets[name] && !el._novaWidgetMounted) {
            try {
                const propsRaw = el.getAttribute('data-nova-props') || '{}';
                const props = JSON.parse(propsRaw);
                window.NovaWidgets[name].mount(el, props);
                el._novaWidgetMounted = true;
            } catch (err) {
                console.error(`[NovaCPP] Error mounting widget '${name}':`, err);
            }
        }
    });
}

// Trigger declarative effects dispatched from C++ server responses
function triggerServerEffects(root = document) {
    const triggers = root.querySelectorAll('[data-nova-trigger-effect]');
    triggers.forEach(el => {
        const effect = el.getAttribute('data-nova-trigger-effect');
        if (window.NovaEffects && typeof window.NovaEffects[effect] === 'function') {
            window.NovaEffects[effect](el);
        }
        el.remove(); // Clean up transient trigger marker
    });
}

// Automatically fetch live components in the background
function setupPolling() {
    const pollElements = document.querySelectorAll('[nova-poll]');
    pollElements.forEach(el => {
        const intervalMs = parseInt(el.getAttribute('nova-poll'));
        const targetId = el.id;

        if (intervalMs && targetId) {
            // Clear any prior interval for this element before setting up a new one
            if (window._novaPollIntervals[targetId]) {
                clearInterval(window._novaPollIntervals[targetId]);
            }

            el.setAttribute('nova-poll-active', 'true');

            window._novaPollIntervals[targetId] = setInterval(async () => {
                try {
                    const response = await fetch('/nova/poll', {
                        method: 'POST',
                        headers: { 'X-Nova-Target': targetId },
                        credentials: 'same-origin'
                    });

                    if (response.ok) {
                        const html = await response.text();
                        const targetEl = document.getElementById(targetId);
                        if (targetEl && targetEl.innerHTML !== html) {
                            const wasFlipped = targetEl.querySelector('.flip-card.is-flipped') !== null;
                            targetEl.innerHTML = html;
                            if (wasFlipped) {
                                const newCard = targetEl.querySelector('.flip-card');
                                if (newCard) newCard.classList.add('is-flipped');
                            }
                            setupWidgets(targetEl);
                            triggerServerEffects(targetEl);
                        }
                    }
                } catch (error) {
                    // Suppress transient errors during server restart
                }
            }, intervalMs);
        }
    });
}

// Handle SPA navigation dynamically with scroll position preservation
async function navigateTo(path, isHMR = false) {
    try {
        const scrollX = window.scrollX;
        const scrollY = window.scrollY;

        const headers = { 'X-Nova-Path': path };
        if (isHMR) {
            headers['X-Nova-HMR'] = 'true';
        }

        const response = await fetch('/nova/navigate', {
            method: 'POST',
            headers: headers,
            credentials: 'same-origin'
        });

        if (response.ok) {
            const html = await response.text();
            const root = document.getElementById('root');
            if (root) {
                root.innerHTML = html;
                window.scrollTo(scrollX, scrollY);
                setupPolling();
                setupWidgets(root);
                triggerServerEffects(root);
            }
        }
    } catch (error) {
        console.error('NovaCPP navigation error:', error);
    }
}

// NovaCPP Fast Refresh & HMR Client
function initHMR() {
    if (typeof EventSource === 'undefined') return;

    let hmrSource = null;
    let isReconnecting = false;

    function connect() {
        try {
            hmrSource = new EventSource('/nova/hmr');

            hmrSource.onopen = () => {
                console.log('[Nova HMR] Connected to live reload stream.');
                if (isReconnecting) {
                    isReconnecting = false;
                    console.log('[Nova HMR] Reconnected! Fast Refreshing current view...');
                    navigateTo(window.location.pathname, true);
                }
            };

            hmrSource.addEventListener('style-update', () => {
                console.log('[Nova HMR] Hot-reloading styles...');
                const links = document.querySelectorAll('link[rel="stylesheet"]');
                links.forEach(link => {
                    const url = new URL(link.href, window.location.origin);
                    url.searchParams.set('t', Date.now());
                    link.href = url.toString();
                });
            });

            hmrSource.addEventListener('component-update', async (e) => {
                try {
                    const data = JSON.parse(e.data || '{}');
                    const targetId = data.target;
                    if (targetId) {
                        console.log(`[Nova HMR] Fast Refreshing component: #${targetId}`);
                        const response = await fetch('/nova/poll', {
                            method: 'POST',
                            headers: { 'X-Nova-Target': targetId },
                            credentials: 'same-origin'
                        });
                        if (response.ok) {
                            const html = await response.text();
                            const el = document.getElementById(targetId);
                            if (el) {
                                el.innerHTML = html;
                                setupPolling();
                                setupWidgets(el);
                                triggerServerEffects(el);
                                return;
                            }
                        }
                    }
                    navigateTo(window.location.pathname, true);
                } catch (err) {
                    console.error('[Nova HMR] Fast refresh error:', err);
                }
            });

            hmrSource.addEventListener('reload', () => {
                console.log('[Nova HMR] Fast Refreshing view on reload signal...');
                navigateTo(window.location.pathname, true);
            });

            hmrSource.onerror = () => {
                isReconnecting = true;
                if (hmrSource) {
                    hmrSource.close();
                }
                setTimeout(connect, 1000);
            };
        } catch (err) {
            setTimeout(connect, 2000);
        }
    }

    connect();
}

// Applies surgical single-target, multi-target, or full-page DOM updates
function applyDomUpdates(html, targetId, isMultiTarget) {
    if (isMultiTarget) {
        const parser = new DOMParser();
        const doc = parser.parseFromString(html, 'text/html');
        const templates = doc.querySelectorAll('template[data-nova-swap]');
        templates.forEach(tpl => {
            const id = tpl.getAttribute('data-nova-swap');
            const el = document.getElementById(id);
            if (el) {
                const wasFlipped = el.querySelector('.flip-card.is-flipped') !== null;
                el.innerHTML = tpl.innerHTML;
                if (wasFlipped) {
                    const newCard = el.querySelector('.flip-card');
                    if (newCard) newCard.classList.add('is-flipped');
                }
                setupPolling();
                setupWidgets(el);
                triggerServerEffects(el);
            }
        });
        return;
    }

    if (targetId) {
        const el = document.getElementById(targetId);
        if (el) {
            const wasFlipped = el.querySelector('.flip-card.is-flipped') !== null;
            el.innerHTML = html;
            if (wasFlipped) {
                const newCard = el.querySelector('.flip-card');
                if (newCard) newCard.classList.add('is-flipped');
            }
            setupPolling();
            setupWidgets(el);
            triggerServerEffects(el);
        } else {
            console.error('NovaCPP: Target component ' + targetId + ' not found in DOM.');
        }
    } else {
        const root = document.getElementById('root');
        if (root) {
            root.innerHTML = html;
            setupPolling();
            setupWidgets(root);
            triggerServerEffects(root);
        }
    }
}

// Run setup polling, widgets, effects, and HMR on initial page load
window.addEventListener('DOMContentLoaded', () => {
    setupPolling();
    setupWidgets();
    triggerServerEffects();
    initHMR();
});

// Handle browser Back/Forward buttons
window.addEventListener('popstate', () => {
    navigateTo(window.location.pathname);
});

// Intercept form submissions via nova-form
document.addEventListener('submit', async (e) => {
    const form = e.target.closest('[nova-form]');
    if (!form) return;

    e.preventDefault();
    const actionName = form.getAttribute('nova-form');
    const targetId = form.getAttribute('nova-target');

    const formData = new FormData(form);
    const payload = {};
    formData.forEach((value, key) => {
        if (payload.hasOwnProperty(key)) {
            if (Array.isArray(payload[key])) {
                payload[key].push(value);
            } else {
                payload[key] = [payload[key], value];
            }
        } else {
            payload[key] = value;
        }
    });

    // Also collect any elements with nova-model inside the form if not in formData
    const modeled = form.querySelectorAll('[nova-model][name]');
    modeled.forEach(input => {
        const name = input.getAttribute('name');
        if (!payload.hasOwnProperty(name)) {
            payload[name] = input.type === 'checkbox' ? (input.checked ? 'true' : 'false') : input.value;
        }
    });

    try {
        const headers = { 'Content-Type': 'application/json' };
        if (targetId) headers['X-Nova-Target'] = targetId;

        const response = await fetch('/nova/form/' + actionName, {
            method: 'POST',
            headers: headers,
            body: JSON.stringify(payload),
            credentials: 'same-origin'
        });

        if (response.ok) {
            const isMulti = response.headers.get('X-Nova-Multi-Target') === 'true';
            const html = await response.text();
            applyDomUpdates(html, targetId, isMulti);
        }
    } catch (error) {
        console.error('NovaCPP form submit error:', error);
    }
});

document.addEventListener('click', async (e) => {
    // Intercept SPA routing links
    if (e.target.hasAttribute('nova-link')) {
        e.preventDefault();
        const path = e.target.getAttribute('href');

        // Push to browser history
        window.history.pushState({}, "", path);

        // Execute the navigation
        await navigateTo(path);
        return;
    }

    // Intercept declarative npm effects
    const effectEl = e.target.closest('[data-nova-effect]');
    if (effectEl) {
        const effect = effectEl.getAttribute('data-nova-effect');
        if (window.NovaEffects && typeof window.NovaEffects[effect] === 'function') {
            window.NovaEffects[effect](effectEl);
        }
    }

    // Intercept component interactions
    if (e.target.hasAttribute('nova-click')) {
        const actionName = e.target.getAttribute('nova-click');
        const targetId = e.target.getAttribute('nova-target');

        try {
            const headers = {};
            if (targetId) headers['X-Nova-Target'] = targetId;

            const response = await fetch('/nova/action/' + actionName, {
                method: 'POST',
                headers: headers,
                credentials: 'same-origin'
            });

            if (response.ok) {
                const isMulti = response.headers.get('X-Nova-Multi-Target') === 'true';
                const html = await response.text();
                applyDomUpdates(html, targetId, isMulti);
            }
        } catch (error) {
            console.error('NovaCPP fetch error:', error);
        }
    }
});
