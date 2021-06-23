@ok php7_4
<?php

/** @return ?Foo */
function new_nullable_bar($want_null) {
  if ($want_null) {
    return null;
  }
  return new Foo();
}

/** @kphp-no-return */
function always_dies() {
  die('error');
}

function test($bad) {
  $foo = new_nullable_bar(false);
  if (!$foo || $bad) {
    always_dies();
  }
  nonnull_foo($foo);
}

function nonnull_foo(Foo $foo) {}

class Foo {}

test(false);
