<template>
  <div>
      frames: {{ frames }}

      <img v-if="image" :src="image" width="100%" />
  </div>
</template>

<script>
import Image from '../image';

export default {
  name: 'Overview',
  props: ['info'],
  computed: {
    image() {
      const data = this.info.file;
      const str = new TextDecoder().decode(data);
      const img = new Image(str);
      return img.getPNG();
    }
  },
  data() {
      return {
          frames: this.info.frames || 0,
      }
  }
}
</script>