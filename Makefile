dist/filtergrapher.wasm.js:
	mkdir -p dist && \
	emcc --bind \
	-O0 \
	-L/opt/ffmpeg/lib \
	-I/opt/ffmpeg/include/ \
	-s EXTRA_EXPORTED_RUNTIME_METHODS="[FS, cwrap, ccall, getValue, setValue, writeAsciiToMemory]" \
	-s INITIAL_MEMORY=268435456 \
	-s ASSERTIONS=1 \
	-s STACK_OVERFLOW_CHECK=2 \
	-s PTHREAD_POOL_SIZE_STRICT=2 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-lavcodec -lavformat -lavfilter -lavdevice -lswresample -lpostproc -lswscale -lavutil -lm -lx264 \
	-pthread \
	-lworkerfs.js \
	-o dist/filtergrapher.js \
	src/filtergrapher-wrapper.cpp