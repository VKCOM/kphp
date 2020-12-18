---
sort: 3
---

# Available PHPDoc annotations

Here we list PHPDoc annotations that are being parsed by KPHP.

```tip
Skip this page at first reading. Look through it later, when you start using KPHP.
```


## @kphp-... tags for functions

<aside>@kphp-warn-unused-result</aside>

This tag is for non-void functions. If an invocation doesn't use the result (not stored to a variable, not passed as argument), you'll get a compilation error.

<aside>@kphp-should-not-throw</aside>

If a function turns out to be able to throw an exception, you'll get a compilation error.

<aside>@kphp-inline</aside>

Marks this function as 'inline' for gcc, and codegen implementation places it in .h file (not .cpp).  
**Note.** KPHP auto-inlines simple functions, so you typically don't have to use this annotation.

<aside>@kphp-required</aside>

Forces kphp to start compiling this function even if it is not used explicitly. Typically, is required for callbacks passed as strings, as they are resolved much later.

<aside>@kphp-no-return</aside>

Indicates, that this function never returns (always calls *exit()*). While building a control flow graph, KPHP treats all code after such functions' invocations as inaccessible, does not warn on missing break, etc.

<aside>@kphp-pure-function</aside>

Tells KPHP that this function is pure: the result is always the same on constant arguments. Therefore, a function can be called in constant arrays for example.

<aside>@kphp-template [T] {parameters}</aside>
<aside>@kphp-return {description}</aside>

See [template functions](../howto-by-kphp/template-functions.md).

<aside>@kphp-sync</aside>

Marks a function to be sync. If such a function turns out to be resumable, you'll get a compilation error.  
See [async programming](../best-practices/async-programming-forks.md).

<aside>@kphp-disable-warnings {warningName}</aside>

Suppress compilation warning of a specified function.  

<aside>@kphp-profile</aside>
<aside>@kphp-profile-allow-inline</aside>

See [embedded profiler](../best-practices/embedded-profiler.md).  

<aside>@kphp-flatten</aside>

Makes assembler code of this function aggressively inline _everything_, avoiding `callq`. Do not use it without examining assembler output!  

<aside>@kphp-warn-performance {inspections}</aside>
<aside>@kphp-analyze-performance {inspections}</aside>
Available inspections: `array-merge-into`, `array-reserve`, `constant-execution-in-loop`, `implicit-array-cast`.
These annotation are propagated to all reachable functions by the callstack.
See [TODO](../TODD.md).  


## @kphp-... tags for classes

<aside>@kphp-serializable</aside>
<aside>@kphp-serialized-field {index}</aside>
<aside>@kphp-reserved-fields {indexes}</aside>

See [serialization and msgpack](../howto-by-kphp/serialization-msgpack.md).

<aside>@kphp-immutable-class</aside>

Fields of an immutable class are **deeply constant** and can be set only in *__construct()*. All nested instances must be also immutable.  
Such instances can be stored in an [instance cache](../best-practices/shared-memory.md).


## @kphp-... tags for fields

<aside>@kphp-const</aside>

Class fields marked *@kphp-const* can be set only in *__construct()*. Constantness is **not deep**: array elements and nested instance properties can still be modified, so constant is the field itself.


## @var / @param / @return — KPHP uses them to restrict types

This is fully discussed in [PHPDoc is the way you declare types](../static-type-system/phpdoc-to-declare-types.md).


```tip
[KPHPStorm plugin](../kphpstorm-ide-plugin) knows about all doc tags, suggests, and validates them.
```

