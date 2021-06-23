@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/pass mixed to argument \$x of ensure_int/
/pass mixed to argument \$x of ensure_string/
/pass mixed to argument \$x of ensure_float/
/pass mixed to argument \$x of ensure_array/
<?php

// Property smartcasts don't work for these casts.

class Foo {
  /** @var mixed */
  public $mixed;
}

function ensure_int(int $x) {}
function ensure_string(string $x) {}
function ensure_array(array $x) {}
function ensure_float(float $x) {}

function test(Foo $foo) {
  if (is_int($foo->mixed)) {
    ensure_int($foo->mixed);
  }
  if (is_string($foo->mixed)) {
    ensure_string($foo->mixed);
  }
  if (is_array($foo->mixed)) {
    ensure_array($foo->mixed);
  }
  if (is_float($foo->mixed)) {
    ensure_float($foo->mixed);
  }
}

test(new Foo());
