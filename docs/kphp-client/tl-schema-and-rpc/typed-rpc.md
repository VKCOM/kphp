---
sort: 3
---

# Typed RPC

"Typed" means that you have PHP classes representing TL types and functions.  
These classes are generated with [tl2php](./tl2php.md) utility.

```tip
With untyped RPC, *$response['result']['members_count']* is `mixed`, while it is *int* in the TL schema.  
With typed RPC, *$result->members_count* is native `int`: clean typed without boxing at network fetching.
```


## Let's start with an example

The same example as previous, but typed. TL schema:
```
---types---
messages.inviteResult user_id:int already_in_chat:bool = messages.InviteResult;

---functions---
messages.inviteUsersToChat chat_id:long user_ids:%(Vector int) silent:bool = Vector %messages.InviteResult; 
```

Here is how we send a request:
```php
$query = new messages_inviteUserToChat(
  123456,                        // chat_id (int)
  [190, 336098765],              // user_ids (int[])
  false                          // silent (bool)
);

$connection = new_rpc_connection($host, $port);
$query_id = typed_rpc_tl_query_one($connection, $query);  // int
```

Here is how we get and parse the response:
```php
$response = typed_rpc_tl_query_result_one($query_id);     // RpcResponse
// check for errors
if ($response->isError()) {
  // ->getError()->error_code
}
// ::result() returns a valid object unless error
$result = messages_inviteUserToChat::result($response);
// it is messages_inviteResult[] 
foreach ($result as $row) {  
  $row->user_id;               // int
  $row->already_in_chat;       // bool
}
```


## Functions

```note
tl2php generates all classes to the \VK\TL namespace.
```

For TL function *'memcache.get'*, a class *VK\TL\memcache\Functions\memcache_get* exists.  
Typing *"new memget"* in IDE triggers suggestions and automatically inserts *use* statement.

All parameters can be either passed with constructor invocation or assigned later to *$query*. 


## Types, constructors, polymorphic types

**Type with one constructor**

```
stats.counterFrame = 
  object_id:%(stats.ObjectId)
  counter_type:int
  recursive_period:bool
  = stats.CounterFrame;
```

Is codegenerated into a simple class with corresponding fields:
```php
class stats_counterFrame {
  /** @var \VK\TL\stats\Types\stats_ObjectId */
  public $object_id = null;
  
  /** @var int */
  public $counter_type = 0;
  
  /** @var bool */
  public $recursive_period = false;
}
```

**Type with multiple constructors**

```
memcache.not_found = memcache.Value;
memcache.str_value value:string = memcache.Value;
memcache.numeric_value value:long = memcache.Value;
```

Is codegenerated into an interface (type) and classes (constructors):
```php
interface memcache_Value { /* ... */ }

class memcache_not_found implements memcache_Value {}

class memcache_str_value implements memcache_Value {
  /** @var string */
  public $value = '';
}

class memcache_numeric_value implements memcache_Value {
  /** @var int */
  public $value = 0;
}
```

That's why when a function returns a polymorphic type — this interface has to be checked with `instanceof`.

```
memcache.get key:string = memcache.Value; 
``` 

Here *memcache_get::result()* returns *memcache_Value*, and you should detect, what value actually is:
```php
$result = memcache_get::result($response);
if ($result instanceof memcache_not_found) {
  echo "key not found: " . $query->key;
} elseif ($result instanceof memcache_strvalue) {
  echo "key found: string " . $result->value;
}
```

(in untyped RPC we used *$result['_']* to get a constructor name — analog to *instanceof*)


## Checking for errors

*RpcResponse* has *isError()* and *getError()* methods. Usage pattern:
```php
$query = new classof_tlfunction(...); 
$response = typed_rpc_tl_query_one($query);
if ($response->isError()) {
  // engine unavailable / incorrect data / overflow / etc
  $response->getError()->error_code; // int
  $response->getError()->error;      // string
  return;
}
// if no errors — call ::result()
$result = classof_tlfunction::result($response);
// have a typed answer depending on tl function
$result->
```


## fields_mask

Whereas untyped RPC just misses elements in a hashtable, typed RPC uses *?T* to express absence.
```
fileStorage.localCopy {fields_mask:#}
    hash_id:int
    cached_at:fields_mask.0?int
    available:fields_mask.1?Bool
    last_sync_info:fields_mask.2?(Maybe %fileStorage.SyncInfo)
    = fileStorage.LocalCopy fields_mask;
```

This type is codegenerated into such class:
```php
class fileStorage_localCopy {
  const BIT_CACHED_AT_0 = (1 << 0);
  const BIT_AVAILABLE_1 = (1 << 1);
  const BIT_LAST_SYNC_INFO_2 = (1 << 2);
 
  /** @var int */
  public $hash_id = 0;
  
  /** @var ?int */
  public $cached_at = null;
 
  /** @var ?bool */
  public $available = null;

  /** @var ?\VK\TL\fileStorage\Types\fileStorage_SyncInfo */
  public $last_sync_info = null;
}
```

Constants are present to create queries: 
```php
$query->fields_mask = fileStorage_localCopy::BIT_CACHED_AT_0 | fileStorage_localCopy::BIT_LAST_SYNC_INFO_2; 
```

Non-requested fields will have default values — *null*, as they are nullable:
```php
foreach ($result as $local_copy) {
  $local_copy->hash_id;         // int
  $local_copy->cached_at;       // typed ?int, but int at runtime, as bit 0 was set
  (int)$local_copy->cached_at;  // to pass as clean type if needed 
  $local_copy->available;       // null, as 1 bit was not set
  $local_copy->last_sync_info;  // it's Maybe T — requested, but may be null due to TL schema
}
```


## How do TL types map onto PHP types

| TL type  |    PHP inferred Type (declared in *@var*)   
|----------|---------------
| int, # | int (when storing more than 2^31, cut from C++ *int64_t* to C++ *int*) 
| long | int  
| float | float (when storing, converting to C++ *float* from C++ *double*)
| double | float 
| string | string
| bool | bool
| True | bool
| Vector\<T\> | T[] (array-vector)
| Maybe\<T\> | ?T
| Tuple\<T\>(n) | T[] (array-vector)
| Dictionary\<T\> | T[] (array-associative)
| Anonymous arrays | instance[] (array-vector)
| {t:Type} | Instance specialization, like *VectorTotal__int*, see below
| fields_mask | ?T unless T can carry *null* itself
| !X | RpcFunction 
| =X | RpcFunctionReturnResult


## 'int' in TL is 4 bytes

TL scheme has *int* and *long* types, which both are presented as *int* in PHP.  

When storing a TL *int*, if the value doesn't fit 4 bytes, only low bytes are taken and an warning is output. However, if you call `set_fail_rpc_on_int32_overflow(true)` in advance, such a situation will lead to a storing error, not only to a runtime warning.


## Template specializations: what is Vector__stats_counterFrame

PHP does not have template classes. But TL schema has template types, like this:
```
vectorTotal {t:Type} total_count:int vector:%(Vector t) = VectorTotal t;
map {X:Type} {Y:Type} key:X value:Y = Map X Y;
```

To keep bound in type system restrictions and to make IDE autocomplete our intentions, a new PHP class is codegenerated for every *t* while compilation: *VectorTotal__int*, *VectorTotal__String*, etc.  
In fact, **we just write A__T instead of A\<T\>**.

If a template type is polymorphic, then all template constructors for a particular specialization also exist.


## Diagonal queries, proxied responses

TL schema allows using "some another function" as a type:
``` 
rpc.diagonalQueryTo actor_id:long query:!X = Vector (Maybe X);
```

It accepts *any RPC function* as a *query* — known only at runtime, not at compile-time — and returns a result, the type of which depends on the type of the result of that function.

Here is how this is supported:
* !X — is *RpcFunction*
* X as a result — *RpcFunctionReturnResult*

In fact, every RPC function — *memcache_get* and others — implements *RpcFunction*. And each of them returns *RpcFunctionReturnResult*. When you invoke a known function, you use *::result()*, and that's all. But invoking an unknown function leads to handle this manually.
```php
$sub_query = new messages_inviteUserToChat(...);
$query = new rpc_diagonalQueryTo($actor_id, $sub_query);

$query_id = typed_rpc_tl_query_one($connection, $query);
$response = typed_rpc_tl_query_result_one($query_id);
if ($response->isError()) {
  return;
}
 
// it returns Vector (Maybe X), so its ::result() is RpcFunctionReturnResult[]
$vector_of_results = rpcProxy_diagonal::result($response);
foreach ($vector_of_results as $actor_answer) {  // ?RpcFunctionReturnResult
  if (!$actor_answer)    // filter out 'null' of Maybe
    continue;
 
  // we know we had sent messages_inviteUserToChat — here it is, that X          
  // difference! not ::result($response) — another function and another argument
  $invite_result = messages_inviteUserToChat::functionReturnValue($actor_answer);
  $invite_result->user_id;
} 
``` 


## Flat optimization: why PHPDoc sometimes differs from TL schema

TL can have types, that automatically **flatten**. For example, a *'geo'* engine has to operate with ids like *[int,int,int,...]*. Not to declare *%(Tuple int n)* every time, you create an **alias**:
```
math.objectId {n:#} %(Tuple int n) = math.ObjectId n;
```

Then you use these types in other types or functions:
```
math.deleteObject n:# object_id:%(math.ObjectId n) = Bool;
```

This field would be represented in PHP code as
```php
  /** @var int[] */
  public $object_id = [];      // not math_objectId!  
```

A type is automatically flattened if: this is a not polymorphic type, and its constructor has exactly one argument, and this argument is not hidden with fields mask.  
Flatting is applied to untyped RPC also as an option, but here it looks explicitly.


## vkext

You should install [vkext](../../kphp-language/php-extensions/vkext.md) to be able to use RPC in plain PHP.  
It behaves exactly the same as KPHP, creating instances of the same classes, but works absolutely differently. While KPHP knows full lists of classes and full TL schema at compile-time, vkext has to depend on class names.

```warning
For this reason, classes must not only contain proper fields but be named correctly and placed to pre-known paths. That's why manual writing is inappropriate, so use tl2php generator.
```
