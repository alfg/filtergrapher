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
            <b-form-input v-model="filter" placeholder="Input a filter"></b-form-input>
            <b-input-group-append>
              <b-button @click="onRender">Apply</b-button>
            </b-input-group-append>
          </b-input-group>
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

        <img v-if="img" :src="img" width="100%" />
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
        { name: 'Video Counter (10min, unfragmented, AVC Baseline)', value: 'https://video-examples-public.s3.us-west-2.amazonaws.com/video_counter_10min_unfragmented_avc.mp4' },
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
      filter: 'eq=contrast=1.75:brightness=0.20',
      selected: 1,
    }
  },
  methods: {
    onFile(event) {
      this.$worker.onmessage = (e) => {
        this.data = e.data;
        this.renderImage(e.data.file);
      }
      this.file = event.dataTransfer ? event.dataTransfer.files[0] : event.target.files[0];
      this.$worker.postMessage([ 'run_filter', this.file, this.filter, 256000 ]); // bbb
    },
    onDownload() {
      this.showProgress = true;
      this.$worker.onmessage = (e) => {
        this.data = e.data;
        this.renderImage(e.data.file);
      }
      const xhr = new XMLHttpRequest();
      xhr.onprogress = (event) => {
        if (event.lengthComputable) {
          this.progress = parseInt(((event.loaded / event.total) * 100), 10);
        }
      }
      xhr.onload = (event) => {
        this.progress = 100;
        const file = new File([event.target.response], "file");
        this.size = file.size;
        this.$worker.postMessage([ 'run_filter', file, 0 ]);
        setTimeout(() => { this.showProgress = false; }, 2000);
      }
      xhr.open('GET', this.url, true);
      xhr.responseType = 'blob';
      xhr.send();
    },
    renderImage(data) {
      const blob = new Blob([data]);
      const reader = new FileReader();
      reader.onload = (event) => {
        const data = event.target.result;
        const img = new Image(data);
        this.img = img.getPNG();
      }
      reader.readAsBinaryString(blob);
    },
    onRender() {
      this.$worker.onmessage = (e) => {
        this.data = e.data;
        this.renderImage(e.data.file);
      }
      this.$worker.postMessage([ 'run_filter', this.file, this.filter, 256000 ]); // bbb
    },
    onFrameChange(frame) {
      this.$worker.onmessage = (e) => {
        this.renderImage(e.data);
      }
      this.$worker.postMessage([ 'load_frame', this.file, frame ]);
    },
  }
}
</script>

<style scoped>
.protocol {
  flex: 0 0 20% !important;
}
</style>