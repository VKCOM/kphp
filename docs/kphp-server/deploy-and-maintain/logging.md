---
sort: 4
---

# Logging

While executing, master and worker processes write logs about requests and errors.


## KPHP server option

By default, the KPHP server writes all log messages into *stderr*. Option `_--log/-l_` allows specifying log file name.  
It's possible to specify a unique log file for each worker via '%' or '-%' in a file name, 
e.g. option `--log kphp-server-%.log` means: *kphp-server.log* for the master process, *kphp-server-0.log* for worker 0, and so on.


## Rotation

If the KPHP server master process receives *SIGUSR1*, it initiates the reopening logs process for itself and all workers. 
This feature can be used for implementing log rotation logic.


## Incoming requests logging

On each incoming request, a worker writes a one-line short message into the log file with a timestamp, URI, and some request statistics: working time, memory usage.


## Errors and warnings logging

For all errors and warnings, a detailed message with stacktrace is written into the log file.  
PHP function `error_reporting($level)` can be used for setting stacktrace type (default is 2): 

* 0 — without stacktrace;
* 1 — only function addresses (*addr2line* can be used for getting function names);
* 2 — functions names (works slowly);
* 3 — functions names via gdb (installed *gdb* is required, works super slowly).

However, the effect of *error_reporting* call can be suppressed by `--php-warnings-minimal-verbosity` option.

All warnings and error messages can be suppressed by PHP operator `@` for any expression.

In some cases, an error or warning may terminate the request or worker. 
* Request termination means, that the worker stops request immediately and sends 500 as a response
* Worker termination means, that the worker immediately calls *exit()* or *_exit()* without any response; the master process will detect, that one of the workers is dead, and create a new one    

**Runtime warnings**

The KPHP server may encounter a warning due to many reasons: 
* a subset of PHP runtime warnings/notices;
* different runtime behavior between PHP and KPHP;
* inefficient runtime operations;
* custom warning from user code.

If `--fatal-warnings/-K` is used, the warning handler tries to terminate the request, or if it is impossible, the worker is terminated.  
Otherwise, warnings don't affect request processing and worker lifetime. 

Note, that operator *'@'* suppresses warnings totally, regardless of `--fatal-warnings`.  
 
**Assertions and Critical errors**

Failed assertions are caused by bugs in the KPHP runtime library.   
Critical errors are caused by unrecoverable problems like out of memory.  
They both try to stop processing the request and recover the worker, or if it's impossible, the worker is terminated.   

Note, that operator *'@'* suppresses only logging an assertion or critical error message, the request will be terminated anyway. 

**SIGSEGV**

On SIGSEGV, a worker writes register values and terminates unless it is a stack overflow.  
In the case of stack overflow, the signal handler tries to terminate the request and recover the worker state, or if it's impossible, the worker is terminated.

Note, that *error_reporting()* and suppression don't affect signal handlers.  


## JSON logs

On each warning or error (except signals), the worker process writes a JSON-formatted line (if *'@'* isn't used). The name of the JSON log is the same as for the usual log, but with *.json* in the end.

JSON log is not a JSON file, but **each line is a correct JSON**:
```
// in reality, it's in single line 
{
  "version": 0,                 // integer from "--error-tag/-E" option 
  "type": 2,                    // 1 - error, 2 - warning, 8 - notice
  "created_at": 1601639604,     // current timestamp
  "msg": "message",             // description 
  "trace": [                    // stacktrace (function addresses)
    "0x55ba85bde74c",
    "0x55ba85b23cab",
    "0x55ba85b23b26",
    "0x55ba85c75ee7",
    "0x7f5ee7b06660"
  ],
  // @see kphp_set_context_on_error($tags, $extra_info, $env)
  "tags": ["tag1"],             // tags (string array) 
  "extra_info": {"k": "v"},     // extra info (any key-value) 
  "env": "production"           // environment
}
```

```tip
Having C++ traces, you use them to locate exact lines from PHP code: see next chapter.
```
