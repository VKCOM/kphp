---
sort: 2
---

# Writing and running tests

Testing KPHP implies many scenarios. 


## 1. Unit tests

They are placed in `/tests/cpp/` folder, invoked with **gtest**, and use standard gtest macros like `TEST(…)`, `ASSERT_EQ` and others. Take a look at existing tests to find out their structure.

To compile and run unit tests, invoke CMake with the default configuration:
```bash
mkdir build
cd build
cmake ..
```

Then compile and run them:
```bash
make -j$(nproc) all
ctest -j$(nproc)
```


## 2. Functional tests for runtime and server

They perform complex scenarios like: compile and start HTTP server, send requests, compare responses, check HTTP headers, and so on. They are placed in the `/tests/python/tests/` folder, invoked with **pytest**.

Before running them, you should once perform a setup:
```bash
pip3 install --user -r tests/requirements.txt
```

To run, use just one-line command:
```bash
python3 -m pytest --tb=native -n5 tests/python/tests
```

To debug, use `-s` option and don't use `-n`


## 3. Running Zend tests (partially)

A list of KPHP-compatible Zend tests (from PHP sources) is maintained [manually](../../../tests/zend-test-list). 
To run them, clone `php-src` somewhere at first:
```bash
git clone https://github.com/php/php-src.git
cd php-src
git checkout 6326717dccf9e85dc41b3778692b790e2501fb2a
```

The following command will do all other stuff:
```bash
./kphp/tests/kphp_tester.py -j32 -d ./php-src --from-list ./kphp/tests/zend-test-list
```


## 4. Testing compiler and runtime: compare KPHP and PHP

```note
This is the most important testing part, and typically you'll write such tests when adding new KPHP features.
```

This scenario is the following:
1. There is a *.php* file
2. Run it with PHP
3. Compile and run it with KPHP
4. Compare results

All such tests are placed in the `/tests/phpt` folder. It's supposed that you have *vkext* installed and *PHP 7.4* executable. 

As a prerequisite, create a local copy of *kphp-polyfills*. This should be done only once:
```bash
git clone https://github.com/VKCOM/kphp-polyfills.git
cd kphp-polyfills
composer install
export KPHP_TESTS_POLYFILLS_REPO=$(pwd)
```

To run all tests (assume that you are in `kphp/` sources folder):
```bash
tests/kphp_tester.py
```

To run a specific test / set of tests, use tags and parts of filenames or folders:
```bash
tests/kphp_tester.py tag1
tests/kphp_tester.py substring_of_filename
tests/kphp_tester.py dir
```

To write such tests, examine how existing tests are made. For example:
```
@ok
<?php
require_once 'kphp_tester_include.php';

class Foo {
  var $v = 0;
  public function __construct(int $v) { $this->v = $v; }
}

/** @var tuple(Foo, string)[] */
$xs = [tuple(new Foo(5), "a"), tuple(new Foo(10), "b")];

foreach ($xs as [$foo, $s]) {
  var_dump($foo->v);
}
```

* `@ok` above is **a tag**; there can be many tags, for example, `@ok access` — you use them for filtering
* `require_once` is to make polyfills be available for plain PHP (not needed if you don't use KPHP specifics)
* it's a regular PHP code, you can use `#ifndef` and others

After tags, you can optionally set `ENV_VAR=value` on the next lines, before `<?php`.

There is a special tag `@kphp_should_fail` — to declare, that code doesn't compile, but it's expected:
```
@kphp_should_fail
/mix class A with non-class/
<?php
class A { public $x = 1; }

$a = new A;
if (0) 
  $a = false;
```

After *@kphp_should_fail*, an optional regexp can be present on the next line.

A special tag `@kphp_should_warn` is similar, indicating that KPHP compiles but produces a compilation warning. 

```tip
Always write such tests when implementing new PHP syntax features or adding new functions.
```
