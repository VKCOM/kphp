---
sort: 8
title: Generic classes C&lt;T&gt;
---

# Generic classes Container\<T\>

**They are "work in progress".**

They will be similar to generic functions: with `@kphp-generic` over the class, `T` in `@var` over fields and `@param/@return` over methods. Inheritance will also be supported, as well as generic interfaces. 

An example from the future:

```php
/**
 * @kphp-generic T
 */
class Container {
  /** @var T[] */
  private array $items = [];

  /**
   * @param T $e
   */
  function append($e) { 
    $this->items[] = $e; 
  }

  /**
   * @return T[]
   */
  function getAll() { 
    return $this->items;
  }

  /**
   * @kphp-generic T2
   * @param Container<T2> $rhs
   */
  function equalSizes($rhs) {
    return count($this->items) === count($rhs->items);
  }

  /**
   * @return Container<mixed>
   */
  function toMixed() {
    $c_mixed = new self/*<mixed>*/();
    foreach ($this->items as $i)
      $c_mixed->append($i);
    return $c_mixed;
  }

  /**
   * @kphp-generic T
   * @return Container<T>
   */  
  static function createEmpty() {
    return new Container/*<T>*/;
  }
}
```

```note
Generic classes will be available in late 2022 or early 2023 — when all expected features in KPHP are ready and KPHPStorm supports required cases.
```
