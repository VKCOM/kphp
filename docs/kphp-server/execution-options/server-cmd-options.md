---
sort: 1
---

# Server command-line options

Here we list command-line options, that can be passed to the compiled KPHP binary.  
You pass them directly to the binary compiled with `-M server` mode (it's the default one).  
Binaries compiled with CLI mode skip some of these options, as they are applied only to a web server.


## Options and environment variables

Unlike [compiler options](../../kphp-language/kphp-vs-php/compiler-cmd-options.md), all server-side arguments should be passed with command-line:
```bash
./server [options]
```  
They have no associated environment variables per each.


## Most useful options (basic level)

<aside>--workers-num {n} / -f {n}</aside>

The number of worker processes, **required to be set for server (not cli) mode**.  
For development and testing, 1-2 workers is okay.  
For production, you typically set this number a bit less than the number of CPU cores on the server.

<aside>--http-port {port} / -H {port}</aside>
 
A port for accepting HTTP connections, default **empty** — by default, KPHP won't listen to HTTP unless passed, so always pass this option.

<aside>--log {name} / -l {name}</aside>

A log file name, default **stderr**. '%' or '-%' can be used for writing different log files for each worker process. More info [here](../deploy-and-maintain/logging.md).

<aside>--small-acsess-log [{value}] / -U [{value}]</aside>

When used once (or with value 1), prevents writing get parameters to log files.  
When used twice (or with argument 2), prevents writing the whole request URI to log files.

<aside>--hard-memory-limit {limit} / -m {limit}</aside>
 
A memory limit for the script usage, default **512M**. To override, pass "{number}M" or "{number}G".
 
<aside>--once / -o</aside>

Runs the script only once and prints the result to stdout, without starting the server.

<aside>--define {name_value} / -D {name_value}</aside>
 
Define a variable for *ini_get()* (arg format: key=value).

<aside>--define-from-config {filename} / -i {filename}</aside>

Define variables for *ini_get()* from the config file (in form key=value on each row).

<aside>--profiler-log-prefix {prefix}</aside>
 
A prefix for the [profiler](../../kphp-language/best-practices/embedded-profiler.md) log file. When profiling is enabled, this option is mandatory.



## Not so common options (intermediate level)

<aside>--php-version</aside>

Show the compiled PHP code version and exit.  
See the [compiler option](../../kphp-language/kphp-vs-php/compiler-cmd-options.md) `--php-code-version / KPHP_PHP_CODE_VERSION`

<aside>--error-tag {filename} / -E {filename}</aside>
 
A file name with a maximum of 40 bytes contents, it is output to stderr on every warning.  
If it is numeric, it's a "version" also output to [JSON error logs](../deploy-and-maintain/logging.md), default **0**.  
This option points to a file (not just a value), because on each redeploy/rollback it's much more handy to rewrite a file, than to update startup options.

<aside>--backlog {size} / -b {size}</aside>
 
A TCP listening backlog size, default **8192**. Controls a queue size for connections waiting, described [here](../kphp-as-backend/nginx-streaming-to-kphp.md).

<aside>--connections {limit} / -c {limit}</aside>
 
A connections limit (file descriptors limit), default **65536**.

<aside>--master-port {port} / -p {port}</aside>
 
An RPC port for service queries, default **empty** — by default, KPHP does not start listening to it.  
Do not confuse it with the HTTP port! Service queries are RPC queries handled by the master process: get current statistics, get the current version, get running time, etc.

<aside>--rpc-port {port} / -r {port}</aside>

An RPC port for microservice queries, default **empty** — by default, KPHP does not start listening to it.  
Do not confuse it with the HTTP port! Queries sent to this port are described in a TL scheme with `@kphp` annotation, read more about it [here](../execution-options/microservices.md). 

<aside>--fatal-warnings / -K</aside>

Terminates request processing after the first warning, default **false**.

<aside>--cluster-name {name} / -s {name}</aside>
 
A unique name for the KPHP server, required if several KPHP servers are launched simultaneously.
  
<aside>--time-limit {limit} / -t {limit}</aside>
 
A time limit in seconds for script processing, default **30s** for server mode and **infinity** for CLI mode. To override, pass a whole number. (You can use the s/m/h/d prefix to set the limit in seconds/minutes/hours/days)

<aside>--worker-queries-to-reload {n}</aside>

The number of processed requests after which the script memory is remapped, default **100**.

<aside>--worker-memory-to-reload {n}</aside>
 
A script memory limit after which the script memory is remapped, default **infinity**. To override, pass "{number}M" or "{number}G".

<aside>--use-madvise-dontneed</aside>
 
Uses madvise `MADV_DONTNEED` for freeing script memory above the limit (disables `--worker-memory-to-reload` option).

<aside>--lock-memory / -k</aside>
 
Locks paged memory (see [mlockall](https://man7.org/linux/man-pages/man2/mlockall.2.html) `MCL_CURRENT | MCL_FUTURE`).

<aside>--static-buffers-size {limit} / -L {limit}</aside>
 
A memory limit for static buffers length (e.g. limits script output size), default **16M**. 

<aside>--instance-cache-memory-limit {limit}</aside>

A memory limit for [shared memory](../../kphp-language/best-practices/shared-memory.md) storage, default **256M**. The maximum is "4G".

<aside>--verbosity [{level}] / -v [{level}]</aside>
 
A verbosity level for logging, default **0**, in range *[0,4]*. 

<aside>--verbosity-net-events [{level}]</aside>
 
A verbosity level for logging net events, default **0**, in range *[0,4]*.

<aside>--statsd-host {host}</aside>
 
A hostname for statsd service, default **localhost**.
 
<aside>--statsd-port {port}</aside>
 
A port for statsd service, default: **8125, 14880**. By default, KPHP tries to connect to the *8125* port, and if fails, tries to connect to the *14880* port. To override, specify a port number (a single one).

<aside>--disable-statsd</aside>
 
Disables sending stats to statsd, which is enabled by default.

<aside>--user {uname} / -u {uname}</aside>

If launched from **root**, switches to this user immediately after binding the ports, default: **kitten**.  
You can explicitly specify `-u root` to disable switching.

<aside>--group {gname} / -g {gname}</aside>

Like `--user`, but controls the group name, default **kitten**. The same: makes sense only if launched from **root**.

<aside>--job-workers-ratio {ratio}</aside>

The [jobs workers](../../kphp-language/best-practices/parallelism-job-workers.md) ratio of the overall workers number, float, from 0.0 to 1.0, default **0**: no job workers are launched by default.


## Rarely used options (advanced level)

```warning
Most likely, you'll never have a need to change them.
```

<aside>--nice {level}</aside>
 
Sets a "niceness level" for the process — `nice()` C++ function from `unistd.h`

<aside>--oom-score {score}</aside>
 
Adjust OOM score (see `/proc/[pid]/oom_score_adj`).

<aside>--hostname {name}</aside>
 
The self hostname for the KPHP server.

<aside>--do-not-use-hosts</aside>
 
Disables `/etc/hosts` usage in favor of system `gethostbyname()`

<aside>--unix-socket-directory {dir}</aside>
 
A directory with UNIX sockets.
 
<aside>--bind-to-my-ipv4</aside>
 
Binds to PID the current IP address before connecting.

<aside>--force-ipv4 {arg}</aside>

Forces the KPHP server to choose the interface and IP in a given subnet. Arg format: `[iface:]ip[/subnet]`

<aside>--force-net-encryption</aside>
 
Forces encryption for client network activity, even on localhost and the same data-center.

<aside>--aes-pwd {filename}</aside>
 
A file with the AES encryption key, default **/etc/engine/pass**. This influences RPC encryption only. If a file is not found, a warning is output if verbosity is set, and no encryption is used. For localhost RPC queries, the encryption is also not used.

<aside>--tcp-buffers {arg}</aside>
 
Control TCP buffers size. Arg format: `number:size`, default **128:1k**.

<aside>--msg-buffers-size {size}</aside>
 
A memory buffers size for udp/new tcp/binlog buffers, default **256M**.

<aside>--epoll-sleep-time {microseconds}</aside>

An epoll sleep time in the main cycle (between 1us and 0.5s), by default no sleep is called at all.
 
<aside>--no-crc32c</aside>
 
Forces using CRC32 instead of CRC32-C for TCP RPC protocol.
 
<aside>--php-warnings-minimal-verbosity {level}</aside>
 
A minimum verbosity level for PHP warnings, in range of *[0,3]*, default **0**.  
Controls the minimum applied value of `error_reporting()` PHP call. 

<aside>--numa-node-to-bind {numa_node_id}:{cpus}</aside>

NUMA node description for binding workers to its cpu cores / memory. `{numa_node_id}` is a number, and `{cpus}` is a comma-separated list of node numbers or node ranges.  
Example: '0: 0-27, 55-72'.  
To set up multiple NUMA nodes, use this option more than once providing different configs.

<aside>--numa-memory-policy local|bind</aside>

NUMA memory policy for workers. Takes effect only if `--numa-node-to-bind` option is used.  
*local* — bind to a local numa node, in case out of memory take memory from the other nearest node (**default**);  
*bind* — bind to the specified node, in case out of memory raise a fatal error.


## Other options (VK.com proprietary)

When you run `./server --help`, you'll see more options than listed above.

The rest options are related to VK.com internals and therefore are unapplicable at external configurations.
