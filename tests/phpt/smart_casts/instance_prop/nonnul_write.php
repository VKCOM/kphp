@ok php7_4
<?php

function nonnull_foo(Foo $foo) {}

class Foo {
  public ?Foo $next;
}

function test(Foo $foo) {
  if (is_null($foo->next)) {
    $foo->next = new Foo();
  }
  nonnull_foo($foo->next);
}
