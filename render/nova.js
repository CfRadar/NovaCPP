// nova.js
// This script acts as the bridge between the browser DOM and our C++ backend.

// Global interval tracking to prevent duplicate polling loops during Fast Refresh
window._novaPollIntervals = window._novaPollIntervals || {};

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
                            targetEl.innerHTML = html;
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
                setupPolling(); // Re-initialize polling for components
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

// Run setup polling and HMR on initial page load
window.addEventListener('DOMContentLoaded', () => {
    setupPolling();
    initHMR();
});

// Handle browser Back/Forward buttons
window.addEventListener('popstate', () => {
    navigateTo(window.location.pathname);
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
                const html = await response.text();

                if (targetId) {
                    const el = document.getElementById(targetId);
                    if (el) {
                        el.innerHTML = html;
                        setupPolling();
                    } else {
                        console.error('NovaCPP: Target component ' + targetId + ' not found in DOM.');
                    }
                } else {
                    document.getElementById('root').innerHTML = html;
                    setupPolling();
                }
            }
        } catch (error) {
            console.error('NovaCPP fetch error:', error);
        }
    }
});
