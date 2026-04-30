function setupPinValidator() {
    const hardwareLimits = {
        "ESP32-C6": { gpio: [0,1,2,3,4,5,6,7,8,10,15,18,19,20,21,22], spi: {5:4} },
        "ESP32-S3": { gpio: [1,2,4,5,6,7,8,10,16,17,18,48], spi: {5:4} }, // GPIO48 = built-in WS2812B
        "ESP32-C3": { gpio: [0,1,2,3,4,5,6,7,8,10,20,21], spi: {7:6} }, // GPIO08 = built-in WS2812B
        "ESP8266":  { gpio: [2],                       spi: {13:14} },
        "ESP32":    { gpio: [2,4,5,12,13,14,15,16,17,18,19,21,23,25,26,27,32,33], spi: {23:18} },
        "ESP32-S2": { gpio: [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,21,33,34,35,36,37,38,39,40,41,42,45], spi: {35:36} },
        "ESP32-ETH01": { gpio: [2,4],                  spi: {2:4} },
        "ESP32-C2": { gpio: [0,1,2,3,4,5,6,7,10],      spi: {7:6} },
        "ESP32-C5": { gpio: [0,1,2,3,4,5,6,7,8,10,11,27], spi: {7:6} }, // GPIO27 = built-in WS2812B
        "RP2040":   { gpio: null,                      spi: {19:18} },
        "RP2350":   { gpio: null,                      spi: {19:18} },
        "esp32-GLEDOPTO_GL_C_616WL": { gpio: [16, 2, 12, 14], spi: {16:2} },
        "esp32-GLEDOPTO_GL_C_615WL": { gpio: [16, 2], spi: {16:2} },
        "esp32-DOMRAEM_WLE_ADM": { gpio: [ 16, 2, 17, 18], spi: {16:2} },
        "esp32-IOTORERO_ETHERNET": { gpio: [ 5, 16, 4, 12], spi: {5:16} }
    };
    
    const arch = (typeof cfgDeviceArchitecture !== 'undefined') ? cfgDeviceArchitecture : "";
    const els = { type: document.getElementById('ledType'), addSegBtn: document.getElementById('addSegmentBtn'), segContainer: document.getElementById('segmentContainer') };

    if (!els.type || !els.addSegBtn || !els.segContainer) {
        console.warn("LED Validator: Missing required DOM elements (ledType/addSegBtn/segContainer)");
        return;
    }

    const setField = (wrapper, name, opts) => {
        const old = wrapper.querySelector(`select[name="${name}"]`);
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
        } else {
            el.value = old.value || 0;
        }

        old.replaceWith(el);
        
        if (wasFocused) el.focus();
        return el;
    };

    function setupSegments() {
        const isSpi = els.type.value == "2";
        const cfg = hardwareLimits[arch];
        const basePins = cfg ? (isSpi ? Object.keys(cfg.spi).map(Number) : cfg.gpio) : null;
        const allUsedPins = cfgSegments.map(s => parseInt(s.data, 10));

        els.segContainer.innerHTML = ''; 
        
        cfgSegments.some((segment, i) => {
            const wrapper = document.createElement('div');
            wrapper.id = `segment_${i}`;
            wrapper.className = 'segmentElement';
            
            wrapper.innerHTML = `
                <div class="grid">
                    <label>Data Pin (GPIO)
                        <select name="dataPin${i}" required>
                            <option value="${segment.data}">GPIO ${segment.data}</option>
                        </select>
                    </label>

                    ${isSpi ? `
                    <label aria-live="polite">Clock Pin (GPIO)
                        <select name="clockPin${i}">
                            <option value="${segment.clock}">GPIO ${segment.clock}</option>
                        </select>
                    </label>                    
                    ` : ''}

                    ${(cfgSegmentSupported && !isSpi && (i > 0)) ? `
                    <label class="segment-start-label" aria-live="polite">Segment Start
                        <input type="number" name="startIndex${i}" required min="0" value="${segment.startIndex}">
                    </label>                        
                    ` : ''}                        
                </div>
                
                ${(i > 0) ? `
                <div style="text-align: right; margin-top: 0.5rem;">
                    <button type="button" class="delSegmentBtn outline" data-index="${i}">
                        - Remove segment
                    </button>
                </div>
                ` : ''}
            `;
                        
            els.segContainer.appendChild(wrapper);
            
            const availablePins = (i > 0 && (arch == "RP2040" || arch == "RP2350")) ? [ Math.min((parseInt(cfgSegments[0].data, 10) + i), 22) || i ] :
                                  (basePins ? basePins.filter(p => !allUsedPins.includes(p) || p === parseInt(segment.data, 10)) : null);
            const dataPinEditor = setField(wrapper, `dataPin${i}`, availablePins);
            cfgSegments[i].data = dataPinEditor?.value || cfgSegments[i].data;
            dataPinEditor.onchange = () => {
                cfgSegments[i].data = parseInt(dataPinEditor.value, 10);
                setupSegments();
            };

            if (cfgSegmentSupported && !isSpi) {
                const startIndex = wrapper.querySelector(`input[name="startIndex${i}"]`);
                if (startIndex) {
                    startIndex.oninput = (e) => cfgSegments[i].startIndex = parseInt(e.target.value, 10) || 0;
                }
            }

            if (isSpi) {
                const autoClk = (cfg && cfg.spi && dataPinEditor.value != null) ? (cfg.spi[dataPinEditor.value] ?? null) : null;
                const clockPinEditor = setField(wrapper, `clockPin${i}`, ((autoClk !== null) ? [autoClk] : null));
                cfgSegments[i].clock = clockPinEditor?.value || cfgSegments[i].clock;
                clockPinEditor.onchange = () => cfgSegments[i].clock = parseInt(clockPinEditor.value, 10);              
            }

            return !cfgSegmentSupported || isSpi;
        });

        if (cfgSegmentSupported) {
            const delButtons = els.segContainer.querySelectorAll('.delSegmentBtn');
            delButtons.forEach(btn => {
                btn.onclick = () => {
                    const idx = parseInt(btn.getAttribute('data-index'), 10);
                    cfgSegments.splice(idx, 1);
                    setupSegments();
                };
            });
        }

        if (cfgSegmentSupported && !isSpi) {
            els.addSegBtn.style.display = 'block';
            els.addSegBtn.onclick = () => {
                if (cfgSegments.length >= cfgSegmentSupported) return;

                const currentUsed = cfgSegments.map(s => parseInt(s.data, 10));
                const freePin = basePins ? basePins.find(p => !currentUsed.includes(p)) : 0;
                
                const numLeds = parseInt(document.querySelector('input[name="numLeds"]')?.value, 10) || -1;
                let nextStart = 0;
                if (cfgSegments.length > 0 && numLeds > 0) {
                    nextStart = Math.floor((cfgSegments.at(-1).startIndex + numLeds) / 2);
                }
                
                cfgSegments.push({ 
                    data: (freePin !== undefined) ? freePin : 0,
                    clock: 0, 
                    startIndex: nextStart 
                });
                setupSegments();
            };
        }
        else {
            els.addSegBtn.style.display = 'none';
            els.addSegBtn.onclick = null;
        }        
    }

    els.type.onchange = setupSegments;
    setupSegments();
};
