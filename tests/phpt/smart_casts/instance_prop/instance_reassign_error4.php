@kphp_should_fail
/passed nullable Bar value as \$bar argument to nonnull_bar/
<?php

function test(bool $cond) {
  $foo = new Foo();
  if ($foo->x) {
    if ($cond) {
      $foo = new Foo();
    } else {
      $foo = new Foo();
    }
    nonnull_bar($foo->x);
  }
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $x = null;
}

class Bar {}

test(true);
