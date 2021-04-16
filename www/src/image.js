// Based on http://paulcuth.me.uk/netpbm-viewer/

class Image {
  constructor(data) {
    this._data = data;
    this._parser = null;
    this._formatter = null;

    this.width = null;
    this.height = null;

    this.init();
  }

  init() {
    const exp = /^(\S+)\s+(#.*?\n)*\s*(\d+)\s+(\d+)\s+(\d+)?\s*/;
    const match = this._data.match(exp);

    if (match) {
      const width = (this.width = parseInt(match[3], 10));
      const height = (this.height = parseInt(match[4], 10));
      const maxVal = parseInt(match[5], 10);
      const bytes = maxVal < 256 ? 1 : 2;
      const data = this._data.substr(match[0].length);

      switch (match[1]) {
        case "P1":
          this._parser = new ASCIIParser(maxVal + " " + data, bytes);
          this._formatter = new PBMFormatter(width, height);
          break;

        case "P2":
          this._parser = new ASCIIParser(data, bytes);
          this._formatter = new PGMFormatter(width, height, maxVal);
          break;

        case "P3":
          this._parser = new ASCIIParser(data, bytes);
          this._formatter = new PPMFormatter(width, height, maxVal);
          break;

        case "P4":
          this._parser = new BinaryParser(data, bytes);
          this._formatter = new PBMFormatter(width, height);
          break;

        case "P5":
          this._parser = new BinaryParser(data, bytes);
          this._formatter = new PGMFormatter(width, height, maxVal);
          break;

        case "P6":
          this._parser = new BinaryParser(data, bytes);
          this._formatter = new PPMFormatter(width, height, maxVal);
          break;

        default:
          throw new TypeError("Format not supported: ", match[1]);
      }
    } else {
      throw new TypeError("Not a Netpbm file.");
    }
  }

  getPNG() {
    const canvas = this._formatter.getCanvas(this._parser);
    return canvas.toDataURL();
  }
}

class BinaryParser {
  constructor(data, bytes) {
    this._data = data;
    this._bytes = bytes;
    this._pointer = 0;
  }

  getNextSample() {
    if (this._pointer >= this._data.length) return false;

    let val = 0;
    for (let i = 0; i < this._bytes; i++) {
      val = val * 255 + this._data.charCodeAt(this._pointer++);
    }
    return val;
  }
}

class ASCIIParser {
  constructor(data, bytes) {
    this._data = data;
    this._bytes = bytes;
    this._pointer = 0;
  }

  getNextSample() {
    if (this._pointer >= this._data.length) return false;

    let val = 0;
    for (let i = 0; i < this._bytes; i++) {
      val = val * 255 + parseInt(this._data[this._pointer++], 10);
    }
    return val;
  }
}

class PPMFormatter {
  constructor(width, height, maxVal) {
    this._width = width;
    this._height = height;
    this._maxVal = maxVal;
  }

  getCanvas(parser) {
    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");
    let img;

    canvas.width = ctx.width = this._width;
    canvas.height = ctx.height = this._height;

    img = ctx.getImageData(0, 0, this._width, this._height);

    for (let row = 0; row < this._height; row++) {
      for (let col = 0; col < this._width; col++) {
        const factor = 255 / this._maxVal;
        const r = factor * parser.getNextSample();
        const g = factor * parser.getNextSample();
        const b = factor * parser.getNextSample();
        const pos = (row * this._width + col) * 4;

        img.data[pos] = r;
        img.data[pos + 1] = g;
        img.data[pos + 2] = b;
        img.data[pos + 3] = 255;
      }
    }

    ctx.putImageData(img, 0, 0);
    return canvas;
  }
}

class PGMFormatter {
  constructor(width, height, maxVal) {
    this._width = width;
    this._height = height;
    this._maxVal = maxVal;
  }

  getCanvas(parser) {
    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");
    let img;

    canvas.width = ctx.width = this._width;
    canvas.height = ctx.height = this._height;

    img = ctx.getImageData(0, 0, this._width, this._height);

    for (let row = 0; row < this._height; row++) {
      for (let col = 0; col < this._width; col++) {
        const d = parser.getNextSample() * (255 / this._maxVal);
        const pos = (row * this._width + col) * 4;

        img.data[pos] = d;
        img.data[pos + 1] = d;
        img.data[pos + 2] = d;
        img.data[pos + 3] = 255;
      }
    }

    ctx.putImageData(img, 0, 0);
    return canvas;
  }
}

class PBMFormatter {
  constructor(width, height) {
    this._width = width;
    this._height = height;
  }

  getCanvas(parser) {
    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");
    let img;

    if (parser instanceof BinaryParser) {
      let data = "";
      let byte;
      const bytesPerLine = Math.ceil(this._width / 8);

      for (let i = 0; i < this._height; i++) {
        let line = parser._data.substr(i * bytesPerLine, bytesPerLine);
        let lineData = "";

        for (let j = 0; j < line.length; j++)
          lineData += ("0000000" + line.charCodeAt(j).toString(2)).substr(-8);
        data += lineData.substr(0, this._width);
      }

      while ((byte = parser.getNextSample()) !== false) {
        data += ("0000000" + byte.toString(2)).substr(-8);
      }

      parser = new ASCIIParser(data.split("").join(" "), 1);
    }

    canvas.width = ctx.width = this._width;
    canvas.height = ctx.height = this._height;

    img = ctx.getImageData(0, 0, this._width, this._height);

    for (let row = 0; row < this._height; row++) {
      for (let col = 0; col < this._width; col++) {
        const d = (1 - parser.getNextSample()) * 255;
        const pos = (row * this._width + col) * 4;
        img.data[pos] = d;
        img.data[pos + 1] = d;
        img.data[pos + 2] = d;
        img.data[pos + 3] = 255;
      }
    }

    ctx.putImageData(img, 0, 0);
    return canvas;
  }
}

export default Image;