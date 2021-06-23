@kphp_should_fail
/passed nullable Bar value as \$bar argument to nonnull_bar/
<?php

function test(Foo $foo) {
  // Assignment to $foo causes all property casts to be discarded.
  if (isset($foo->bar)) {
    $foo = new Foo();
    nonnull_bar($foo->bar);
  }
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $bar = null;
}

class Bar {}

test(new Foo());
