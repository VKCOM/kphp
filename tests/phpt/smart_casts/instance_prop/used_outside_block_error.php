@kphp_should_fail
/passed nullable Bar value as \$bar argument to nonnull_bar/
<?php

function test(Foo $foo) {
  // ->x is used after the block.
  if ($foo->x) {}
  nonnull_bar($foo->x);
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $x = null;
}

class Bar {}

test(new Foo());
