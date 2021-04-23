onmessage = e => {
  const type = e.data[0];
  const file = e.data[1];

  let data, f;

  switch (type) {
    case "run_filter":
      if (FS.analyzePath("/work").exists) {
        FS.unmount("/work");
      } else {
        FS.mkdir("/work");
      }

      // Mount FS for files.
      FS.mount(WORKERFS, { files: [file] }, "/work");

      // Call the wasm module.
      const filter = e.data[2];
      const offset = e.data[3];
      const info = Module.run_filter("/work/" + file.name, filter, offset);

      // Remap frames into collection.
      const frames = [];
      for (let i = 0; i < info.frames.size(); i++) {
        frames.push(info.frames.get(i));
      }
      info.frames = frames;

      const versions = {
        libavutil: Module.avutil_version(),
        libavcodec: Module.avcodec_version(),
        libavformat: Module.avformat_version(),
        libavfilter: Module.avfilter_version(),
      };

      // Get first frame.
      const stats = FS.stat("/frame-1.ppm");
      f = FS.readFile("/frame-1.ppm");
      f2 = FS.readFile("/frame-1-filt.ppm");

      // Send back data response.
      data = {
        ...info,
        original: f,
        filtered: f2,
        versions,
        stats
      };
      postMessage(data);
      break;

    case "load_frame":
      const frame = e.data[2];
      f = FS.readFile(`/frame-${frame}.ppm`);
      f2 = FS.readFile(`/frame-${frame}-filt.ppm`);
      postMessage({ original: f, filtered: f2 });
      break;

    default:
      break;
  }
};
self.importScripts("filtergrapher.js"); // Load ffprobe into worker context.
