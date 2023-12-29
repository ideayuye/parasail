###命令：

```
emcc -I/home/bill/Desktop/lab/parasail-master test.c -o ./build/res.js -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["UTF8ToString", "lengthBytesUTF8", "stringToUTF8"]'
```

### cmake build:

ecmmake cmake ..
emmake make

#### single build command:

```
<!-- gennerate assembling file "xx.o" -->
emcc -I../ test.c -c -o res.o -s WASM=1

<!-- link "xxx.o" with "libparasail.a" gennerate wasm module -->
emcc res.o ../buildEM/libparasail.a -o wa_test.js  -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["UTF8ToString", "lengthBytesUTF8", "stringToUTF8"]'
```
