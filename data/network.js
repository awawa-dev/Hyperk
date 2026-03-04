function promptSwitchToAP() {
    const dialog = document.createElement('dialog');
    dialog.innerHTML = `
    <article>
        <h3>⚠️ Switch to AP Mode</h3>
        <p>This will reset Hyperk WiFi password and switch to AP after reboot. Are you sure?</p>
        <footer>
        <button class="secondary outline" id="ap-cancel">Cancel</button>
        <button class="contrast" id="ap-confirm">Yes, proceed</button>
        </footer>
    </article>
    `;
    document.body.appendChild(dialog);
    dialog.showModal();

    dialog.querySelector('#ap-cancel').addEventListener('click', () => {
        dialog.close();
        dialog.remove();
    });

    dialog.querySelector('#ap-confirm').addEventListener('click', async (e) => {
        const btn = e.target;
        btn.setAttribute('aria-busy', 'true');
        
        try {
            const params = new URLSearchParams();
            params.append('reset_wifi', cfgSSID);

            const res = await fetch('/save_config', { 
                method: 'POST', 
                body: params 
            });
            
            if (res.ok) {
                const data = await res.json();
                showToast(data.status === 'reboot');
            }
        } catch (err) { 
            alert("Error!"); 
        } finally {
            dialog.close();
            dialog.remove();
        }
    });
}

function setupNetwork() {
    const apBtn = document.getElementById('btn-switch-ap');
    if (apBtn) {
        apBtn.addEventListener('click', promptSwitchToAP);
    }
}
