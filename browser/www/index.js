let currentDevice = null;
let currentCharacteristics = null;
let currentState = null;

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
            log('Getting metadata...');
            currentCharacteristics = characteristics;
            return getCurrentMetadata(characteristics);
        })
        .then(metadata => {
            log('Getting state...');
            $("#metadata").text(JSON.stringify(metadata, null, 2));
            return getCurrentStates(currentCharacteristics);
        })
        .then((currentStates) => {
            log('Completed');
            $("#states").val(JSON.stringify(currentStates, null, 2));
            const { animations } = currentStates;
            const stateIds = [];
            for (const animation of animations) {
                if (!stateIds.includes(animation.animation.from_state)) {
                    stateIds.push(animation.animation.from_state);
                }
                if (!stateIds.includes(animation.animation.to_state)) {
                    stateIds.push(animation.animation.to_state);
                }
            }
            stateIds.sort();
            $("#current-state").empty();
            for (const stateId of stateIds) {
                $("#current-state").append(`<option value="${stateId}">${stateId}</option>`);
            }
            return getCurrentState(currentCharacteristics);
        })
        .then(state => {
            $("#current-state").val(state);
            log('Getting states...');
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

function switchState(state) {
    if (!currentCharacteristics) {
        log("No characteristics available");
        return;
    }
    log("Switching State...");
    setCurrentState(currentCharacteristics, state)
        .then(() => {
            log('Completed, Reading new state...');
            return getCurrentState(currentCharacteristics);
        })
        .then((state) => {
            log('Completed');
            $("#current-state").val(state);
        })
        .catch(error => {
            console.error('Argh! ' + error);
            log('Argh! ' + error);
        });
}


$(document).ready(function () {
    $('#read').attr('disabled', true);
    $('#write').attr('disabled', true);
    $('#set-state').attr('disabled', true);
    $('#current-state').attr('disabled', true);
    
    $("#connect").on('click', function () {
        connect(function() {
            $('#connect').attr('disabled', true);
            $('#read').attr('disabled', false);
            $('#write').attr('disabled', false);
            $('#set-state').attr('disabled', false);
            $('#current-state').attr('disabled', false);
        });
    });
    $("#read").on('click', function () {
        read();
    });
    $("#write").on('click', function () {
        write();
    });
    $("#set-state").on('click', function () {
        const state = $("#current-state").val();
        switchState(state);
    });
});