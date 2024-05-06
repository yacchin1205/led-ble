const METADATA_CHARACTERISTICS_UUID = '19B10002-E8F2-537E-4F6C-D104768A1215';

async function getCurrentMetadata(characteristics) {
    console.log('Got Characteristics:', characteristics);
    const target = characteristics.find(c => c.uuid === METADATA_CHARACTERISTICS_UUID.toLowerCase());
    if (!target) {
        throw new Error('Characteristics not found.');
    }

    const metadata = await getMetadataCharacteristic(target);
    console.log('Metadata:', metadata);
    return metadata;
}

async function getMetadataCharacteristic(characteristic) {
    const value = await characteristic.readValue();
    console.log('Value:', value);
    const buffer = new DataView(value.buffer);
    const { metadata } = parseMetadata(buffer, 0);
    return metadata;
}

function parseMetadata(buffer, offset) {
    /*
    uint8_t uuid[SIZE_UUID];

    uint8_t device_name_size;
    char* device_name;

    uint8_t num_channels;
    channel_metdata_t* channels;
    */
    const uuid = [...Array(16).keys()].map((index) => buffer.getUint8(index + offset));
    offset += 16;
    const deviceNameSize = buffer.getUint8(offset);
    offset += 1;
    // device_name
    offset += 3; // 4 bytes padding
    offset += 4;

    const numChannels = buffer.getUint8(offset);
    offset += 1;
    // channels
    offset += 3; // 4 bytes padding
    offset += 4;

    const deviceName = new TextDecoder().decode(buffer.buffer.slice(offset, offset + deviceNameSize));
    offset += deviceNameSize;
    console.log('UUID:', uuid, 'Device Name Size:', deviceNameSize, 'Device Name:', deviceName, 'Number of Channels:', numChannels);

    const channels = [];
    for (let i = 0; i < numChannels; i++) {
        const channelMetadata = parseChannelMetadata(buffer, offset);
        offset = channelMetadata.offset;
        channels.push(channelMetadata.channelMetadata);
    }
    return {
        metadata: {
            uuid,
            deviceName,
            channels,
        },
        offset,
    };
}

function parseChannelMetadata(buffer, offset) {
    /*
    uint8_t type;
    uint8_t sink;

    uint8_t channel_name_size;
    char* channel_name;
    */
    const type = buffer.getUint8(offset);
    offset += 1;
    const sink = buffer.getUint8(offset);
    offset += 1;
    const channelNameSize = buffer.getUint8(offset);
    offset += 1;

    console.log('Type:', type, 'Sink:', sink, 'Channel Name Size:', channelNameSize);

    // channel_name
    offset += 1; // 4 bytes padding
    offset += 4;

    const channelName = new TextDecoder().decode(buffer.buffer.slice(offset, offset + channelNameSize));
    offset += channelNameSize;
    return {
        channelMetadata: {
            type,
            sink,
            channelName,
        },
        offset,
    };
}