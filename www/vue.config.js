module.exports = {
  pages: {
    index: {
      entry: 'src/main.js',
      title: 'Filtergrapher',
    }
  },
  configureWebpack: {
    devServer: {
      headers: {
        'Cross-Origin-Opener-Policy': 'same-origin',
        'Cross-Origin-Embedder-Policy': 'require-corp',
      }
    }
  }, 
};