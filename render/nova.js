// nova.js
// This script acts as the bridge between the browser DOM and our C++ backend.

// Automatically fetch live components in the background
function setupPolling() {
    const pollElements = document.querySelectorAll('[nova-poll]');
    pollElements.forEach(el => {
        const intervalMs = parseInt(el.getAttribute('nova-poll'));
        const targetId = el.id;
        
        if (intervalMs && targetId && !el.hasAttribute('nova-poll-active')) {
            // Mark it so we don't setup multiple intervals for the same component
            el.setAttribute('nova-poll-active', 'true');
            
            setInterval(async () => {
                try {
                    const response = await fetch('/nova/poll', {
                        method: 'POST',
                        headers: {
                            'X-Nova-Target': targetId
                        },
                        credentials: 'same-origin'
                    });
                    
                    if (response.ok) {
                        const html = await response.text();
                        const targetEl = document.getElementById(targetId);
                        // Only update if the DOM actually changed to prevent cursor jumps
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

// Run setup polling on initial page load
window.addEventListener('DOMContentLoaded', setupPolling);

document.addEventListener('click', async (e) => {
    // If the clicked element has a 'nova-click' attribute
    if (e.target.hasAttribute('nova-click')) {
        const actionName = e.target.getAttribute('nova-click');
        const targetId = e.target.getAttribute('nova-target'); // Check if it targets a specific component

        try {
            // Prepare headers for the request
            const headers = {};
            if (targetId) {
                // Tell the C++ server to only render this specific component!
                headers['X-Nova-Target'] = targetId;
            }

            // Send the click event to our C++ backend server
            const response = await fetch('/nova/action/' + actionName, {
                method: 'POST',
                headers: headers,
                credentials: 'same-origin' // Ensures the nova_session cookie is sent for state isolation!
            });

            if (response.ok) {
                // Get the newly rendered HTML from the C++ server
                const html = await response.text();
                
                // Surgical DOM Replacement (Like React)
                if (targetId) {
                    // Update ONLY the targeted component
                    const el = document.getElementById(targetId);
                    if (el) {
                        el.innerHTML = html;
                        setupPolling(); // Re-initialize polling inside the newly swapped HTML just in case
                    } else {
                        console.error('NovaCPP: Target component ' + targetId + ' not found in DOM.');
                    }
                } else {
                    // Fallback: Update the entire root DOM
                    document.getElementById('root').innerHTML = html;
                    setupPolling();
                }
            }
        } catch (error) {
            console.error('NovaCPP fetch error:', error);
        }
    }
});
