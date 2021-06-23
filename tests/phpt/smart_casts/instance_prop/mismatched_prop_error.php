@kphp_should_fail
/passed nullable Bar value as \$bar argument to nonnull_bar/
<?php

function test(Foo $foo) {
  // cast is applied to ->x, but ->y is used as an argument.
  if ($foo->x) {
    nonnull_bar($foo->y);
  }
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $x = null;
  public ?Bar $y = null;
}

class Bar {}

test(new Foo());
