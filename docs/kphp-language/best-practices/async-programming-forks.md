---
sort: 2
title: "Async programming: coroutines"
---

# Async programming: coroutines (forks)

KPHP supports some kind of coroutines (green threads). 
It is **not multithreading** — it is switching execution context to perform computations while another function is waiting for data.


## What is 'async' and how do you write such functions

Let's talk about the following intention. We have *$user_id* and need to load this user from DB. Also, we need to calculate the fingerprint of the request (e.g., SHA based on cookies). And return both results.

A common approach is doing like this:
```php
$user = loadUser($user_id);
$fingerprint = calculateFingerprint();
return tuple($user, $fingerprint);
```

But loading user means that we send a query to DB, wait for the response, parse the result and return it. We calculate the fingerprint only after the user is loaded.

A better way is this order:
1. Start loading user
2. While the user is loading, calculate the fingerprint
3. After calculating, wait for the loading user
4. Return the result

This can be done with `fork() + wait()`:
```php
$user_future = fork(loadUser($user_id));
$fingerprint = calculateFingerprint();
$user = wait($user_future);
return tuple($user, $fingerprint);
```

In PHP *fork()* does almost nothing, the code remains synchronous. But in KPHP it becomes parallel. 

```tip
This can remind *async/await* from other languages. This is partially true, but forks syntax is focused to be used without changing PHP code.  
So, you can call *f()* to get *T* and you can call *fork(f())* to get *future<T>*.
```


## When exactly does a forked function stop?

Mainly, forks are useful for network queries. Execution stops when a function starts waiting for a network response.

```php
function loadUser(int $user_id): ?User {
  // prepare RPC query
  $query = ['_' => 'user.load', 'user_id' => $user_id, 'allow_deleted' => true];
  // send this query to engine
  $query_id = rpc_tl_query_one(ConnectionPool::userCluster(), $query);
  // bytes are sent, wait for response
  $response = rpc_tl_query_result_one($query_id); // <--------- (1) waiting point => stop
  // parse response
  if (!is_array($response)) {
    return null;
  }
  return User::createFromEngineResponse($response); 
}
```

So, calling `fork(loadUser($user_id))` execute till **(1)**, then *loadUser()* stops, execution point positions right after *fork()* outside, and `wait()` continues from **(1)** till the end.

Let's see how it works when *loadUser()* calls other functions:
```php
// this function does not invoke network, so it is syncronous always
function prepareLoadUserQuery(int $user_id, bool $allow_deleted): array {
  return ['_' => 'user.load', 'user_id' => $user_id, 'allow_deleted' => $allow_deleted];
}

// this function can stop
function executeLoadUserQuery(array $query, $cluster) {
  $query_id = rpc_tl_query_one($cluster, $query);
  $response = rpc_tl_query_result_one($query_id); // <--------- (1) stop
  return $response;
}

// this function can't stop again, it is sync
function parseLoadUserResponse($response): ?User {
  if (!is_array($response)) {
    return null;
  }
  return User::createFromEngineResponse($response); 
}

// loadUser now looks like this 
function loadUser(int $user_id): ?User {
  $query = prepareLoadUserQuery($user_id, true);
  $response = executeLoadUserQuery($query, ConnectionPool::userCluster()); // <--------- (2) stop
  return parseLoadUserResponse($response);
}

$user_future = fork(loadUser($user_id)); 
$fingerprint = calculateFingerprint();
$user = wait($user_future);       // <--------- (3) also a stop point
return tuple($user, $fingerprint);
```

Now *loadUser()* starts executing, calls *executeLoadUserQuery()*, it stops => *loadUser()* also stops.

```tip
Functions that can stop and continue later are called **resumable**.
```


## Can I load more than one query in parallel?

Of course, you can fork many functions. Forks can be nested. You manually control waiting points — typically you *wait()* when you need it.

```php
$user_future = fork(loadUser($user_id));
$messages_future = fork(loadLastMessages($user_id));
$fingerprint = calculateFingerprint();
logRequest($user_id, $fingerprint); // it may send a query in parallel also
$user = wait($user_future);
if ($user->isAdmin()) {
  $user->loadExtendedInfo();
}
$messages = wait($messages_future);
```


## How does it work internally?

When a function stops, all local variables and are saved to a special memory.  
When a function resumes, their state is fully restored from that memory followed by JMP to dynamic address.

When a nested function stops, the execution point traverses up through the call stack, saving states of all functions, until where *fork()* was called.  
When an outer function resumes, the execution point traverses down, restoring state, and launches the next instruction, as if there were no stops at all. 

There can be many running forks in parallel. "Main thread" always exists. At the exact moment only one fork is running, others are stopped or waiting. When a fork stops, any other ready fork resumes. 


## How can I parallelize something except RPC calls?

```warning
While using KPHP proprietary, we always communicated with data sources using RPC. That's why almost all low-level resumable functions are RPC-related.  
Technically MySQL and other DBs can be combined with forks, but we never needed it for now.  
```
```tip
You can combine forks and sync executions with curl or with anything that can check "readiness", as below.
```

Besides low-level resumable functions, you can control forks from your PHP code:

<aside class="nooffset">sched_yield() — "stop me now, resume some other fork, and then resume me somewhen"</aside>

Let's say you use *curl multi* and you want to allow some another code execute while you are waiting.

```php
function dispatchAndWaitAllResponses($curlMultiHandler) {
  $active = 0;
  do {
    $status = curl_multi_exec($curlMultiHandler, $active);
    $info = curl_multi_info_read($curlMultiHandler);
    while ($info !== false) {
      /* ... */
      $info = curl_multi_info_read($curlMultiHandler);
    }

    if ($active > 0) {   
      // <----- here it is: 
      // without this line, next curl_multi_select() will block execution, 
      //   do-while will end only after all requests are processed,
      //   and dispatchAndWaitAllResponses() will be sync
      // with this line, any other fork can be resumed and do a useful CPU work,
      //   and when that other forks stops, this function will continue selecting curl
      sched_yield();
      curl_multi_select($curlMultiHandler, 0.001);
    }
  } while ($active && $status === CURLM_OK);
}
```

<aside class="nooffset">sched_yield_sleep(float $timeout) — like above, but resume me in <i>$timeout</i> seconds</aside>

```php
while (!isConnectionReady()) {
  sched_yield_sleep(0.001);
  // not sched_yield(), because if there are no other running forks, isConnectionReady() will be called infinitely 
  // not sleep(), because it would stop all work 
  // but sched_yield_sleep() — run some other forks, but resume this only after 1 ms has passed 
} 
```


## Wait queues

We have already seen examples when we *fork* and then *wait*. 
Now imagine the following: you have *$user_id* and *$friend_ids* and you are building UI for friends list, where for each friend you need to know if it is banned or not. 

Without forks, code looks like this:
```php
/**
 * @param int[] $friend_ids vector: id1, id2, ... 
 * @return bool[] hashmap: friend_id => is_banned
 */
function getIsBannedMap(int $user_id, array $friend_ids): array {
  $result_is_banned = [];
  foreach ($friend_ids as $friend_id) {
    $is_banned = isFriendBanned($user_id, $friend_id);  // <---- sync call
    $result_is_banned[$friend_id] = $is_banned; 
  }
  return $result_is_banned;
}
```

Suppose *isFriendBanned()* executes a query to an engine. This is slow: send a query, wait for the result, add to an array, send a query, wait for the result, add to an array, ... — *N* times.

We want the following: send *N* queries immediately, then as the next result is ready, add it to an array, until all results are got. While waiting, let other forks work — we want to be async. Here we use the **wait queue**:
```php
function getIsBannedMap(int $user_id, array $friend_ids): array {
  $result_is_banned = [];

  // execute N queries and save [fork_id => friend_id]
  $fork_ids = [];
  $map_fork_to_friend_id = [];
  foreach ($friend_ids as $friend_id) {
    $fork_id = fork(isFriendBanned($user_id, $friend_id));  // <------ async
    $map_fork_to_friend_id[$fork_id] = $friend_id;
    $fork_ids[] = $fork_id;
  }
 
  // create a waiting queue and pick up the next result when it's ready
  $queue = wait_queue_create($fork_ids);
  while (!wait_queue_empty($queue)) {
    $ready_fork_id = wait_queue_next($queue);  // <------ (1)
    $friend_id = $map_fork_to_friend_id[$ready_fork_id];
    $is_banned = wait($ready_fork_id);  // will return immediately
    $result_is_banned[$friend_id] = $is_banned;
  }

  return $result_is_banned;
}
```
The idea is pretty simple. Line **(1)** blocks the current function until the next result is available and switches context to the next fork ready to resume. When other forks stop (reach the end or starts waiting) and the next result is available, the current fork continues, *wait()* immediately returns since is it ready, and the loop continues.


## Forks and type system 

If *f()* returns `T`, then *fork(f())* returns `future<T>`, and *wait()* returns `?T`.

If *f()* returns `T`, then *wait_queue_create(fork(f()))* returns `future_queue<T>`, *wait_queue_next()* returns `future<T>|false`, and *wait()* returns `?T`.

So, there are 2 types related to forks: `future<T>` and `future_queue<T>`. 


## Forks and exceptions

Let's modify the previous example. Don't return *null* on a bad response, but *throw*: 
```php
function loadUser(int $user_id): User {
  $query_id = rpc_tl_query_one(ConnectionPool::userCluster(), [...]);
  $response = rpc_tl_query_result_one($query_id);

  if (!is_array($response)) {
    throw new Exception("bad response");
  }
  return User::createFromEngineResponse($response); 
}
```

When we fork *loadUser()*, it starts loading, context is switched to another fork, somewhen it resumes, somewhen it will throw... How to catch this exception?

**wait() throws an exception if it has been thrown inside a fork**:
```php
$user_future = fork(loadUser($user_id));
$fingerprint = calculateFingerprint();
$user = wait($user_future);            // <!----- here will be an exception
return tuple($user, $fingerprint);
```

So, *wait()* either returns a function return value or throws a caught exception.


## Common issues and unforkable constructs

KPHP is single-threaded: functions that just perform calculations and never call network are useless for forking, you can't parallelize CPU executions.

All functions accessible from forked down through call graph must contain resumable calls only in simple expressions. Assume that code below is inside fork, and both *loadAge1()* and *loadAge2()* are resumable:

```php
if ($a > $b && loadAge1()) {} 
// will NOT compile, because when loadAge1() is interrupted — at that exact point to resume?

// correct it like this:
$is_ab = $a > $b;
$age1 = loadAge1();
if ($is_ab && $age1) {}

// or like this, if order is executions doesn't matter
$age1 = loadAge1();
if ($a > $b && $age1) {}
```

```php
$ages = [loadAge1(), loadAge2()];
// again, the same reason: we can't resume back inside an array creation 
```

So, if you get a compilation error *"Can't compile resumable call in too complex expression"* — simplify this expression, extract variables manually.

`@kphp-sync` annotation in PHPDoc above function forces the compiler to check, that a thread can't be stopped inside this function. If it turns out to be resumable, you'll get a compilation error.

In plain PHP, *fork()* and all others are just simple polyfills, PHP code remains synchronous, so you get improvements only on KPHP. Don't forget to test forkable code well before deploying. 


## Full forks API

<aside>fork( f(): T ): future&lt;T&gt;</aside>

Executes *f()* until the first stop, returns *fork_id* that can be cast to *(int)* or used as hashmap key.

<aside>wait( future&lt;T&gt;|false ): ?T (resumable)</aside>

If fork result is ready, returns it immediately; else blocks current fork until the result is ready and lets other forks to execute; throws an exception if an exception has been thrown inside fork; can be called **only once** for the same *fork id*; if *T* is *void*, *wait()* can be called, but its result can't be assigned anywhere.

<aside>wait_synchronously( future&lt;T&gt;|false ): ?T</aside>

Like *wait()*, but not resumable (blocks the current fork and all others).

<aside>wait_multi( future&lt;T&gt;[] ): (?T)[] (resumable)</aside>

Like *wait()*, but for an array of futures with the same type.

<aside>wait_concurrently( future&lt;any&gt; ): void (resumable)</aside>

Waits for a future, which can be awaited by many forks at the same time; unlike *wait()*, which can be called only once for the same *fork id*, it doesn't clear result buffer: just ensures that a forked function is finished.

<aside>wait_queue_create( (future&lt;T&gt;|false)[] ): future_queue&lt;T&gt;</aside>

Creates a waiting queue, as described above; can be called with an empty array or existing forks.

<aside>wait_queue_push( &amp;future_queue&lt;T&gt;, future&lt;T2&gt;|false ): void</aside>

Adds a fork to an existing queue; if *T2!=T*, leads to re-inferring *T=lca(T,T2)* for this queue type.

<aside>wait_queue_empty( future_queue&lt;any&gt; ): bool</aside>

Returns if this queue is empty (no forks to wait).

<aside>wait_queue_next( future_queue&lt;T&gt; ): future&lt;T&gt;|false (resumable)</aside>

Stops the current fork until any result of the queue is ready and lets other forks to run.

<aside>wait_queue_next_synchronously( future_queue&lt;T&gt; ): future&lt;T&gt;|false</aside>

Stops everything until any result of the queue is ready (context doesn't switch to other forks).

<aside>sched_yield(): void (resumable)</aside>

Lets any other random fork to run (switches context to it), or returns immediately if no other forks exist; there was an example above.

<aside>sched_yield_sleep( float ): void (resumable)</aside>

Same as *sched_yield()*, but resumes at least after *$timeout* in seconds has passed.

<aside>get_running_fork_id(): future&lt;void&gt;</aside>

Converted to *(int)*, represents internal scheduler fork id (0 for "main thread"); can be used in caching layers to save "is this object already loading by some other fork".


```tip
## Conclusion about forks
Forks remind async/await, but they even can be used without modifying function declaration.    
Forks are especially useful combined with RPC calls. Without RPC they are not so rich for now, but technically MySQL and other DB connections can be made forkable (this is not ready yet).  
PHP code executes as usual: parallel execution is KPHP-only, so test it perfectly before deploying.
```
