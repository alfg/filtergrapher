<template>
    <div class="file">
      <b-form-row>
        <b-col>
          <b-form-group label="Select a file" label-for="file">
            <b-input-group>
              <b-form-select class="protocol" v-model="protocol">
                <option v-for="o in protocols" :key="o.id" :value="o.value">{{ o.name }}</option>
              </b-form-select>

              <b-form-file
                  v-if="protocol === 'file'"
                  id="file"
                  accept=".mp4, .mkv"
                  v-model="file"
                  :state="Boolean(file)"
                  placeholder="Choose a file or drop it here..."
                  drop-placeholder="Drop file here..."
                  @change="onFile"
              ></b-form-file>

              <b-form-input
                v-if="protocol === 'url'"
                v-model="url"
                :state="Boolean(url)"
                placeholder="Enter a URL"
              ></b-form-input>

              <b-form-select v-if="protocol === 'example'" v-model="url">
                <template #first>
                  <b-form-select-option :value="null" disabled>-- Please select an option --</b-form-select-option>
                </template>
                <option v-for="o in examples" :key="o.id" :value="o.value">{{ o.name }}</option>
              </b-form-select>

              <b-input-group-append v-if="protocol !== 'file'">
                <b-button @click="onDownload">Download</b-button>
              </b-input-group-append>
            </b-input-group>
          </b-form-group>
        </b-col>
      </b-form-row>

      <b-progress
        class="mb-2"
        height="2px"
        v-if="showProgress"
        :value="progress"
        max="100">
      </b-progress>

      <div v-if="data">
        <div v-if="file">{{ file ? `${file.name}: ${file.size} bytes` : '' }}</div>
        <div v-else>URL: {{ url ? `${url} (${size} bytes)` : '' }}</div>

        <hr />
        <b-form-group label="Filter" label-for="filter">
          <b-input-group>
            <b-input-group-prepend is-text>
              <b-form-checkbox v-model="filterEnabled" name="check-button" switch class="mr-n2"></b-form-checkbox>
            </b-input-group-prepend>
            <b-form-input v-model="filter" placeholder="Input a filter" :state="!data.error" aria-describedby="input-filter-feedback"></b-form-input>
            <b-input-group-append>
              <b-button @click="onRender">Apply</b-button>
            </b-input-group-append>
            <b-form-invalid-feedback id="input-filter-feedback">
              {{ data.error_message }}
            </b-form-invalid-feedback>
          </b-input-group>
        </b-form-group>

        <b-form-group label="Timestamp:" label-for="timestamp">
          <b-input class="float-left col-2" v-model="timestamp" @change="onTimestampChange"></b-input>
          <b-input id="timestamp" v-model="timestamp" type="range" min="0" :max="data.duration" @change="onTimestampChange"></b-input>
          <div v-if="isLoading" class="text-center"><b-spinner small label="Spinning"></b-spinner> Loading...</div>
        </b-form-group>

        <b-form-group label="Frame" v-slot="{ ariaDescribedby }">
          <b-form-radio-group
            v-model="selected"
            :options="frames"
            :aria-describedby="ariaDescribedby"
            name="plain-inline"
            plain
            @change="onFrameChange"
          ></b-form-radio-group>
        </b-form-group>

        <img v-if="img" :src="filterEnabled ? imgFiltered : img" width="100%" />
      </div>
    </div>
</template>

<script>
import Image from '../image';

export default {
  name: 'File',
  components: {},
  computed: {
    frames() {
      const f = [];
      for (let i = 0; i < this.data.frames.length; i++) {
        f.push({
          text: this.data.frames[i].frame_number,
          value: this.data.frames[i].frame_number,
        });
      }
      return f;
    },
  },
  data() {
    return {
      protocols: [
        { name: 'File', value: 'file'},
        { name: 'URL', value: 'url'},
        { name: 'Example', value: 'example'},
      ],
      examples: [
        { name: 'Video Counter (10min, unfragmented, AVC Baseline, 3.38 MB)', value: 'https://video-examples-public.s3.us-west-2.amazonaws.com/video_counter_10min_unfragmented_avc.mp4' },
        { name: 'Tears of Steel 360p (00:12:14, unfragmented, AVC Baseline, 67.85 MB)', value: 'https://video-examples-public.s3.us-west-2.amazonaws.com/tears-of-steel-360p.mp4' },
      ],
      protocol: 'file',
      file: null,
      url: null,
      size: null,
      data: null,
      tabIndex: 0,
      progress: 0,
      showProgress: false,
      img: null,
      imgFiltered: null,
      filter: 'eq=contrast=1.75:brightness=0.20',
      selected: 1,
      timestamp: 0,
      filterEnabled: true,
      isLoading: false,
    }
  },
  methods: {
    onFile(event) {
      this.isLoading = true;
      this.timestamp = 0;
      this.$worker.onmessage = (e) => {
        this.data = e.data;
        this.renderImage(e.data.original, (img) => this.img = img);
        this.renderImage(e.data.filtered, (img) => this.imgFiltered = img);
        this.isLoading = false;
      }
      this.file = event.dataTransfer ? event.dataTransfer.files[0] : event.target.files[0];
      this.$worker.postMessage([ 'run_filter', this.file, this.filter, parseInt(this.timestamp) ]);
    },
    onDownload() {
      this.isLoading = true;
      this.timestamp = 0;
      this.showProgress = true;
      this.$worker.onmessage = (e) => {
        this.data = e.data;
        this.renderImage(e.data.original, (img) => this.img = img);
        this.renderImage(e.data.filtered, (img) => this.imgFiltered = img);
        this.isLoading = false;
      }
      const xhr = new XMLHttpRequest();
      xhr.onprogress = (event) => {
        if (event.lengthComputable) {
          this.progress = parseInt(((event.loaded / event.total) * 100), 10);
        }
      }
      xhr.onload = (event) => {
        this.progress = 100;
        this.file = new File([event.target.response], "file");
        this.size = this.file.size;
        this.$worker.postMessage([ 'run_filter', this.file, this.filter, 0 ]);
        setTimeout(() => { this.showProgress = false; }, 2000);
      }
      xhr.open('GET', this.url, true);
      xhr.responseType = 'blob';
      xhr.send();
    },
    renderImage(data, callback) {
      const blob = new Blob([data]);
      const reader = new FileReader();
      reader.onload = (event) => {
        const data = event.target.result;
        const img = new Image(data);
        const png = img.getPNG();
        return callback(png);
      }
      reader.readAsBinaryString(blob);
    },
    onRender() {
      this.isLoading = true;
      this.$worker.onmessage = (e) => {
        this.data = e.data;
        this.renderImage(e.data.original, (img) => this.img = img);
        this.renderImage(e.data.filtered, (img) => this.imgFiltered = img);
        this.isLoading = false;
      }
      this.$worker.postMessage([ 'run_filter', this.file, this.filter || 'null', parseInt(this.timestamp) ]);
    },
    onFrameChange(frame) {
      this.$worker.onmessage = (e) => {
        this.renderImage(e.data.original, (img) => this.img = img);
        this.renderImage(e.data.filtered, (img) => this.imgFiltered = img);
      }
      this.$worker.postMessage([ 'load_frame', this.file, frame ]);
    },
    onTimestampChange(value) {
      this.isLoading = true;
      this.$worker.onmessage = (e) => {
        this.data = e.data;
        this.renderImage(e.data.original, (img) => this.img = img);
        this.renderImage(e.data.filtered, (img) => this.imgFiltered = img);
        this.isLoading = false;
      }
      this.$worker.postMessage([ 'run_filter', this.file, this.filter || 'null', parseInt(value) ]);
    },
  }
}
</script>

<style scoped>
.protocol {
  flex: 0 0 20% !important;
}
</style>