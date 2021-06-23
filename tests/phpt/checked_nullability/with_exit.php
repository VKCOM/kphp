@ok php7_4
<?php

/** @return ?Foo */
function new_nullable_bar($want_null) {
  if ($want_null) {
    return null;
  }
  return new Foo();
}

function test($bad) {
  $foo = new_nullable_bar(false);
  if (!$foo || $bad) {
    exit(0);
  }
  nonnull_foo($foo);
}

function nonnull_foo(Foo $foo) {}

class Foo {}

test(false);
