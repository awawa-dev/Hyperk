document.querySelector('form[action="/save_wifi"]').addEventListener('submit', e => {
    const select = document.getElementById('ssid_select');
    const customInput = document.getElementById('ssid_custom');

    if (select.value === 'CUSTOM' && !customInput.value.trim()) {
        e.preventDefault();

        let msg = document.getElementById('wifi-error-msg');
        if (!msg) {
            msg = document.createElement('small');
            msg.id = 'wifi-error-msg';
            msg.className = 'form-error-msg';
            const container = document.getElementById('custom_ssid_wrapper');
            if (container) {
                container.appendChild(msg);
            }
        }
        msg.textContent = 'Custom SSID is required';
        customInput.classList.add('error-active');
        customInput.focus();              

        setTimeout(() => { 
            if (msg) msg.textContent = '';
            customInput.classList.remove('error-active');
        }, 5000);

        return;
    }

    const f = e.target, btn = f.querySelector('button');
    btn.setAttribute('aria-busy', 'true');
    btn.disabled = true;
});

async function scanWifi() {
    const select = document.getElementById('ssid_select');
    const customDiv = document.getElementById('custom_ssid_wrapper');

    try {
    const res = await fetch('/api/wifi_scan?t=' + Date.now());

    if (res.status === 200) {
        const nets = await res.json();

        let html = '<option value="">-- Select network --</option>';

        if (Array.isArray(nets)){
            nets.sort((a, b) => b.rssi - a.rssi).forEach(n => {
                const q = n.rssi > -50  ? 'excellent' :
                            n.rssi > -60  ? 'very good' :
                            n.rssi > -70  ? 'good'      :
                            n.rssi > -80  ? 'fair'      :
                                            'poor';
                html += `<option value="${n.ssid}">${n.ssid} (${n.rssi} dBm – ${q})</option>`;
            });
        }

        html += '<option value="CUSTOM">Custom SSID...</option>';

        const prev = select.value;
        select.innerHTML = html;

        if (prev) select.value = prev;

        scanInterval = 12000;
    }
    else if (res.status === 202) {
        if (select.options[0]) {
            select.options[0].text = "Scanning...";
        }
        scanInterval = 4000;
    }
    }
    catch (e) {
        if (select.options[0]) {
        select.options[0].text = "Connection lost";
        }
    }

    setTimeout(scanWifi, scanInterval);
};


function setupWifi() {
    const select = document.getElementById('ssid_select');
    const customDiv = document.getElementById('custom_ssid_wrapper');
    const customInput = document.getElementById('ssid_custom');

    select.addEventListener('change', () => {
        if (select.value === 'CUSTOM') {
            customDiv.style.display = 'block';
            setTimeout(() => customInput.focus(), 100);
            customInput.required = true;
        }
        else {
            customDiv.style.display = 'none';
            customInput.required = false;
            customInput.value = '';
        }
    });

    scanWifi();
};
