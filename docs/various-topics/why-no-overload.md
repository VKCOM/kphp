---
noprevnext: true
notopnavigation: true
---

# Why can't KPHP overload functions

Imagine the following code:
```php
$g1 = 0;
$g2 = 0.5;
 
function f($arg) {
  return $arg;
}
 
function setG1() {
  global $g1, $g2;
  $g1 = f($g2);
}
 
function setG2() {
  global $g1, $g2;
  $g2 = f($g1);
}
```

KPHP needs to infer types for *$g1*, *$g2*, *$arg* and *f()*.

Now the algorithm works this way:
* $g1 :: lca(int, f($g2)); $g2 :: lca(double, f($g1)); $arg :: lca(int, double); 
* $arg :: double
* f() :: double
* $g1 :: lca(int, double) = double
* $g2 :: lca(double, double) = double

If we want to automatically generate overloads for *f()* — so that *f($g1)* and *f($g2)* would be probably different functions *f??($g1)* and *f?($g2)* — we need to infer these overloads exactly.
* $g1 :: lca(int, f?($g2)); $g2 :: lca(double, f??($g1))
* f?($g2) :: lca(double, f??($g1))
* f??($g1) :: lca(int, f???($g2))
* f???($g2) :: lca(double, lca(int, lca(double, f????($g1)))
* f????($g1) :: lca(int, lca(double, lca(int, lca(double, lca(int, f?????($g2)))))
* f?????($g2) :: lca(double, lca(int, lca(double, lca(int, lca(double, lca(int, lca(double, f??????($g1)))))))
* …

In words: to infer the type of *$g1*, we need to know what overload of *f($g2)* to call, for this we need to know the type of *$g2*, for this we need to know what overload of *f($g1)* to call, for this we need to know the type of *$g1*. An infinite loop.

**Type inferring assumes, that it is a convergent process**. On every iteration, types become more concrete upwards the tree, while any types are changed.

```note
That's why overloading can't be done, as soon as types are inferred by KPHP, not set by a developer.  
Without apriori types information, it is a fundamental limitation.
```

In strictly typed languages we explicitly set types of arguments, returns, etc. — that exact apriori information. 
Heve we have PHP. Here we don't do it.  
That's why type inferring exists, which is based on types generalization (lowest common ancestor — lca) and assuming it to be converged.

<div>{%- include navigate-back.liquid -%}</div>
