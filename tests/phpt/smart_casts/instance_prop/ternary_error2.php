@kphp_should_fail
/passed nullable Bar value as \$bar argument to nonnull_bar2/
<?php

function test(Foo $foo) {
  // ->bar is used outside of the ternary.

  nonnull_bar1(($foo->bar !== null) ? $foo->bar : new Bar());
  nonnull_bar2($foo->bar);
}

function nonnull_bar1(Bar $bar) {}
function nonnull_bar2(Bar $bar) {}

class Foo {
  public ?Bar $bar = null;
}

class Bar {}

test(new Foo());
