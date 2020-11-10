---
sort: 3
title: Server
---

# KPHP internals — stage 3: server

The **Server** KPHP stage handles HTTP requests and performs master/worker orchestration.


## One master and N workers

<p class="img-c">
    <img alt="architecture" src="../../assets/img/kphp-web-server.png" width="421">
</p>

The master's and workers' responsibility area is described in an [article about the web server](../../kphp-server/kphp-as-backend/web-server.md).

Server architecture is launched if *kphp* was invoked with `-M server`. If a binary was compiled with [CLI mode](../../kphp-server/execution-options/cli-mode.md), it just executes the script and exits.


## A journey of an HTTP request

When KPHP server is launched, it starts accepting connections on a `--http-port/-H`. 
 
When a new incoming HTTP request arises, its connection is acquired by a random free worker. This works, because the master process forks after opening the port, so workers are allowed to use that descriptor. Workers that are ready (not handling a request currently) are accepting that connection.

A worker parses HTTP request (header, body, POST data, and so on) and fills superglobals like `$_SERVER` and `$_GET` in a way PHP does. 

Then it resets all static/global PHP variables to the initial state and gives execution to your PHP script — wrapper function of the *main file* passed initially to the compilation process.

Until a PHP script is finished, no response is sent. There is no "partial sending" as far as the output buffer is being filled, there is no *fastcgi_finish_request()* analog. If an error occurs, *5xx* is sent.

When a script is successfully finished, the response body is sent, the connection is closed and this worker becomes ready to accept a new request — unless a *keep-alive* header is present in an incoming response. If *keep-alive*, a worker will continue keeping this connection, waiting for the next request.

On successful execution, a log line is written into the log file. On fail, errors are logged depending on the context. Read more about logging [here](../../kphp-server/deploy-and-maintain/logging.md).


## Signals

Both master and workers handle various signals. Typically, when a signal is emerged, execution point is switched to a signal handler, unless it is inside a critical section (see `enter_critical_section() / leave_critical_section()`). If a signal is caught inside a critical section guard, it is postponed and handled immediately after an execution point leaves that critical piece of code.

If a worker is killed somehow, the master process establishes a new one.

<aside>SIGKILL</aside>

When sent to the master, immediately stops all workers and exits.

<aside>SIGTERM</aside>

When sent to the master, performs graceful shutdown: stops accepting new connections, but waits until all running workers become free, then exists. Similar to [graceful restart](../../kphp-server/deploy-and-maintain/graceful-restart.md).

<aside>SIGUSR1</aside>

When sent to the master, rotates all [logs](../../kphp-server/deploy-and-maintain/logging.md).

<aside>SIGUSR2</aside>

Raised internally inside a worker when there is no enough memory. Sends *5xx* and tries to stop the execution.  
If stopping fails, it continues and stops right after all critical sections are left. Hence, this can lead to core dumps inside critical sections, when an allocator returns *nullptr* due to out of memory. 

<aside>SIGRTMIN</aside>

Sent from master to workers to get execution statistics, see below.

<aside>SIGRTMIN + 1</aside>

Raised internally inside a worker when a critical error occurs or an assertion fails. Critical errors are situations like warnings, but unlike warnings, that are just logged — it's incomprehensible, how to resume execution: see `php_critical_error()` macros usages. Typically, they are results of incorrect PHP code, like passing unsupported flags, bad encodings, etc. Failed assertions, on the other hand, more seem like bugs in runtime: see `php_assert()` usages.  
When execution is inside a critical section, the *exit()* is called, so the worker is killed. Otherwise, a worker responds *5xx* and stops handling the current request.

<aside>SIGRTMIN + 2</aside>

Raised internally inside a worker when a stack overflow occurs. It also points to a bug in PHP code.  
Handled like the previous one.


## Sending statistics

KPHP server produces a lot of [metrics](../../kphp-server/deploy-and-maintain/statsd-metrics.md) supposed to be viewed with grafana.

The master process contains a `cron()` function, executed every second. Inside this cron, it sends `SIGRTMIN` to all workers: once a second with an argument "give me brief stats", once in 5 seconds meaning "give me full stats". Workers collect all requested stats and fill a structure passed back through the Unix pipe as raw bytes. Then the master process aggregates info from all workers and pushes metrics to *statsd*.

"Brief stats" is collected even inside a critical section. "Detailed stats" collection is postponed until a critical section ends. It contains much more info, including QPS, percentiles, and others.

