---
sort: 2
title: vkext
---

# vkext.so (a PHP extension)

**vkext** is a PHP extension that must be installed if you plan to use TL/RPC calls.  
Without vkext, your code can be compiled, but TL/RPC won't work in plain PHP.

vkext exposes functions written in C because they are performance-critical (network layer).

```tip
If you don't need TL/RPC, you don't need vkext. You just need PHP polyfills, described earlier.
```


## Check if vkext is installed

vkext for PHP 7.4 is installed on the `php7.4-vkext` package installation. 

This package is listed in recommended of the `kphp` package, that's why you may have occasionally installed it.

How to check if vkext is installed:
```bash
php -m | grep vk_ext
```

If you see nothing, *vkext* is not installed.


## Installing vkext

```bash
apt install php7.4-vkext
```


## Installing vkext for a different PHP version

See [Compiling KPHP from sources](../../kphp-internals/developing-and-extending-kphp/compiling-kphp-from-sources.md).

