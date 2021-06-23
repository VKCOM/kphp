@kphp_should_fail
/passed nullable Bar value as \$bar argument to nonnull_bar/
<?php

function test(Foo $foo) {
  // This condition is always false, but we don't care.
  if ($foo !== $foo) {
    $foo->bar = null;
  }
  nonnull_bar($foo->bar);
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $bar = null;
}

class Bar {}

test(new Foo());
