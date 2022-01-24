---
sort: 5
---

# JSON encode and decode

Here we discuss, how to transform your key-value data to JSON and back. 


## For mixed: just json_encode()/json_decode()

If you have a `mixed` or `mixed[]`, you can just use standard PHP functions:

```php
// $a is mixed[]
$a = ['id' => 1, 'name' => "Alice"];
$json = json_encode($a);
$a_copy = json_decode($json, true);
```

Don't forget *true* as second *json_decode* parameter (to return assoc array, not stdClass).


## For instances: not ready yet, only manual

Unlike *instance_serialize()*, for now, there is no similar method for JSON and back yet.

As a workaround, you can use a temporary array:
```php
$user = new User;
$json = json_decode(to_array_debug($user));
```

It will convert an instance to `mixed[]` deeply first, which is slow, but the only way for now.

For back conversion, only manual fields assignments with type casting are possible currently:
```php
$user_arr = json_decode($json, true);
$user = new User;
$user->id = (int)$user_arr['id'];
$user->name = (string)$user_arr['name'];
```

```note
While using KPHP proprietary, we did not need this, that's why it is not ready for now.  
This will probably be done in the future, with some *@annotations-for-json* for skipping/renaming fields and so on.
```
