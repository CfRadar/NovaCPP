// nova.js
// This script acts as the bridge between the browser DOM and our C++ backend.

// Automatically fetch live components in the background
function setupPolling() {
    const pollElements = document.querySelectorAll('[nova-poll]');
    pollElements.forEach(el => {
        const intervalMs = parseInt(el.getAttribute('nova-poll'));
        const targetId = el.id;
        
        if (intervalMs && targetId && !el.hasAttribute('nova-poll-active')) {
            el.setAttribute('nova-poll-active', 'true');
            
            setInterval(async () => {
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
                    console.error('NovaCPP polling error:', error);
                }
            }, intervalMs);
        }
    });
}

// Handle SPA navigation dynamically
async function navigateTo(path) {
    try {
        const response = await fetch('/nova/navigate', {
            method: 'POST',
            headers: { 'X-Nova-Path': path },
            credentials: 'same-origin'
        });
        
        if (response.ok) {
            const html = await response.text();
            document.getElementById('root').innerHTML = html;
            setupPolling(); // Re-initialize polling for the new page
        }
    } catch (error) {
        console.error('NovaCPP navigation error:', error);
    }
}

// Run setup polling on initial page load
window.addEventListener('DOMContentLoaded', setupPolling);

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
