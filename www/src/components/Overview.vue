<template>
  <div>
      frames: {{ frames }}
  </div>
</template>

<script>
export default {
  name: 'Overview',
  props: ['info'],
  data() {
      return {
          frames: this.info.frames || 0,
      }
  }
}
</script>