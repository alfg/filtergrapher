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
            const info = Module.run_filter('/work/' + file.name);

            const versions = {
                libavutil:  Module.avutil_version(),
                libavcodec:  Module.avcodec_version(),
                libavformat:  Module.avformat_version(),
            };

            const stats = FS.stat('/frame-5.ppm');
            const f = FS.readFile('/frame-5.ppm');

            // Send back data response.
            data = {
                ...info,
                file: f,
                versions,
                stats,
            }
            postMessage(data);

            // Cleanup mount.
            FS.unmount('/work');
            break;
        
        default:
            break;
    }
}
self.importScripts('filtergrapher.js'); // Load ffprobe into worker context.