function setCalibration(gain, r, g, b) {
    const fields = { 'calGain': gain, 'calRed': r, 'calGreen': g, 'calBlue': b };

    for (const [name, value] of Object.entries(fields)) {
        const el = document.querySelector(`input[name="${name}"]`);
        if (el) {
            el.value = value;                  
        }
    }
}

function toggleCalibration() {
    const ledTypeSelect = document.getElementById('ledType');
    const calSection = document.getElementById('whiteCalibration');
    
    calSection.style.display = (ledTypeSelect.value === "1") ? "block" : "none";
}

function setupCalibration(){
    const ledTypeSelect = document.getElementById('ledType');
    ledTypeSelect.addEventListener('change', toggleCalibration);

    document.getElementById('cal-cold')?.addEventListener('click', () => {
        setCalibration(255, 160, 160, 160);
    });

    document.getElementById('cal-neutral')?.addEventListener('click', () => {
        setCalibration(255, 176, 176, 112);
    });    
    
    toggleCalibration();
};
