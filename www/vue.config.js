module.exports = {
  pages: {
    index: {
      entry: 'src/main.js',
      title: 'Filtergrapher',
    }
  },
  publicPath: process.env.NODE_ENV === 'production'
    ? '/filtergrapher/'
    : '/',
  configureWebpack: {
    devServer: {
      headers: {
        'Cross-Origin-Opener-Policy': 'same-origin',
        'Cross-Origin-Embedder-Policy': 'require-corp',
      }
    }
  }, 
};