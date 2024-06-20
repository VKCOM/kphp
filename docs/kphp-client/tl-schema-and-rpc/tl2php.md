---
sort: 4
---

# tl2php

A console tool to generate PHP classes based on TL schema. TL schema must be beforehand compiled to *.tlo*.


## How to make the .tlo file from TL schema

*.tlo* file is a "compiled TL schema" file. You use `tlgen` to generate it from a set of **.tl* files. It is inside the `tlgen` package, that had been installed if you followed the installation instructions.  
`-ignoreGeneratedCode` *- omits generated Golang code of (de)serializers. More on that see [here](https://github.com/vkcom/tl).*
```bash
tlgen -ignoreGeneratedCode -tloPath /path/to/output.tlo input1.tl input2.tl ...
```


## How to run tl2php

The `tl2php` command is also available after the `vk-tl-tools` installation. Command-line usage is:
```
tl2php [options] /path/to/tl/scheme.tlo
```

List of available options:

<aside>--output-directory {path}</aside>

Output directory for PHP code (default '.')

<aside>--force</aside>

Overwrite output directory if exists (default false)

<aside>--gen-tl-internals</aside>

Generate PHP classes even TL functions marked `@internal` (default false)


## Output directory structure

Functions and types are named based on TL schema. 
```
---types---
memcache.not_found = memcache.Value;
memcache.str_value value:string = memcache.Value;

---functions---
memcache.get key:string = memcache.Value;
```

This will lead to the following structure:
```
output/VK/TL/
    memcache/
        Types/
            memcache_not_found.php
            memcache_str_value.php
            memcache_Value.php
        Functions/
            memcache_get.php
    RpcFunction.php 
    RpcResponse.php
```

Yes, classes from small letters — unless they are named from capitals in TL. This naming turns out to be the most useful for reading, navigation, and IDE usage. Symbols without namespace are output to *_common* directory. 


```note
Generated PHP classes are supposed to be committed to your project (though it's not good advice to store generated code under VCS, it's reliable here).
```

