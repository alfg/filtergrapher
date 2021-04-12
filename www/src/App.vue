<template>
  <div>
    <b-navbar type="dark" variant="dark">
      <div class="container">
        <b-navbar-nav>
          <b-nav-item href="#">
            Filtergrapher
          </b-nav-item>
        </b-navbar-nav>
      </div>
    </b-navbar>

    <GitHubCorner />

    <div id="app" class="container">
      <b-alert variant="warning" size="sm" show>
        Please use Google Chrome or Microsoft Edge.
      </b-alert>

      <hr />
      <File />
    </div>

    <footer class="container mt-4 text-center">
      <hr />
      <div class="text-muted">
        <ul>
          <li>{{ name }}-{{ version }}</li>
          <li><a href="https://github.com/alfg/filtergrapher">Source</a></li>
          <li><a href="https://github.com/alfg/filtergrapher/issues">Report Bugs</a></li>
        </ul>
      </div>
    </footer>
  </div>
</template>

<script>
import { name, version } from '../package.json';
import File from './components/File.vue'

export default {
  name: 'App',
  components: {
    File,
  },
  data() {
    return {
      name,
      version,
    }
  },
}
</script>

<style>
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  margin-top: 60px;
}

footer ul {
  display: inline-block;
  padding-left: 0;
  text-align: left;
  width: 100%;
}

footer ul li {
  display: inline;
  margin: 0 6px;
  list-style: none;
}
</style>
