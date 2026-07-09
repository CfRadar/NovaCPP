document.addEventListener('DOMContentLoaded', () => {
    // We attach a global event listener to intercept all clicks on elements with a 'nova-click' attribute
    document.body.addEventListener('click', async (e) => {
        // Find the closest parent with a nova-click attribute (in case they clicked an icon inside a button)
        const target = e.target.closest('[nova-click]');
        if (!target) return;

        e.preventDefault(); // Prevent default button behavior
        
        const actionName = target.getAttribute('nova-click');
        
        try {
            // Send the click event to our C++ backend server
            const response = await fetch('/nova/action/' + actionName, {
                method: 'POST'
            });

            if (response.ok) {
                const newHtml = await response.text();
                
                // The C++ server sends back the re-rendered Virtual DOM.
                // We inject it directly into the root!
                document.getElementById('root').innerHTML = newHtml;
            } else {
                console.error("NovaCPP Server Error:", response.statusText);
            }
        } catch (error) {
            console.error("NovaCPP Network Error: Cannot reach C++ backend", error);
        }
    });
});
