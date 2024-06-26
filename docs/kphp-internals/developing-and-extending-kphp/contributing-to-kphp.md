---
sort: 4
---

# Contributing to KPHP

KPHP sources are not well documented, but they contain lots of code to do "by analogy" with.

You should definitely mind the border between [compiler](../kphp-architecture/internals-compiler.md) and [runtime](../kphp-architecture/internals-runtime.md). They are absolutely different parts but share the same contract of inferred types and codegenerated bindings. 

```note
This section is about contributing to KPHP itself.  
If you've developed a useful PHP library that successfully compiles with KPHP, feel free to share it [here]({{site.url_github_kphp_snippets}}).
``` 


## The release cycle

All development takes place [on GitHub]({{site.url_github_kphp}}). The "Issues" tab is a public issue tracker, also VK.com has a private one.

The *master* branch is protected for direct pushing, all changes are made using pull requests. Merging is disabled: only squashing and rebasing are allowed to keep a git history linear.

PRs can be merged only by KPHP maintainers. 

When a PR is merged to the *master* branch, it is automatically tested with CI. The CI is not hosted on GitHub: instead, a public GitHub repo is mirrored to private VK.com infrastructure, and all testing and packing are held there. Once in a couple of weeks, VK.com builds all public *.deb* packages — and that's the release point.

Release versions and .deb package versioning are **not semver** — they are linked to dates and build numbers. This is mostly due to historical and internal reasons.


## Walking through an imaginary situation #1

```tip
Let's imagine, that KPHP doesn't support `list()` with keys — and you want to implement it. 
```
```php
// imagine, that this works
list($a, $b, , $d) = f();

// but this doesn't
list(0 => $a, 3 => $d) = f();
// and this doesn't
['asdf' => $v1, $k => $v2] = $arr;
```

At first, you learn, how *list()* without keys is presented in AST — analyzing *op_list* usages and debugging.  
```
// list($a, $b, , $d) = f();
op_list
  op_var ($a)
  op_var ($b)
  op_lvalue_null
  op_var ($d)
  op_func_call (f)
```
Vertex *op_list* has accessors *→list()* to get all but last, and *→array()* to get a node after assignment sign. 

You decide to add *op_list_keyval* vertex type, to make AST be like this:
```
// list($a, $b, , $d) = f();
op_list
  op_list_keyval
    op_int_const 0
    op_var ($a)
  op_list_keyval
    op_int_const 1
    op_var ($b)
  op_list_keyval
    op_int_const 3
    op_var ($d)
  op_func_call (f)

// ['asdf' => $v1, $k => $v2] = $arr;
op_list
  op_list_keyval
    op_string 'asdf'
    op_var ($v1)
  op_list_keyval
    op_var ($k)
    op_var ($v2)
  op_var ($arr)
```

So, you introduce a new vertex to JSON, extending *meta_op_binary*, as it has two children. 

Then you implement this, taking care of several moments:
* convert a linear structure *op_list_ce* with possible *=>* (*op_double_arrow*) to a tree *op_list* with *op_list_keyval*, auto-calculating incremental indexes like PHP would do, skipping *op_lvalue_null*; probably, you want to deny *list($a,4=>$b)* — considering mixing keyed and unkeyed indexes a bad code style, though it works in PHP; the best place for this conversion is just after initial AST has been built — in *GenTreePostprocessPass*
* modify *class_assumptions.cpp* to recognize instances on the left side of arrow access, for example in case `list(1=>$a) = tuple(1,new A)` to correctly bind `$a->f()` to *A* method — based on existing code
* in right/left usages detection (*CalcRLF* pipe), specify, that in `list($k=>$v)` *$k* has access for reading (*val_r* — on the right side of assignment), but *$v* for writing (*val_l* — like on the left side)
* in control flow graph (*CFGBeginF* pipe), describe, that at first the left side will be calculated 
* in building type inferring edges (*CollectMainEdgesPass* pipe), don't forget a set-edge from the right-side of the i-th index of the *list* to the corresponding left-keyed value
* in the final check (*FinalCheckPass* pipe), when tuple or shape is on the right, check that all left keys are constant numbers/strings with presented indexes
* in C++ codegeneration, also implement valid shapes access: as earlier keyed lists were unsupported, all indexes were implicitly numeric, so shapes couldn't be assigned to them, and codegen part wasn't ready for it
* write tests that should pass and tests that should fail (should not compile)

Check for real *op_list_keyval* implementation of what's highlighted on here.


## Walking through an imaginary situation #2

```tip
Let's imagine, that KPHP doesn't support power operator `**` — and you want to add it to KPHP.  
```
Here you should not only modify the compiler but also add runtime functionality.

First, you think about priority: `$a ** $b + c` — what should happen first? `++$a ** $b` — and here? You learn PHP docs and decide, where to insert additions to *OpInfo::init_static()* for a new vertex type.  
You also ask yourself: `2**3**4` — is it *(2^3)^4* or *2^(3^4)*? In PHP, it works as the second, so you'll declare it as *right_opp* fixity.

Then you add *op_pow* vertex to JSON: it is also a binary operation.  
Also remember about `$a **= rhs` syntax, so add *op_set_pow* too (see current implementation in sources).

```
// $a ** -3
op_pow
  op_var ($a)
  op_int_const -3
```

Next, you need to parse `**` syntax in the lexer and convert it to AST. So, you introduce *tok_pow* and *tok_set_pow*. Calling `add_binary_op(priority, tok_pow, op_pow)` would perform token→AST conversion like for any other binary operation, taking priority and associativity into account.  

Then you think about type inferring. What type should using power operator lead to? `2**2` — it's *int*, okay. And `$a**2` — how should it be inferred, depending on *$a* type? Finally, you conclude with:
* it should emit *int* or *float* depending on base: *2^2* is *int*, but *2.5^2* is *float*
* but! we are sure of *int* only if exponent positive, as *2^-2* is *float* (0.25)
* so, to infer *int*, an exponent should be known at compile-time: *2^$a* shouldn't infer *int* even if *$a* is *int*, as it can be negative at runtime
* but! it's incorrect to treat *$a^$b* as *float*, because if *$a=$b=2* the result should be *4*, not *4.0*
* that's why *$a^$b* **can't be inferred** as *int*, but in **can be *int* at runtime**
* to support both *int* and *float* in this case, we should infer *mixed*

**After all simplifications**, you get: for constexpr positive integer exponent — *lca(int,base)*; otherwise, *float*.  
On type inferring step, you introduce *recalc_power()*, call it as necessary, and implement given logic.
  
Next, you need to tie codegeneration and C++ implementation together.   
As you resulted in having 3 different inferrings, you need at least 3 C++ functions: say, you name them *int_power()*, *float_power()*, and *mixed_power()* and implement them somewhere in runtime — in *runtime-core.h* for example; the last one not only returns *mixed* but accepts *mixed* also, even though arguments could be inferred as clean types, they would be implicitly converted to *mixed* — it's easier to create a single function without lots overloads in this case.  
On codegeneration of *op_pow*, you take the inferred result and output calling one of these functions.

To support `**=`, you consider how `+=` and similar are made: "set operator" depends on "base operator".

Also, you probably want to improve the *pow()* function inferring to make *pow(4,2)* also be *int*. The easiest way is to replace *pow()* call with *op_pow* in *GenTreePostprocessPass*.


## Walking through an imaginary situation #3

```tip
KPHP doesn't support *ARRAY_FILTER_USE_KEY* — and you want to fix this.  
```

`array_filter()` declaration in KPHP looks this way:
```
function array_filter ($a ::: array, callback ($x ::: ^1[*]) ::: bool) ::: ^1;
```
But *array_filter()* in PHP can accept a third parameter — *ARRAY_FILTER_USE_KEY* or *ARRAY_FILTER_USE_BOTH*. 

Before "fixing", ask yourself a question: why doesn't KPHP support this? Is it a fault, or is it on purpose?  
**Here it is on purpose**. If an input array is *int[]*, then the *callback* is called with an *int* argument. If *ARRAY_FILTER_USE_KEY* passed, then *callback* should be called with *mixed* (not item value, but item key), and if the third parameter is not constexpr, how *callback* should be typed? Sometimes it accepts *T*, sometimes it accepts *mixed*. If you pass a lambda with an *int* type hint, you'll get an unexpected error. Moreover, *T|mixed* won't work with instances. 

That's why *ARRAY_FILTER_USE_KEY* support can't be added **in a PHP way**.   
The solution is **to add a separate KPHP function** that does what you want. Declared like
```
function array_filter_by_key ($a ::: array, callback ($key ::: mixed) ::: bool) ::: ^1;
```

And it's a right way here. What you need to do:
1. Implement a PHP polyfill for *array_filter_by_key()* — to use it for development
2. Declare it in functions.txt — to make compiler know about it
3. Implement it in C++ — to make runtime know about it
```cpp
template<class T, class T1>
array<T> f$array_filter_by_key(const array<T> &a, const T1 &callback) { /* ... */ }
```
4. Write tests and create a pull request


## Conclusion

Adding new functionality is quite complicated and involves thinking about all compilation steps, but lots of existing code hints a lot, and writing tests always detects if you have missed something.

As it has been told many times, KPHP has always been a proprietary project with various presumptions and abandoned areas. Lots of lacking functionality — both at compile-time, at runtime, and at PHP stdlib support — are still to be added. 

If you have something to share, feel free to contribute.
