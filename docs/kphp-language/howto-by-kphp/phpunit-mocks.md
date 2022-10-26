---
sort: 4
---

# PHPUnit, mocks, etc

PHPUnit and other testing frameworks are based on reflection, mocks, redefine, and another fundamentally uncompilable syntax.
 
KPHP will never support this, but here is good news: you don't have to use PHPUnit together with KPHP.


## PHPUnit is only for dev — and KPHP is for production

Unit testing — like much other stuff like bundling — should be executed before deploying code to production.

As far as KPHP's syntax is backward compatible with regular PHP, you develop and test your site with PHP. And only when testing/linting/others succeed, you start compiling.

When compiling, test files are just not analyzed by KPHP at all.


## How to make test files invisible to KPHP

You invoke KPHP compiling at a concrete file (entrypoint). All reachable functions, all included files, all mentioned classes — they would be found iteratively and compiled at once.

If you don't require a file from the KPHP-reachable code, KPHP will never see it.

```php
// entrypoint.php
require_once 'urlmap.php';

function site_main() {
  $page_name = parseInputUrl();
  if ($page_name !== '')
    handleRequestToPage($page_name);
  else
    response404();
}  

site_main();


// testing.php
class MyTest {
  static function testSiteMain() {
    mock('site_main', function() { /* ... */ });
  }
}
```  

When compiling *entrypoint.php*, KPHP will process *urlmap.php* and all recursive includes, found classes, etc. But it **will not process *testing.php*** unless it is manually required.

Another option to exclude code from KPHP reachability is to use *#ifndef*:
```php
require_once 'urlmap.php';
#ifndef KPHP
require_once 'php-relative-dev-only-stuff.php';
#endif

#ifndef KPHP
if (isInvokedByPhpAndNeedsAnotherLogic())
  phpDevelopmentLogic();
else
#endif
site_main();
```

This topic is deeply discussed here: [What PHP code exactly would be compiled?](../kphp-vs-php/reachability-compilation.md)


```tip
It's easy to structure your code in a way, that PHPUnit will never be seen while compiling production.
```
