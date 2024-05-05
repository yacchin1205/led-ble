const STATE_CHARACTERISTICS_UUID = '19B10001-E8F2-537E-4F6C-D104768A1214';

async function getCurrentState(characteristics) {
    console.log('Got Characteristics:', characteristics);
    const target = characteristics.find(c => c.uuid === STATE_CHARACTERISTICS_UUID.toLowerCase());
    if (!target) {
        throw new Error('Characteristics not found.');
    }

    const value = await target.readValue();
    console.log('Value:', value);
    const buffer = new DataView(value.buffer);
    return buffer.getUint8(0);
}

async function setCurrentState(characteristics, state) {
    console.log('Got Characteristics:', characteristics);
    const target = characteristics.find(c => c.uuid === STATE_CHARACTERISTICS_UUID.toLowerCase());
    if (!target) {
        throw new Error('Characteristics not found.');
    }

    const buffer = new ArrayBuffer(1);
    const view = new DataView(buffer);
    view.setUint8(0, state);
    return await target.writeValue(view);
}