---
sort: 1
---

# Strip a binary before deploying

By default, *kphp* makes an unstripped binary "as is", containing debug symbols. 

Before deploying, **strip** this binary. It will become smaller and start slightly faster. 

"Stripping" is excluding debug symbols from a binary. It is done with [a native Linux utility](https://sourceware.org/binutils/docs/binutils/strip.html) named `strip`.

```tip
Do not delete the original binary! You'll need it later to [map C++ traces to PHP code](./map-cpp-trace-to-php.md)   
```
