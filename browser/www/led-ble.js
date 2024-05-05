const SERVICE_UUID = '19B10000-E8F2-537E-4F6C-D104768A1214';
const CHARACTERISTICS_UUID = '19B10002-E8F2-537E-4F6C-D104768A1215';

const SIZE_ANIMATION_CHARACTERISTIC_HEADER = 1;
const SIZE_STATES_HEADER = 4;
const SIZE_ANIMATION_HEADER = 2;
const SIZE_CHANNEL_COLLECTION_HEADER = 1;
const SIZE_BASE_CHANNEL_HEADER = 2;
const SIZE_LED_CHANNEL_HEADER = 1;
const SIZE_KEYFRAME = 8;

/**
 * Get advertisement data via BLE
 *
 * @param {*} device BLE device
 * @returns BLE characteristics on the device
 */
async function getCharacteristicsOnDevice(device) {
    console.log('Got device:', device);
    /*
    device.addEventListener('advertisementreceived', (event) => {
        console.log('Advertisement received.');
        console.log('  Device:', event.device);
        console.log('  RSSI:', event.rssi);
        console.log('  TX Power:', event.txPower);
        console.log('  UUIDs:', event.uuids);
        console.log('  Manufacturer Data:', event.manufacturerData);
        console.log('  Service Data:', event.serviceData);
    });
    console.log('Watching advertisements...');
    return device.watchAdvertisements();
    */
    const server = await device.gatt.connect();

    console.log('Getting Service...');
    const service = await server.getPrimaryService(SERVICE_UUID.toLowerCase());

    console.log('Getting Characteristics...');
    return await service.getCharacteristics();
}

async function getCurrentStates(characteristics) {
    console.log('Got Characteristics:', characteristics);
    const target = characteristics.find(c => c.uuid === CHARACTERISTICS_UUID.toLowerCase());
    if (!target) {
        throw new Error('Characteristics not found.');
    }

    const { states, animations } = await getStatesCharacteristic(target);
    console.log('Got Animations:', states, animations);

    const animationDefs = [];
    for (const animation of animations) {
        const { from_state, to_state } = animation;
        console.log('From State:', from_state, 'To State:', to_state);
        const sendBuffer = createQueryBuffer(from_state, to_state);
        console.log('Sending Value...', sendBuffer);
        await target.writeValue(sendBuffer);
    
        console.log('Done.');
        await sleepPromise(500);
    
        console.log('Getting Value...');
        const value = await target.readValue();
        console.log('Got Value:', value);
        const {
            channelCollection,
            channels
        } = parseAnimationCharacteristicsBuffer(value);
        console.log('Animation:', channelCollection, channels);
        animationDefs.push({ animation, channelCollection, channels });
    }
    return {
        states,
        animations: animationDefs
    };
}

async function setCurrentStates(characteristics, data) {
    const target = characteristics.find(c => c.uuid === CHARACTERISTICS_UUID.toLowerCase());
    if (!target) {
        throw new Error('Characteristics not found.');
    }

    const sendBuffer = new ArrayBuffer(SIZE_ANIMATION_CHARACTERISTIC_HEADER + SIZE_STATES_HEADER);
    const sendView = new DataView(sendBuffer);
    // type = ANIMATION_CHARACTERISTIC_MODE_WRITE | ANIMATION_CHARACTERISTIC_TYPE_HEADER
    serializeAnimationCharacteristicHeader(sendView, 0, { type: 0x21 });
    const { states } = data;
    if (!states) {
        throw new Error('States not found.');
    }
    serializeStatesHeader(
        sendView,
        SIZE_ANIMATION_CHARACTERISTIC_HEADER,
        Object.assign({}, states, { num_animations: 0 })
    );
    console.log('Sending Value...', sendBuffer);
    await target.writeValue(sendBuffer);
    console.log('Done.');

    await sleepPromise(500);

    const updatedStatesValue = await target.readValue();
    const updatedStates = parseStatesCharacteristicBuffer(updatedStatesValue);
    console.log('Updated States:', updatedStates);
    if (updatedStates.states.initial_state !== states.initial_state) {
        throw new Error('Failed to update states.');
    }
    if (updatedStates.states.delay_msec !== states.delay_msec) {
        throw new Error('Failed to update states.');
    }

    const { animations } = data;
    if (!animations) {
        throw new Error('Animations not found.');
    }
    for (const animationDef of animations) {
        const { animation, channels } = animationDef;
        if (!animation) {
            throw new Error('Animation not found.');
        }
        if (animation.from_state === undefined || animation.to_state === undefined) {
            throw new Error(`Invalid animation: ${animation.from_state}, ${animation.to_state}`);
        }
        if (!channels) {
            throw new Error('Channels not found.');
        }
        let channelSize = 0;
        for (const { channel, keyframes } of channels) {
            if (!channel) {
                throw new Error('Channel not found.');
            }
            if (!keyframes) {
                throw new Error('Keyframes not found.');
            }
            channelSize += SIZE_BASE_CHANNEL_HEADER + SIZE_LED_CHANNEL_HEADER + keyframes.length * SIZE_KEYFRAME;
        }
        const sendAnimationBuffer = new ArrayBuffer(
            SIZE_ANIMATION_CHARACTERISTIC_HEADER +
            SIZE_STATES_HEADER +
            SIZE_ANIMATION_HEADER +
            SIZE_CHANNEL_COLLECTION_HEADER +
            channelSize
        );
        const sendAnimationView = new DataView(sendAnimationBuffer);
        // type = ANIMATION_CHARACTERISTIC_MODE_WRITE | ANIMATION_CHARACTERISTIC_TYPE_HEADER
        let offset = serializeAnimationCharacteristicHeader(sendAnimationView, 0, { type: 0x22 });
        offset = serializeStatesHeader(
            sendAnimationView,
            offset,
            { delay_msec: 0, initial_state: 0, num_animations: 1 }
        );
        offset = serializeAnimationHeader(
            sendAnimationView,
            offset,
            animation
        );
        offset = serializeChannelCollectionHeader(
            sendAnimationView,
            offset,
            { num_channels: channels.length }
        );
        for (const { channel, keyframes } of channels) {
            if (!channel) {
                throw new Error('Channel not found.');
            }
            if (!keyframes) {
                throw new Error('Keyframes not found.');
            }
            offset = serializeLEDChannelHeader(
                sendAnimationView,
                offset,
                Object.assign({}, channel, {
                    base: {
                        type: 0x01,
                        buffer_size: keyframes.length * SIZE_KEYFRAME
                    }
                })
            );
            for (let i = 0; i < keyframes.length; i++) {
                offset = serializeKeyframe(
                    sendAnimationView,
                    offset,
                    keyframes[i]
                );
            }
        }

        console.log('Sending Animation Value...', sendAnimationBuffer);
        await target.writeValue(sendAnimationBuffer);
        console.log('Done.');

        await sleepPromise(500);

        console.log('Getting Animation Value...');
        const updatedAnimationValue = await target.readValue();
        console.log('Got Animation Value:', updatedAnimationValue);
        const updatedAnimation = parseAnimationCharacteristicsBuffer(updatedAnimationValue);
        console.log('Updated Animation:', updatedAnimation);
    }
}

async function getStatesCharacteristic(targetCharacteristics) {
    const sendBuffer = new ArrayBuffer(SIZE_ANIMATION_CHARACTERISTIC_HEADER);
    const sendView = new DataView(sendBuffer);
    // type = ANIMATION_CHARACTERISTIC_MODE_READ | ANIMATION_CHARACTERISTIC_TYPE_HEADER
    serializeAnimationCharacteristicHeader(sendView, 0, { type: 0x11 });
    
    console.log('Sending Value...', sendBuffer);
    await targetCharacteristics.writeValue(sendBuffer);

    await sleepPromise(500);

    console.log('Getting Value...');
    const value = await targetCharacteristics.readValue();
    console.log('Got Value:', value);
    return parseStatesCharacteristicBuffer(value);
}

function sleepPromise(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function serializeAnimationCharacteristicHeader(buffer, offset, header) {
    /*
        // Serialize the buffer
        typedef struct __animation_characteristic_header_t {
            uint8_t type;
        }
    */
    buffer.setUint8(0 + offset, header.type);
    return SIZE_ANIMATION_CHARACTERISTIC_HEADER + offset;
}

function serializeStatesHeader(buffer, offset, header) {
    /*
        // Serialize the buffer
        typedef struct __states_header_t {
            uint16_t delay_msec;
            uint8_t initial_state;
            uint8_t num_animations;
        }
    */
    buffer.setUint16(0 + offset, header.delay_msec, true);
    buffer.setUint8(2 + offset, header.initial_state);
    buffer.setUint8(3 + offset, header.num_animations);
    return SIZE_STATES_HEADER + offset;
}

function serializeAnimationHeader(buffer, offset, header) {
    /*
        // Serialize the buffer
        typedef struct __animation_header_t {
            uint8_t from_state;
            uint8_t to_state;
        }
    */
    buffer.setUint8(0 + offset, header.from_state);
    buffer.setUint8(1 + offset, header.to_state);
    return SIZE_ANIMATION_HEADER + offset;
}

function serializeChannelCollectionHeader(buffer, offset, header) {
    /*
        // Serialize the buffer
        typedef struct __channel_collection_header_t {
            uint8_t num_channels;
        }
    */
    buffer.setUint8(offset, header.num_channels);
    return SIZE_CHANNEL_COLLECTION_HEADER + offset;
}

function serializeBaseChannelHeader(buffer, offset, header) {
    /*
        // Serialize the buffer
        typedef struct __base_channel_header_t {
            uint8_t type;
            uint8_t buffer_size;
        }
    */
    buffer.setUint8(offset, header.type);
    buffer.setUint8(offset + 1, header.buffer_size);
    return SIZE_BASE_CHANNEL_HEADER + offset;
}

function serializeLEDChannelHeader(buffer, offset, header) {
    /*
        // Serialize the buffer
        typedef struct __led_channel_header_t {
            base_channel_header_t base;
            uint8_t pin;
        }
    */
    const baseOffset = serializeBaseChannelHeader(buffer, offset, header.base);
    buffer.setUint8(baseOffset, header.pin);
    return SIZE_LED_CHANNEL_HEADER + baseOffset;
}

function serializeKeyframe(buffer, offset, keyframe) {
    /*
        // Serialize the buffer
        typedef struct __keyframe_t {
            uint8_t type;
            uint16_t frame;
            float value;
        }
    */
    buffer.setUint8(offset, keyframe.type);
    buffer.setUint16(offset + 2, keyframe.frame, true);
    buffer.setFloat32(offset + 4, keyframe.value, true);
    return SIZE_KEYFRAME + offset
}

function createQueryBuffer(fromState, toState) {
    const sendBuffer = new ArrayBuffer(2 + 4 + 2);
    const sendView = new DataView(sendBuffer);
    const headerOffset = serializeAnimationCharacteristicHeader(sendView, 0, { type: 0x12 });
    const statesOffset = serializeStatesHeader(
        sendView,
        headerOffset,
        { delay_msec: 0, initial_state: 0, num_animations: 1 }
    );
    serializeAnimationHeader(sendView, statesOffset, { from_state: fromState, to_state: toState });
    return sendBuffer;
}

function parseAnimationCharacteristicHeader(buffer, offset) {
    /*
        // Parse the buffer
        typedef struct __animation_characteristic_header_t {
            uint8_t type;
        } animation_characteristic_header_t;
    */
    const header = {
        type: buffer.getUint8(0 + offset),
    };
    return {header, offset: 1} 
}

function parseStatesHeader(buffer, offset) {
    /*
        // Parse the buffer
        typedef struct __states_header_t {
            uint16_t delay_msec;
            uint8_t initial_state;
            uint8_t num_animations;
        }
    */
    const states = {
        delay_msec: buffer.getUint16(0 + offset, true),
        initial_state: buffer.getUint8(2 + offset),
        num_animations: buffer.getUint8(3 + offset)
    };
    return {states, offset: 4 + offset};
}

function parseAnimationHeader(buffer, offset) {
    /*
        // Parse the buffer
        typedef struct __animation_header_t {
            uint8_t from_state;
            uint8_t to_state;
        }
    */
    const animation = {
        from_state: buffer.getUint8(0 + offset),
        to_state: buffer.getUint8(1 + offset)
    };
    return {animation, offset: 2 + offset};
}

function parseChannelCollectionHeader(buffer, offset) {
    /*
        // Parse the buffer
        typedef struct __channel_collection_header_t {
            uint8_t num_channels;
        }
    */
    const channelCollection = {
        num_channels: buffer.getUint8(offset)
    };
    return {channelCollection, offset: SIZE_CHANNEL_COLLECTION_HEADER + offset};
}

function parseBaseChannelHeader(buffer, offset) {
    /*
        // Parse the buffer
        typedef struct __base_channel_header_t {
            uint8_t type;
            uint8_t buffer_size;
        }
    */
    const baseChannel = {
        type: buffer.getUint8(offset),
        buffer_size: buffer.getUint8(offset + 1)
    };
    return {baseChannel, offset: SIZE_BASE_CHANNEL_HEADER + offset};
}

function parseLEDChannelHeader(baseChannel, buffer, offset) {
    /*
        // Parse the buffer
        typedef struct __led_channel_header_t {
            base_channel_header_t base;
            uint8_t pin;
        }
    */
    const ledChannel = {
        base: baseChannel,
        pin: buffer.getUint8(offset)
    };
    return {ledChannel, offset: SIZE_LED_CHANNEL_HEADER + offset};
}

function parseKeyframe(buffer, offset) {
    /*
        // Parse the buffer
        typedef struct __keyframe_t {
            uint8_t type;
            uint16_t frame;
            float value;
        }
    */
    const keyframe = {
        type: buffer.getUint8(offset),
        frame: buffer.getUint16(offset + 2, true),
        value: buffer.getFloat32(offset + 4, true)
    };
    return {keyframe, offset: SIZE_KEYFRAME + offset};
}

function parseStatesCharacteristicBuffer(value) {
    const buffer = new DataView(value.buffer);
    const { header, offset: headerOffset } = parseAnimationCharacteristicHeader(buffer, 0);
    console.log('Header:', header);
    if (header.type !== 0x01) {
        throw new Error(`Invalid header type. ${header.type}`);
    }
    const { states, offset: statesOffset } = parseStatesHeader(buffer, headerOffset);
    const animations = [];
    let offset = statesOffset;
    for (let i = 0; i < states.num_animations; i++) {
        const { animation, offset: animationOffset } = parseAnimationHeader(buffer, offset);
        animations.push(animation);
        offset = animationOffset;
    }
    return {
        states,
        animations,
    };
}

function parseAnimationCharacteristicsBuffer(value) {
    const buffer = new DataView(value.buffer);
    const { header, offset: headerOffset } = parseAnimationCharacteristicHeader(buffer, 0);
    console.log('Header:', header);
    if (header.type !== 0x02) {
        throw new Error(`Invalid header type. ${header.type}`);
    }
    const { states, offset: statesOffset } = parseStatesHeader(buffer, headerOffset);
    console.log('States:', states);

    const { animation, offset: animationOffset } = parseAnimationHeader(buffer, statesOffset);
    console.log('Animation:', animation);

    const { channelCollection, offset: channelCollectionOffset } = parseChannelCollectionHeader(buffer, animationOffset);
    let offset = channelCollectionOffset;
    console.log('Animation Header:', animation, 'Channel Collection:', channelCollection);

    const channels = [];
    for (let i = 0; i < channelCollection.num_channels; i++) {
        const { baseChannel, offset: baseChannelOffset } = parseBaseChannelHeader(buffer, offset);
        offset = baseChannelOffset;
        let channel = null;
        const keyframes = [];
        if (baseChannel.type === 0x01) {
            const { ledChannel, offset: ledChannelOffset } = parseLEDChannelHeader(baseChannel, buffer, offset);
            channel = ledChannel;
            offset = ledChannelOffset;

            if (baseChannel.buffer_size % SIZE_KEYFRAME !== 0) {
                throw new Error(`Invalid buffer size. ${baseChannel.buffer_size}`);
            }
            for (let j = 0; j < baseChannel.buffer_size / SIZE_KEYFRAME; j++) {
                const { keyframe, offset: keyframeOffset } = parseKeyframe(buffer, offset);
                keyframes.push(keyframe);
                offset = keyframeOffset;
            }
            console.log('LED Channel:', ledChannel, 'Keyframes:', keyframes);
        } else {
            throw new Error(`Invalid channel type. ${baseChannel.type}`);
        }

        channels.push({ channel, keyframes });
    }
    return {
        states,
        animation,
        channelCollection,
        channels,
    };
}
