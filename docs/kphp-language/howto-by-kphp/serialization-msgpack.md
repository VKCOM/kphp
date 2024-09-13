---
sort: 2
---

# Serialization, msgpack

Typically, when you want to *serialize* any data — you want to store it somewhere (in a file, in Memcache, transfer through a network) — and then *deserialize* it back.


## Plain PHP untyped version: easy but not optimal

If we talk about *common PHP way*, we got used to simple PHP functions:
```php
// $a is mixed[]
$a = ["id" => 1, "name" => "Alice"];

$str = serialize($a);
// $str = a:2:{s:2:"id";i:1;s:4:"name";s:5:"Alice";}
$a_copy = unserialize($str);
```
This is the easiest, but with the following disadvantages:
* the serialized string is big
* serializing an array of similar items stores all keys names, the string is big again
* deserialization dynamically creates hashtables, lots of memory allocations
* unserialize() can't be typed: it is just *mixed*

```warning
As a consequence, this approach is not acceptable to instances.
```


## KPHP version: binary instance serialization

```note
Initially this solution was focused on **typed Memcache**, but the actual usage scope is much wider.
```

```php
$u = new User();
$u->id = 1;
$u->name = "Alice";

$str = instance_serialize($u);
// $str = a short binary string
$u_copy = instance_deserialize($str, User::class);
```

Where *User* is a `@kphp-serializable` class:
```php
/**
 * @kphp-serializable
 */
class User {
  /**
   * @var int
   * @kphp-serialized-field 1
   */
  public $id;
 
  /**
   * @var string
   * @kphp-serialized-field 2
   */
  public $name;
 
  // any methods / static fields if needed
}
```


## What is @kphp-serializable

If **a class** is marked with this annotation, then:
* it can be **serialized** it to a binary string with *instance_serialize($object)*
* it can be **deserialized** back with *instance_deserialize($object, A::class)*
* all **instance fields** must be *@kphp-serialized-field {index}*
* all **indexes** must be unique in range 0..127
* if *@kphp-serialized-field none* — such fields are **skipped** during serialization
* fields can be not only primitives: you can **nest** other *@kphp-serializable* classes and arrays of them

```note
Manual handling `@kphp-serialized-field` is the basics for **versioning** and adding new fields, see below.
```


## Error handling

*instance_serialize() returns `string|null`*  
If an object is correct, *string* is returned. *null* only if there are recursively nested instances. That's why you mostly cast the result to a string.

*instance_deserialize() returns `instance|null`*  
If data is correct, *instance* is returned. Else *null* + warning. When data is incorrect? For example, you passed an empty string, or misprinted class name, or binary data don't coincide.


## Changing data structure, versioning

Imagine we have stored serialized *A* class to Memcache, and then added/deleted/renamed a field of *A*. 
Then old binary data **will deserialize correctly**.

It works due to `@kphp-serialized-field {index}` above each field. Serialization takes only indexes into account, with no field names. 
After having deleted the field, you must specify its index in `@kphp-reserved-fields`.

```tip
* Renaming and adding fields is ok
* Deleting is ok, but don't forget about *@kphp-reserved-fields*
* **Types of fields can't be changed!** Old binary data would not map to the new structure
```
## Inheritance
It is allowed to inherit classes with `@kphp-serializable` annotation, but there are restrictions:

```tip
* Class with instance field must be marked with `@kphp-serializable`
* Within annotation `@kphp-serialized-field {index}` index must be unique across whole inheritance hierarchy.
```

## Serializing floats

At runtime, PHP's *float* is C++ *double* stored as 8 bytes. That's why it takes 9 bytes for serialization (1 for the header).

In case you want to shrink binary size, and losing some precision is okay for you, annontate a field with `@kphp-serialized-float32`. Doubles will be cast to C++ *float* upon serialization and therefore require 5 bytes. Mostly useful for arrays of floats.


## Limitations

There is a shortlist, pretty logical:

1. **Don't change field types and indexes after any object of this class was stored somewhere**
2. *shape* fields not supported: if you need internal structure, create a separate class to imply versioning


## Profit compared to just PHP serialize()

The first, **profit in data size**. 
Usually, it is 3–10 times compact.

The second, **profit in speed**. 
*serialize()* traces hash tables and concatenates PHP strings. 
*unserialize()* allocates *mixed* and rebalances hash table on every field. 
*instance_serialize()* is pre-compiled for needed classes, no hashmaps, uses heap allocator, and gets a PHP string just once. 
*instance_deserialize()* creates an object without constructor invocation and fills fields, sequentially parsing binary data based on tags and compile-time known types.

The third, **typing**. 
We decode a string to a typed instance, not to `mixed`. 


## Internal implementation details

KPHP uses [msgpack](https://msgpack.org/) under the hood. It is a serialization format handling primitives (numbers, C strings, etc). 
Its C++ implementation is extendable to custom types, we use it for KPHP runtime types. 

PHP polyfill uses [this library](https://github.com/rybakit/msgpack.php) with custom implementation above: 
annotation handling, *@var* parsing, etc. which are done with reflection.

Every field has a unique *@kphp-serialized-field {index}*. Once assigned, it must not be changed, just as the field type. 
Field index works as id, the field name is not analyzed and not stored.

The binary format is simple:
```
[index1][value1][index2][value2]...
```

*indexes* are small numbers (1 byte), *values* depend on field type:

<table>
<thead>
<tr><th>type</th><th>format of value</th></tr>
</thead>
<tr>
    <td>int</td>
    <td>1, 2, 3 or 5 bytes
        <a class="i-details" href="https://github.com/msgpack/msgpack/blob/master/spec.md#int-format-family">details</a></td>
</tr>
<tr>
    <td>float</td>
    <td>9 bytes
        <a class="i-details" href="https://github.com/msgpack/msgpack/blob/master/spec.md#float-format-family">details</a></td>
</tr>
<tr>
    <td>string</td>
    <td>len (1 or 2 bytes) + binary-safe contents
        <a class="i-details" href="https://github.com/msgpack/msgpack/blob/master/spec.md#str-format-family">details</a></td>
</tr>
<tr>
    <td>array</td>
    <td>for <a href="https://github.com/msgpack/msgpack/blob/master/spec.md#array-format-family">vectors</a>: len + elements one by one<br>
        for <a href="https://github.com/msgpack/msgpack/blob/master/spec.md#map-format-family">hashmaps</a>: len + key + value + key + value + ...</td>
</tr>
<tr>
    <td>null</td>
    <td>1 byte
        <a class="i-details" href="https://github.com/msgpack/msgpack/blob/master/spec.md#nil-format">details</a></td>
</tr>
<tr>
    <td>bool</td>
    <td>1 byte
        <a class="i-details" href="https://github.com/msgpack/msgpack/blob/master/spec.md#bool-format-family">details</a></td>
</tr>
<tr>
    <td>mixed</td>
    <td>depends on the type (try not to use mixed, use strict type)</td>
</tr>
<tr>
    <td>Optional&lt;T&gt;</td>
    <td>1 byte if false / null; else T without prefix</td>
</tr>
<tr>
    <td>instance</td>
    <td>1 byte if null; else the instance itself without prefix</td>
</tr>
<tr>
    <td>tuple</td>
    <td>elements one by one, no prefix (as a vector, but len is known)</td>
</tr>
<tr>
    <td>shape</td>
    <td>not supported</td>
</tr>
</table>

When a new field is added — old data is mapped to a new instance correctly: the field is initialized by its default.  
When a field is deleted — everything is correct too: an old value is not written to any field.


## msgpack_serialize(mixed)

Besides *instance_serialize()*, there are 2 functions:
```
function msgpack_serialize(mixed $v) : ?string;
function msgpack_deserialize(string $v) : mixed;
```
They can be useful for various custom serialization, when data is not an instance. 


```tip
## Summary: how to use serialization

1. Create *@kphp-serializable* class, all fields are typed and annotated *@kphp-serialized-field {index}*
2. To store instance: *instance_serialize($obj)* and put this string to memcache/etc
3. To get from memcache/etc: fetch string and convert back with *instance_deserialize($str, A::class)*
4. While evolving, you can add/rename/delete fields of class; types cannot be changed
```

