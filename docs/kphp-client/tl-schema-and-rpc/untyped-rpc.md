---
sort: 2
---

# Untyped RPC

"Untyped" means that you don't need any PHP classes representing TL schema: you must use `mixed[]` hashmaps to send and receive queries.

Slightly easier to start, generally it is not a recommended practice: typed RPC works much more performant, especially for huge amounts of traffic and vectors of numbers.


## Let's start with an example

Suppose we have a *'messages'* engine that can execute the following query:
```
---types---
messages.inviteResult user_id:int already_in_chat:bool = messages.InviteResult;

---functions---
messages.inviteUsersToChat chat_id:long user_ids:%(Vector int) silent:bool = Vector %messages.InviteResult; 
```

Here is how we send a request:
```php
// this will be mixed[]
$query = [
  '_'        => 'messages.inviteUsersToChat',  // function name
  'chat_id'  => 123456,                        // long in TL  => int in PHP
  'user_ids' => [190, 336098765],              // Vector int in TL => int[] in PHP
  'silent'   => false,                         // bool in TL  => bool in PHP
];

$connection = new_rpc_connection($host, $port);
$query_id = rpc_tl_query_one($connection, $query);  // int
```

Here is how we get and parse the response:
```php
$response = rpc_tl_query_result_one($query_id);     // mixed[]
// check for errors
if (isset($response['__error'])) {
  // __error, __error_code
}
// ['result'] is valid unless error
$result = $response['result'];   // mixed
// it represents TL structure, here — Vector %messages.InviteResult, an array of arrays (hashmaps)
foreach ($result as $row) {      // we know, that $row is messages.InviteResult  
  $row['user_id'];               // mixed (int at runtime)
  $row['already_in_chat'];       // mixed (bool at runtime)
}
```


## Non-associative queries

Above, we assigned *$query* to 
```php
$query = ['_' => '...', 'chat_id' => 123456, /* ... */ ];
```

If to use a *vector* instead of a map, its behavior will be identical:
```php
$query = [
  'messages.inviteUsersToChat',  // dynamic parsing will enumerate the rest in TL schema order:
  123456,                        // chat_id
  [190, 336098765],              // user_ids
  false,                         // silent
];
```

This seems shorter but is harder to read when you return to this code after weeks.


## Types with multiple constructors (polymorphic types)

Let's consider the following TL fragment:
```
---types---
memcache.not_found = memcache.Value;
memcache.str_value value:string = memcache.Value;
memcache.numeric_value value:long = memcache.Value;

---functions---
memcache.get key:string = memcache.Value; 
```

Here *memcache.Value* is a polymorphic type, so we need to handle this on response parsing:
```php
$result = $response['result'];   // mixed; it's memcache.Value
// $result is one of the following here:
// ['_' => 'memcache.not_found']
// ['_' => 'memcache.str_value', 'value' => (string)]
// ['_' => 'memcache.numeric_value', 'value' => (int)]
$constructor_name = $result['_'];
switch($constructor_name) { /* ... */ }
// or, maybe this is enough in our particular case
$value = $result['value'];   // null / string / int
```


## Field masks

Now let's investigate, how to deal with bitmasks of sent/requested fields.
```
---types---
fileStorage.localCopy {fields_mask:#}
    hash_id:int
    cached_at:fields_mask.0?int
    available:fields_mask.1?Bool
    last_sync_info:fields_mask.2?(Maybe %fileStorage.SyncInfo)
    = fileStorage.LocalCopy fields_mask;

---functions---
fileStorage.getAllLocalCopies
    fields_mask:#
    file_name:string
    = Vector %(fileStorage.LocalCopy fields_mask);
```

As we see, *fields_mask* is a request parameter determining what fields we want to get from an engine.
```php
$query = [
  '_'           => 'fileStorage.getAllLocalCopies',
  'fields_mask' => (1<<0) | (1<<2),  // request 'cached_at' and 'last_sync_info'
  'file_name'   => '...'
];
```

Each row in response will we a hashmap with fields we requested:
```php
foreach ($result as $local_copy) {
  $local_copy['hash_id'];         // always exists
  $local_copy['cached_at'];       // exists, as 0 bit was requested
  /*$local_copy['available']*/    // not set, as 1 bit was not set
  $local_copy['last_sync_info'];  // it's Maybe T — requested, but may not exist due to TL schema
}
```


## How do TL types map onto 'mixed' internals

There are several built-in TL types, which you use to describe your TL types and functions. 

| TL type  |    PHP runtime Type   
|----------|---------------
| int, # | int (when storing more than 2^31, cut from C++ *int64_t* to C++ *int*) 
| long | int  
| float | float (when storing, converting to C++ *float* from C++ *double*)
| double | float 
| string | string
| bool | bool
| True | bool
| Vector\<T\> | mixed[] (array-vector)
| Maybe\<T\> | null or T representation
| Tuple\<T\>(n) | mixed[] (array-vector)
| Dictionary\<T\> | mixed[] (array-associative)
| {t:Type} | Just mixed[] as if no dependence
| fields_mask | mixed (null or value at runtime)
| !X | mixed[] (nested request)
| =X | mixed[] (nested response) 


## API reference for untyped TL

```warning
To use TL/RPC in plain PHP, [vkext](../../kphp-language/php-extensions/vkext.md) should be installed. 
```

<aside>rpc_tl_query_one($connection, array $query, $timeout = -1.0): int</aside>

Executes a query, like in the examples above. Returns query id.

<aside>rpc_tl_query($connection, array $queries, $timeout = -1.0, $ignore_answer = false): int[]</aside>

Executes multiple queries at once. Returns vector of query ids.

<aside>rpc_tl_query_result_one(int $query_id): mixed[] (resumable)</aside>

Wait for a query response, like in the examples above. Returns an array with either *'result'* or *'__error'*.

<aside>rpc_tl_query_result(int[] $query_ids): mixed[][] (resumable)</aside>

Waits for multiple responses at once. Returns an associative array keyed with *query_id* and valued like above.

<aside>rpc_tl_query_result_synchronously(int[] $query_ids): mixed[][]</aside>

Like, *rpc_tl_query_result()*, but not resumable. 

<aside>rpc_tl_pending_queries_count(): int</aside>

Returns a number of queries that have been sent, but not received yet.


## Manual RPC request storing and fetching

There are a number of low-level functions (see [functions.txt]({{site.url_functions_txt}}) for full list):
* *store_int()*, *store_string()* and others for TL built-in types
* *fetch_int()*, *fetch_string()* and company
* *fetch_lookup_int()* — gets an integer at buffer pointer, but doesn't offset fetching position

They allow you to manually prepare RPC output buffer and parse back received bytes — without TL schema. 

```note
Internal KPHP network layer and RPC implementation is the basics for [async programming](../../kphp-language/best-practices/async-programming-forks.md), because *rpc_tl_query_result()* and *rpc_tl_query_result_one()* are resumable.  
When the async model is combined with strict typing, a really great performance can be achieved. Click "Next".  
```
