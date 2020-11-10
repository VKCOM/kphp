---
sort: 1
---

# PHP polyfills 

**Polyfills** are PHP implementations of functions natively supported by KPHP.  
Without polyfills, your code can be compiled, but can't be run by plain PHP.


## 'Polyfills', what is it?

Consider the following code:
```php
$first = array_first_value(['o' => 1, 't' => 2]);  // 1
```

Is can be compiled and run with KPHP, but executing it with PHP leads to 

```danger
Fatal Error:  Call to undefined function *array_first_value()*
```

This function is KPHP built-in. KPHP doesn't support internal arrays pointers and functions like *end()* and *current()* — to have less overhead. Instead, KPHP has analogs, *array_first_value()* for example.  
In PHP, it can be implemented as:
```php
// for PHP only!
function array_first_value(array $a) {
  reset($a);
  return current($a);
}
```

This is a **PHP polyfill** for KPHP *array_first_value()*. 


## Existing polyfills ready for your project

This repository contains everything you need (it's a Composer package):

<a href="{{ site.url_github_kphp_polyfills }}" class="btn-github-page">
  <span class="icon icon-github"></span>
  <span>kphp-polyfills</span>
</a>


Just install this package — and use KPHP built-in functions in PHP code.
