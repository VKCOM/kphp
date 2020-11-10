---
sort: 2
---

# Compile a sample PHP script

In this section, we will compile a PHP file, launch it as a web server, and examine codegenerated C++ sources.


## PHP code for compilation

Save it to `~/kphp-project/my.php` (note the path: it will be mapped inside Docker):
```php
<?php
// when launched, shows a form with <input> for entering a number
// when a number is submitted, prints this number squared and a clear button

class InputArgs {
  private $number = null;

  public function isNumberEntered() {
    return $this->number !== null;
  }      

  public function getNumberEntered() {
    return $this->number;
  }

  static function createFromUrl(): self {
    $input = new self;
    if (is_numeric($_GET['number']))
      $input->number = (int)$_GET['number'];
    return $input;
  }
}

function htmlHeader() {
  return "<h1>Hello from KPHP</h1>";
}

function htmlFormWithInput() {
  return "<form method=get>Enter a number: <input type=text value=123 name=number> <input type=submit value=Square></form>";  
}

function htmlNumberSquared($number) {
  $squared = $number ** 2;
  return "$number<sup>2</sup> = $squared<br><a href='/'>Clear</a>";
}

$input = InputArgs::createFromUrl();
if (!$input->isNumberEntered())
  echo htmlHeader(), htmlFormWithInput();
else
  echo htmlHeader(), htmlNumberSquared($input->getNumberEntered());
```


## Compile and run with Docker

If you have installed KPHP via the Docker registry, run the **container vkcom/kphp**:
```bash
docker run -ti -v ~/kphp-project/:/tmp/dev:rw -p 8080:8080 vkcom/kphp
```

Or, if you have build an image from the Dockerfile locally, run the **container kphp**:
```bash
docker run -ti -v ~/kphp-project/:/tmp/dev:rw -p 8080:8080 kphp
```

Then just compile `my.php` and run the server — **inside Docker**:
```bash
kphp /tmp/dev/my.php
./kphp_out/server -H 8080
```

Open <a href="http://localhost:8080" target="_blank">http://localhost:8080</a>. Your server is running!


## Compile and run without Docker

If you have installed KPHP locally, execute the following:
```bash
cd ~/kphp-project
kphp my.php
./kphp_out/server -H 8080
```

Open <a href="http://localhost:8080" target="_blank">http://localhost:8080</a>. Your server is running!


## Examine C++ sources

Stop the server with `Ctrl+C` and navigate to the `kphp_out/` directory to see its structure:
```bash
> cd kphp_out
> ls
kphp/  objs/  server
```

*server* is the binary that we have just launched.  
*kphp/* is **a folder with C++ sources**. Type `ls kphp_out/kphp` to see its contents:
* *o_25/* and other *o_* with numeric postfixes are folders with functions 
* *cl/* is a folder with classes
* other files and folders are various initializes and runners

For example, take a look at `kphp/cl/C@InputArgs.h` class:
```cpp
struct C$InputArgs: public refcountable_php_classes<C$InputArgs> {
  Optional < int64_t > $number{};
  /* ... */
};
```
Like *InputArgs* PHP class, it contains a single field. We didn't specify the types in PHP, but KPHP saw that only numbers and *null* were assigned to the *$number* field, making it `Optional < int64_t >` in C++.

Take a look at the *htmlHeader()* function. In PHP, it was `return "<h1>...</h1>"`. In C++, it looks this way:
```cpp
string f$htmlHeader() noexcept {
  return v$const_string$usaa89a82146500818;
}
```
As you can see, KPHP extracted a constant string to a separate variable to avoid creating it at runtime.

```note
It seems confusing, but `f$myFn` is a valid symbol in C++ code. Using *$* eliminates conflicts with native names.
```


## What's next?

Congratulations, you have compiled a small piece of PHP code! Here's what you can do now:
* Read about [type inferring](../kphp-language/static-type-system/type-inferring.md) if you're curious about how `Optional<int64_t>` came to be.
* Read about [instances](../kphp-language/static-type-system/instances.md). If you know about *ZVAL* in PHP, you may be surprised seeing C++ *struct*. 
* [Set up a new project](./quickstart-new-project.md) if you want to start experimenting with KPHP.
* Try to apply KPHP in already existing projects, [if you are ready for one thing](./compile-existing-project.md).
* Read out [FAQ](./faq.md) for the answers to the most common questions.
