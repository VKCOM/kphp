@kphp_should_fail
/passed nullable Bar value as \$bar argument to nonnull_bar/
<?php

function test(Foo $foo) {
  // We don't handle this case.
  // foo->bar remains ?Bar-typed.
  $foo->bar = new Bar();
  nonnull_bar($foo->bar);
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $bar = null;
}

class Bar {}

test(new Foo());
