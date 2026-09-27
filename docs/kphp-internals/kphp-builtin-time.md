# Builtin execution time

For crypto and curl, `kphp_builtin_time` measures wall time from entry into
the builtin body to completion, minus explicit external network waits.
This is not CPU time or a sum of active coroutine polls. Internal mutex
contention, queueing, coroutine scheduling, IPC, serialization, component
execution and response parsing stay in the measurement. OS preemption stays
in it too. With concurrent calls this is a sum of per-call durations, not
exclusive thread utilization; overlapping calls can contribute overlapping time.

A sample is accumulated nanoseconds for one PHP request, tagged with
`is_K2`, `group` and `method`. `method=total` sums the methods in the group.
Requests that do not use a method/group emit no sample for it.
Use `avg` with `method=total` for a whole group, or choose one method.
Without a method filter, `total` and method samples are mixed; that is not
the group's average time. A method's average is per participating PHP request,
not per invocation. Workloads and call counts must match.

## Crypto

One continuous caller-side timer covers local code and the internal crypto
component. The crypto component does not perform external network I/O.
Do not pause the caller on IPC or add component time: both would change the
meaning of the metric. Crypto request/response bytes are exactly master's;
there is no timing trailer and no component-side measurement mutex.

The purpose is to measure the existing implementations, not equalize them.
Master's RSA blinding, X509 fields, cipher catalogue and other implementation
differences are deliberately preserved. No adjustment factor is applied.

## Curl network boundaries

Do not exclude `curl_easy_perform()`, `multi.action()` or other whole
libcurl operations. Their CPU work and callbacks remain measured.

* Blocking socket readiness waits: subtract the duration of libcurl's
  `poll`/`select` calls with a nonzero (or infinite) timeout. Zero-timeout
  readiness checks remain measured. The small CPU cost inside a blocking
  wait call is excluded on both sides.
* Synchronous resolver: exclude the whole libc `getaddrinfo` call, including
  its CPU work, because libc/NSS hides DNS waiting internally. The boundary
  is the same function in both runtimes.
* K2 asynchronous socket readiness/timeout: subtract Pending -> first wake,
  not Pending -> next poll. Queueing after the wake remains measured.
  Async mutex and internal channel waits are NOT classified as network.
* Legacy asynchronous curl: start network waiting after libcurl needs more
  socket events; stop when the reactor receives an event, before dispatch.
  Already-ready sockets keep queue/dispatch time in the measurement.

Legacy retains master's `curl_easy_perform()`, connection caches, callbacks,
retries and error behavior. Its bundled static libcurl has only its undefined
poll/select/getaddrinfo references redirected, using objcopy during installation,
to timing wrappers that call the original libc functions. Exclusions belong
only to the current builtin; there is no globally frozen clock. Source, configuration
and transfer algorithms are unchanged.

On Linux K2 can use system shared libcurl. The executable exports forwarding
poll/ppoll/select/pselect/getaddrinfo symbols and resolves original libc functions
with RTLD_NEXT. Attribution is scoped to the current Web request; other calls
pass through. Original arguments, return values and errno are preserved.
These synchronous shared-library hooks are currently Linux-specific.

Web responses carry `network_wait_ns`, not component execution time.
The caller subtracts it once from its continuous timer. An intermediate frame
does not charge the final trailer again. The counter is atomic, without a mutex.
IPC reader/writer and scheduler code keep master's behavior.

This measurement contract assumes calls finish and their responses are received.
Cancelled requests are outside the comparison contract. Work that continues
after builtin completion is outside its wall-time interval.
The separate regexp instrumentation still excludes PHP callback execution and
avoids double-counting nested wrappers.

## Rebuild requirements

Rebuild K2, KPHP and generated PHP application artifacts together. The new Web
trailer has different semantics from the former active-time trailer; mixed
versions give invalid measurements. Crypto reverted to master's response format.

For legacy, regenerate the CMake build and reinstall/rebuild bundled libcurl
as well as relinking the runtime/server. Recompiling just curl.cpp against the
old uninstrumented libcurl archive is insufficient. Verify the installed archive:

```sh
nm -u path/to/libcurl.a | rg 'kphp_curl_(poll|select|getaddrinfo)'
```

Tests use delayed loopback responses, callback work, delayed resolver failure,
mutex contention and delayed dispatch. They validate the measurement boundaries
and unchanged responses; they are not performance comparisons.
