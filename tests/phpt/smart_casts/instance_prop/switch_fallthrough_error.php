@kphp_should_fail
/passed nullable Foo value as \$bar argument to nonnull_foo2/
<?php

function test(Foo $foo) {
  $tag = 10;
  switch ($tag) {
  case 20:
    if ($foo->nullable_foo === null) {
      return;
    }
    nonnull_foo1($foo->nullable_foo);
    // fallthrough
  case 30:
    nonnull_foo2($foo->nullable_foo);
    break;
  }
}

function nonnull_foo1(Foo $foo) {}
function nonnull_foo2(Foo $bar) {}

class Foo {
  public ?Foo $nullable_foo;
}

test(new Foo());
