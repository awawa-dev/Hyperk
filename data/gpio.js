function setupPinValidator() {
    const hardwareLimits = {
        "ESP32-C6": { gpio: [0,1,2,3,4,5,6,7,8,10,15,18,19,20,21,22], spi: {6:5} },
        "ESP32-S3": { gpio: [1,2,4,5,6,7,8,10,16,17,18,48], spi: {11:12} }, // GPIO48 = built-in WS2812B
        "ESP32-C3": { gpio: [0,1,2,3,4,5,6,7,8,10,20,21], spi: {7:6} }, // GPIO08 = built-in WS2812B
        "ESP8266":  { gpio: [2],                       spi: {19:18} },
        "ESP32":    { gpio: null,                      spi: {23:18} },
        "ESP32-S2": { gpio: null,                      spi: {11:7} },
        "ESP32-ETH01": { gpio: [2,4,14],               spi: {4:14} },
        "ESP32-C2": { gpio: [0,1,2,3,4,5,6,7,10], spi: {7:6} },
        "ESP32-C5": { gpio: [0,1,2,3,4,5,6,7,8,10,11,27], spi: {7:6} } // GPIO27 = built-in WS2812B
    };
    
    const arch = (typeof cfgDeviceArchitecture !== 'undefined') ? cfgDeviceArchitecture : "";
    const els = { type: document.getElementById('ledType'), clkLabel: document.getElementById('clockPinLabel') };

    if (!els.type || !els.clkLabel) {
        console.warn("LED Validator: Missing required DOM elements (ledType/clockPinLabel)");
        return;
    }

    els.clkLabel.setAttribute('aria-live', 'polite');

    const setField = (name, opts) => {
        const old = document.getElementsByName(name)[0];
        if (!old) return null;
        
        const isSel = (opts != null);
        const signature = "sig_" + JSON.stringify(opts);

        if ((old.tagName === 'SELECT') === isSel && old.dataset.sig === signature) return old;

        const wasFocused = (document.activeElement === old);

        const el = isSel ? document.createElement('select') : Object.assign(document.createElement('input'), {
            type: 'number', min: 0, max: (arch.includes('8266') ? 16 : 48), step: '1'
        });
        
        el.name = name; el.id = old.id; el.required = true;
        el.dataset.sig = signature;

        if (isSel) {
            opts.forEach(p => el.add(new Option(`GPIO ${p}`, p)));
            el.value = opts.includes(parseInt(old.value)) ? old.value : opts[0];

            if (opts.length === 1) { 
                el.disabled = true;
                el.title = "Only one valid GPIO for this mode/architecture"; 
            }
        } else {
            el.value = old.value || 0;
        }

        old.replaceWith(el);
        el.addEventListener('change', updateUI); 
        
        if (wasFocused) el.focus();
        return el;
    };

    function updateUI() {
        const isSpi = els.type.value == "2";
        const cfg = hardwareLimits[arch];

        let validPins = cfg ? (isSpi ? Object.keys(cfg.spi).map(Number) : cfg.gpio) : null;
        const dataPinEditor = setField('dataPin', validPins);
        
        const autoClk = (cfg && cfg.spi) ? (cfg.spi[dataPinEditor.value] ?? null) : null;
        const clockPinEditor = setField('clockPin', ((autoClk !== null) ? [autoClk] : null)); 

        els.clkLabel.style.display = isSpi ? 'block' : 'none';
        clockPinEditor.disabled = clockPinEditor.disabled || !isSpi;
    }

    els.type.onchange = updateUI;
    updateUI(); 
};
