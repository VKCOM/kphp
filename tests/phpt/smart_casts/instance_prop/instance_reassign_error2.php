@kphp_should_fail
/passed nullable Bar value as \$bar argument to nonnull_bar/
<?php

function test(Foo $foo) {
  if ($foo->x) {
    $foo = new Foo();
    if ($foo->y) {
      nonnull_bar($foo->x);
    }
  }
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $x = null;
  public ?Bar $y = null;
}

class Bar {}

test(new Foo());
