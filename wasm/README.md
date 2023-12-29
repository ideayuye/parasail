###命令：

```
emcc -I/home/bill/Desktop/lab/parasail-master test.c -o ./build/res.js -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["UTF8ToString", "lengthBytesUTF8", "stringToUTF8"]'
```

### cmake build:

emcmake cmake ..
emmake make

#### single build command:

```
<!-- gennerate assembling file "xx.o" -->
emcc -I../ test.c -c -o build/res.o

<!-- link "xxx.o" with "libparasail.a" gennerate wasm module -->
emcc build/res.o ../build/libparasail.a -o build/wa_test.js  -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["UTF8ToString", "lengthBytesUTF8", "stringToUTF8"]'
```
