onmessage = (e) => {
    const type = e.data[0];
    const file = e.data[1];

    let data;

    switch (type) {
        case 'get_file_info':
            console.log('get_file_info');

            // Mount FS for files.
            if (!FS.analyzePath('/work').exists) {
                FS.mkdir('/work');
            }
            FS.mount(WORKERFS, { files: [file] }, '/work');

            // Call the wasm module.
            const info = Module.get_file_info('/work/' + file.name);
            console.log(info);

            // Remap streams into collection.
            // const s = [];
            // for (let i = 0; i < info.streams.size(); i++) {
            //     s.push(info.streams.get(i));
            // }

            const versions = {
                libavutil:  Module.avutil_version(),
                libavcodec:  Module.avcodec_version(),
                libavformat:  Module.avformat_version(),
            };

            const stats = FS.stat('/frame-5.ppm');
            console.log('STAT: ', stats);

            // const stream = FS.open('/frame-1.ppm', 'r');
            // const buf = new Uint8Array(stats.size);
            // FS.read(stream, buf, 0, stats.size);
            // FS.close(stream);

            const f = FS.readFile('/frame-5.ppm');
            // console.log(f);

            // Send back data response.
            data = {
                ...info,
                // streams: s,
                file: f,
                versions,
            }
            postMessage(data);

            // Cleanup mount.
            // FS.unmount('/work');
            break;
        
        default:
            break;
    }
}
self.importScripts('filtergrapher.js'); // Load ffprobe into worker context.