---
sort: 3
---

# CLI mode

By default, the *kphp* command makes a binary launching as an HTTP server.
  
To compile just a console PHP script, use `-M cli` option — **while compilation** (not while execution):
```bash
kphp -M cli [...all other options] index.php
```

Then executing
```bash
./kphp_out/cli
```
Just executes your script and exits.

Lots of server command-line options are just skipped in this case.


## Console arguments, argv

You can use *$argc / $argv* superglobals in a PHP script. To pass them from the console, use
```bash
./kphp_out/cli arg1 arg2 
```

Then *$argv* will be
```
[
  0 => "/path/to/kphp_out/cli",
  1 => "arg1",
  2 => "arg2"
]
```

*$_SERVER\['argv']* will be the same — as in PHP.
