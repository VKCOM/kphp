---
sort: 5
---

# Mapping C++ trace to PHP code

You get C++ trace when:
* `debug_backtrace()` is called from any place of your code (its return value differs from PHP)
* a runtime warning is triggered (its trace is output to error logs) 

Using this trace and **unstripped binary**, you can match it with PHP trace quite precisely.


## Step 1: use addr2line to get C++ location

C++ trace looks like this:
```
0x5bb974
0x4ad80d
0x4ad094
...
```

You can use each of these addresses as an argument to `addr2line` Linux utility:
```bash
$ addr2line -e /path/to/unstripped/kphp/server/binary 0x5bb974
/var/build/kphp/o_76/demoService.cpp:342
``` 

Having C++ location and codegenerated sources, locate an exact PHP line.


## Step 2: use C++ location to get PHP line

While codegenerating C++ sources, KPHP **inserts PHP code as comments** to C++ sources:
```cpp
//11:  function run() {
void f$SomeClass$$run(class_instance<C$SomeClass> const &v$this) {
  array < mixed > v$arg;
//12:    $arg = $this->a;
  v$arg = v$this->a;
//13:    if (!is_array($arg)) {
  if (!f$is_array(v$arg)) {
//14:      $arg = [$arg];
    SAFE_SET_OP (v$arg, =, array < array < mixed > >::create(v$arg), array < array < mixed > >);
//15:    }
  };
  /* ... */
}
```

Thus, having a C++ line, you step upper until *//\d+:* comment is met, and it's the corresponding PHP line.


## How to organize error monitoring in a KPHP project

When there are many servers with KPHP instances, it's a bit complicated to collect errors from all servers.

```tip
This documentation describes a possible approach, though it's not the only way.
```

* Store unstripped binaries in a single place, use only stripped binaries for production
* Use JSON logs: it's more handy than *stderr*
* Optionally, call `kphp_set_context_on_error()` from PHP code to provide more info to JSON logs
* On each KPHP server, organize a daemon, which pushes new errors from JSON files to single storage (Clickhouse / MySQL / etc.)
* On the server, where an unstripped binary is stored, organize a daemon, that checks for new errors in this storage, decrypts cached traces with *addr2line* (better also launched daemonized), and pushes fulfilled errors to Sentry or some other place
* Use Sentry or some other UI to view aggregated errors, filter them, assign, etc.
* Organize versioning in a way to easier filter out newly occurring errors with your UI
* Don't forget to collect logs from master processes' stderr (not related to PHP code)
* Use the same UI to aggregate PHP errors / JS errors / etc.

