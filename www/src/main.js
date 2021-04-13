import Vue from 'vue'
import BootstrapVue from 'bootstrap-vue';
import App from './App.vue'

import 'bootstrap/dist/css/bootstrap.css';
import 'bootstrap-vue/dist/bootstrap-vue.css';

const worker = new Worker('worker.js');
Vue.prototype.$worker = worker;

Vue.use(BootstrapVue);

Vue.config.productionTip = false

new Vue({
  render: h => h(App),
}).$mount('#app')
