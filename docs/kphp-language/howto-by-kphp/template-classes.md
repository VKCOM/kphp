---
sort: 3
---

# Template classes

Unlike template functions, KPHP doesn't offer any syntax for template classes yet.

The easiest way to achieve something like template classes is by using **traits**. Like so:
```php
trait ArrayContainer {
  // concrete class will define private $data of a concrete type

  function __construct(array $data) {
    $this->data = $data;
  }

  function isEmpty(): bool {
    return count($this->data) == 0;
  } 

  /** @return ?any */
  function last() {
    return $this->isEmpty() ? null : array_last_value($this->data);    
  }
}

class UserContainer {
  use ArrayContainer;
  /** @var User[] */
  private $data = [];
}

class OptionalIntContainer {
  use ArrayContainer;
  /** @var (int|false)[] */
  private $data = [];
}
```

So, you manually write "basic logic" and "specializations". This is not too bad, this makes you control what's going on. 
And very important: IDE referencing and autocompletion works quite well.


## What's the problem with something like @kphp-template-class?

While template functions are relatively simple and easy to use, classes are data holders. 
Having template classes means that types are nested arbitrary, instances are mixed with primitives and arrays, and so on.

KPHP is still limited to PHP syntax. 
The only way of declaring types would be in comments. 
Type declarations for primitives would be necessary and can't be deduced from local variables, because the class structure and binding should be known before inferring. 

Without full IDE support, it seems quite useless for real-life usage. Even if we write nested and mixed types in comments, IDE won't recognize it and won't suggest →properties, find usages won't work. This is not a big problem for template functions, because the scope is limited. But it turns out too disappointing for classes.

```note
Not so long ago, KPHPStorm was released.  
Hence, we can reconsider this question and provide a solution for template data structures with IDE support. 
```
