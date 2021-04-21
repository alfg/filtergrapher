onmessage = (e) => {
    const type = e.data[0];
    const file = e.data[1];

    let data;
    let f;

    switch (type) {
        case 'run_filter':
            if (FS.analyzePath('/work').exists) {
              FS.unmount('/work');
            } else {
              FS.mkdir('/work');
            }

            // Mount FS for files.
            FS.mount(WORKERFS, { files: [file] }, '/work');

            // Call the wasm module.
            const filter = e.data[2];
            const offset = e.data[3];
            console.log(filter, offset, file);
            const info = Module.run_filter('/work/' + file.name, filter, offset);

            // Remap frames into collection.
            const frames = [];
            for (let i = 0; i < info.frames.size(); i++) {
                frames.push(info.frames.get(i));
            }
            info.frames = frames;

            const versions = {
                libavutil:  Module.avutil_version(),
                libavcodec:  Module.avcodec_version(),
                libavformat:  Module.avformat_version(),
            };

            const stats = FS.stat('/frame-1.ppm');
            f = FS.readFile('/frame-1.ppm');

            // Send back data response.
            data = {
                ...info,
                file: f,
                versions,
                stats,
            }
            postMessage(data);
            break;

        case 'load_frame':
          const frame = e.data[2];
          f = FS.readFile(`/frame-${frame}.ppm`);
          postMessage(f);
          break;
        
        default:
            break;
    }
}
self.importScripts('filtergrapher.js'); // Load ffprobe into worker context.