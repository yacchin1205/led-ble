let currentDevice = null;
let currentCharacteristics = null;

function log(message) {
    console.log(message);
    const logElement = $("#log");
    logElement.text(logElement.text() + message + "\n");
}

function connect(callback) {
    log("Requesting Bluetooth Device...");    
    navigator.bluetooth.requestDevice({
        acceptAllDevices: true,
        optionalServices: [SERVICE_UUID.toLowerCase()]
    })
        .then(device => {
            currentDevice = device;
            log('Connecting to GATT Server...');
            return getCharacteristicsOnDevice(device);
        })
        .then(characteristics => {
            currentCharacteristics = characteristics;
            log('Getting Characteristics...');
            return getCurrentStates(characteristics);
        })
        .then((currentStates) => {
            log('Completed');
            $("#states").val(JSON.stringify(currentStates, null, 2));
            if (!callback) {
                return;
            }
            callback();
        })
        .catch(error => {
            console.error('Argh! ' + error);
            log('Argh! ' + error);
        });
}

function read() {
    if (!currentCharacteristics) {
        log("No characteristics available");
        return;
    }
    log("Reading Characteristics...");
    getCurrentStates(currentCharacteristics)
        .then((currentStates) => {
            log('Completed');
            $("#states").val(JSON.stringify(currentStates, null, 2));
        })
        .catch(error => {
            console.error('Argh! ' + error);
            log('Argh! ' + error);
        });
}

function write() {
    if (!currentCharacteristics) {
        log("No characteristics available");
        return;
    }
    log("Writing Characteristics...");
    const data = JSON.parse($("#states").val());

    setCurrentStates(currentCharacteristics, data)
        .then(() => {
            log('Completed, Reading new data...');
            return getCurrentStates(currentCharacteristics);
        })
        .then((currentStates) => {
            log('Completed');
            $("#states").text(JSON.stringify(currentStates, null, 2));
        })
        .catch(error => {
            console.error('Argh! ' + error);
            log('Argh! ' + error);
        });
}


$(document).ready(function () {
    $('#read').attr('disabled', true);
    $('#write').attr('disabled', true);
    
    $("#connect").on('click', function () {
        connect(function() {
            $('#connect').attr('disabled', true);
            $('#read').attr('disabled', false);
            $('#write').attr('disabled', false);
        });
    });
    $("#read").on('click', function () {
        read();
    });
    $("#write").on('click', function () {
        write();
    });
});